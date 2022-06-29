#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "smart_speaker.h"
#include "atom_echo.h"
#include "sonitalk.h"
#include "attestation.h"
#include "wifi.h"
#include "http_server.h"

static QueueHandle_t attestation_nonce_queue;
static QueueHandle_t attestation_result_queue;

static char logPrefix[] = "MainLoops";

void attestation_loop(void *pvParameter)
{
    uint8_t *received_msg = NULL;
    while (1)
    {
        if (!sonitalkReceive_stop)
        {
            ESP_LOGI(logPrefix, "Starting attestation listener\n");
            ledSet(COL_SONI_LISTEN);
            received_msg = sonitalk_receive();

            if (received_msg != NULL)
            {
                for (int i = 0; i < 4; i++)
                {
                    ESP_LOGW(logPrefix, "Received %d: %02X", i, received_msg[i]);
                }

                // Message received, send it to attestation task
                xQueueSend(attestation_nonce_queue, (void *)received_msg, 10);

                // Wait for answer while continuing voice-assistant
                uint32_t result;
                while (1)
                {
                    if (xQueueReceive(attestation_result_queue, (void *)&result, portMAX_DELAY) == pdTRUE)
                    {
                        break; // Stop outer while-loop
                    }
                    vTaskDelay(1);
                }

                ledSet(COL_SONI_PLAY);
                // Send Result
                ESP_LOGW(logPrefix, "Attestation Result: 0x%08X (%d)\n", result, result);

                sonitalk_send((uint8_t *)&result);

                free(received_msg);

                sonitalkReceive_stop = true;
            }
        }
        vTaskDelay(10);
    }
}

void voice_assistant_loop(void *pvParameter)
{
    uint32_t buttonPressDuration_ms;
    while (1)
    {
        ledSet(COL_WAIT_BUTTON);

        // Wait until button is pressed
        while (!isButtonDown())
            vTaskDelay(10);

#define LONG_PRESS_MS 3000

        // Wait for button release and count passed time
        buttonPressDuration_ms = 0;
        while (isButtonDown() && buttonPressDuration_ms < LONG_PRESS_MS)
        {
            buttonPressDuration_ms += 100;
            vTaskDelay(100 / portTICK_PERIOD_MS);
        };

        // Decide for action based on Duration
        ESP_LOGI(logPrefix, "Held Button for %d Millisec", buttonPressDuration_ms);

        if (buttonPressDuration_ms < LONG_PRESS_MS)
        {
            ESP_LOGI(logPrefix, "Start Voice Assistant");
            smart_speaker();
            ESP_LOGI(logPrefix, "Fin Voice Assistant");
        }
        else
        {
            ESP_LOGI(logPrefix, "Starting WiFi AP");
            ledSet(COL_WAIT_APCON);
            wifi_set_mode(MODE_WIFI_AP);

            // Wait for button release
            while (isButtonDown())
                vTaskDelay(10 / portTICK_PERIOD_MS);

            // Wait for User-Connection
            ESP_LOGI(logPrefix, "Waiting for Connection to AP");
            while (AP_ConnectedCnt < 1)
            {
                if (isButtonDown())
                {
                    ESP_LOGW(logPrefix, "Aborting Waiting for AP-Connection and Attestation...");
                    break;
                }
                vTaskDelay(10);
            }

            // Cancelled by user
            if (AP_ConnectedCnt < 1)
            {
                while (isButtonDown())
                    vTaskDelay(10);

                // Comment this to make the
                //wifi_set_mode(MODE_WIFI_STA);
                //continue;
            }

            // Start Listening task
            ESP_LOGI(logPrefix, "Requesting Attestation Listener Start");
            sonitalkReceive_stop = false;
            while (!sonitalkReceive_isRunning)
                vTaskDelay(1); // Wait until receiver started before listening to button

            // Receiver is listening on other thread,
            while (!sonitalkReceive_stop || sonitalkReceive_isRunning)
            {
                if (isButtonDown())
                {
                    ESP_LOGW(logPrefix, "Aborting Attestation...");
                    // Stop Receive
                    sonitalkReceive_stop = true;
                    while (sonitalkReceive_isRunning)
                        vTaskDelay(1); // Wait until receiver stopped
                    ESP_LOGI(logPrefix, "Halted Attestation!");

                    while(isButtonDown())
                        vTaskDelay(10);
                }
                vTaskDelay(1);
            }

            // Go back to station-mode
            ESP_LOGI(logPrefix, "Fin Attestation");
            wifi_set_mode(MODE_WIFI_STA);

            ledSet(COL_WAIT_BUTTON);
        }
    }
}

void attestation_task(void *pvParameter)
{
    uint32_t nounce;
    uint8_t sha256hash[32]; // Sha256 = 256Bit = 32Byte
    while (1)
    {
        // Wait endlessly for nounce
        while (!xQueueReceive(attestation_nonce_queue, (void *)&nounce, portMAX_DELAY))
            ;
        ESP_LOGW(logPrefix, "Got nounce: %d\n", nounce);

        for (int i = 0; i < 5; i++)
        {
            if (i == 0)
            {
                attestation_calculateHash((uint8_t *)&nounce, sha256hash);
            }
            else
            {
                attestation_calculateHash(sha256hash, sha256hash);
            }
        }
        ESP_LOGW(logPrefix, "Got result: %02X %02X %02X %02X\n", sha256hash[0], sha256hash[1], sha256hash[2], sha256hash[3]);

        uint32_t *firstfourbyte = (uint32_t *)sha256hash;
        xQueueSend(attestation_result_queue, (void *)firstfourbyte, 10);
    }
}

void app_main()
{
    // Wait a second for UART (MCU is not the issue, but Plattformio Upload&Monitor has lag before listening)
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (nvslib_begin()) {
        nvslib_erase_all(false);
        nvslib_commit();
        nvslib_close();
    }

    // Used for both Client and SoftAP
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_set_mode(MODE_WIFI_STA);
    start_webserver();

    attestation_nonce_queue = xQueueCreate(5, sizeof(uint32_t));
    attestation_result_queue = xQueueCreate(5, sizeof(uint32_t));

    init_button();
    init_led();
    attestation_initPartitionTable();

    xTaskCreatePinnedToCore(&attestation_loop, "sonitalk_task", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&voice_assistant_loop, "voice_assistant_task", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&attestation_task, "attestation_task", 8192, NULL, 5, NULL, 1);
}