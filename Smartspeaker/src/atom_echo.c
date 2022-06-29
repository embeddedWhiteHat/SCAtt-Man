
#include "esp_system.h"
#include <driver/i2s.h>

#include "atom_echo.h"

static int currentI2SMode = -1; // Rember current state of I2S channel (Mic or Speaker)

void init_led()
{
    gpio_config_t ledConf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = GPIO_SEL_27};
    gpio_config(&ledConf);
}


inline void IRAM_ATTR led_delay(volatile int ns)
{
    while (ns > 0)
        ns -= 200;
}

inline void led_sendZero()
{
    gpio_set_level(27, 1);
    led_delay(400);
    gpio_set_level(27, 0);
    led_delay(850);
}

inline void led_sendOne()
{
    gpio_set_level(27, 1);
    led_delay(800);
    gpio_set_level(27, 0);
    led_delay(450);
}

void ledSet(uint32_t r, uint32_t g, uint32_t b)
{
    uint32_t col = (g << 16) + (r << 8) + b;

    uint32_t pos = 1 << 23;
    while (pos > 0)
    {
        if ((col & pos) > 0)
            led_sendOne();
        else
            led_sendZero();
        pos = pos >> 1;
    }
    led_delay(300000);
}

void init_button()
{
    gpio_config_t btnConf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = GPIO_SEL_39};
    gpio_config(&btnConf);
}

bool isButtonDown(void)
{
    return (gpio_get_level(39) == 0);
}

bool InitI2S(int mode, uint32_t sample_rate)
{
    if (currentI2SMode == mode)
        return ESP_OK;

    esp_err_t err = ESP_OK;

    i2s_driver_uninstall(SPEAK_I2S_NUMBER);
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = sample_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 258,
    };
    if (mode == MODE_MIC)
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    }
    else
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;
        i2s_config.tx_desc_auto_clear = true;
    }

    err += i2s_driver_install(SPEAK_I2S_NUMBER, &i2s_config, 0, NULL);
    i2s_pin_config_t tx_pin_config;

    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;

    err = i2s_set_pin(SPEAK_I2S_NUMBER, &tx_pin_config);
    err += i2s_set_clk(SPEAK_I2S_NUMBER, sample_rate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    if (err == ESP_OK)
        currentI2SMode = mode;

    return err;
}

void InitI2SSpeakerOrMic(int mode)
{
    esp_err_t err = ESP_OK;

    i2s_driver_uninstall(SPEAK_I2S_NUMBER);
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 6,
        .dma_buf_len = 60,
    };
    if (mode == MODE_MIC)
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    }
    else
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;
        i2s_config.tx_desc_auto_clear = true;
    }

    err += i2s_driver_install(SPEAK_I2S_NUMBER, &i2s_config, 0, NULL);
    i2s_pin_config_t tx_pin_config;

    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;

    err += i2s_set_pin(SPEAK_I2S_NUMBER, &tx_pin_config);
    err += i2s_set_clk(SPEAK_I2S_NUMBER, 16000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    if (err == ESP_OK)
        currentI2SMode = mode;
}

void play_buffer(int16_t *buffer, int buff_size)
{
    size_t bytes_written;
    i2s_write(SPEAK_I2S_NUMBER, (uint8_t *)buffer, buff_size * sizeof(int16_t), &bytes_written, portMAX_DELAY);
}

void readMic(int16_t *dst, const size_t byteCnt)
{
    size_t offset = 0;
    size_t byte_read = 0;
    uint8_t *dst8 = (uint8_t *)dst;

    while (offset < byteCnt)
    {
        size_t toRead = byteCnt - offset > 1024 ? 1024 : byteCnt - offset; // Limit read to 1024 byte or space left in buffer
        i2s_read(SPEAK_I2S_NUMBER, dst8 + offset, toRead, &byte_read, (100 / portTICK_RATE_MS));
        offset += byte_read;
    }
}

// Accuracy is 1/CONFIG_FREERTOS_HZ Seconds. By default CONFIG_FREERTOS_HZ is 100, so 1/100s = 10ms
double getTimeRTOS()
{
    return (double)xTaskGetTickCount() / (double)CONFIG_FREERTOS_HZ;
}

void rtos_delay_ms(uint16_t duration)
{
    vTaskDelay(duration / portTICK_PERIOD_MS);
}

void benchStart(const char *name)
{
}

void benchEnd()
{
}