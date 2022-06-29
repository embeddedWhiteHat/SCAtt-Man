#include "sonitalk_receive.h"
#include "sonitalk.h"
#include "atom_echo.h"

#include <math.h>
#include <string.h>

#include "esp_dsp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

static const char logPrefix[] = "SoniTalkRecv";

#define N_SAMPLES 512
//#define USE_WINDOW // Not using one would save N_SAMPLES*4 bytes of RAM
#define START_FRAME_ACCEPT_RATIO 8 / 10

/**
 * FREQ_ACCEPT_THRESHOLD is a fixed pseudo-dBm value at which frequencies count as active
 * Once this value passed, a variable threshold algorithm kicks in:
 * The loudest frequency is determined and all other frequencies within FREQ_THRESHOLD_MARGIN
 * also count as active (ie. bits set to 1)
 */
#define FREQ_ACCEPT_THRESHOLD 65
#define FREQ_THRESHOLD_MARGIN 3.0

bool sonitalkReceive_stop = true;
bool sonitalkReceive_isRunning = false;

// Timespan of Samples one fft represents in milliseconds
#define FFT_TIMESTEP_MS (1000 * N_SAMPLES / SAMPLE_RATE_MIC)
static const double fftTimestep_ms = 1000.0 * (double)N_SAMPLES / (double)SAMPLE_RATE_MIC;

// Each element of the fft-result represents multiple frequencies of our total Sampling rate, depending on the Ratio between fft-length and sampling-rate
// eg: If the fft-length is too low, we might not even be able to tell 500hz and 600hz apart (ie resolution>100)
static const double fftBinResolition = (double)SAMPLE_RATE_MIC / N_SAMPLES;

// Note: Lowest freq we can recognize is (SAMPLE_RATE_MIC/N_SAMPLES) Hz
static int16_t *sampleBuffer = NULL;
static float *samplesIm_f32 = NULL;
static float *fftResLog = NULL;
#ifdef USE_WINDOW
static float *window = NULL;
#endif

// Reads microphone (16bit operation) and directly parses it into the Complex float format dsp uses
// Also applies window
void readMic_f32_Im(float *dst, int16_t *buffer, size_t byteCnt)
{
    readMic(buffer, byteCnt);
    for (int i = 0; i < byteCnt / 2; i++)
    {
// write real part
#ifdef USE_WINDOW
        dst[i * 2 + 0] = (float)buffer[i] * window[i];
#else
        dst[i * 2 + 0] = (float)buffer[i];
#endif
        // write imaginary part
        dst[i * 2 + 1] = 0;
    }
}

// Buffer to store all frequency-results of one Block-Size (ie. Block size 200ms, one fft per 50ms, it takes 4 frames to store all results)
// +1 incase of integer flooring
#define ONE_DATUM_BUFF_LEN (int)(1 + (BLOCK_LENGTH / FFT_TIMESTEP_MS))

// +1 for End-Frame
#define FULL_DATA_BUFF_LEN ((ONE_DATUM_BUFF_LEN) * (NR_OF_MESSAGES) + 1)

#define SIZEOF_FRAME (sizeof(bool) * NR_OF_FREQUENCIES)

static bool **recv_dataframes = NULL;

// Keeps track of passed time. Resets on a new transmission
static double soniTalkTime = 0;
static double lastRTOStime = 0;

static bool isReceiving = false; // Currently receiving a SoniTalk sequence
int startFrameNo = 0;            // Number of start-frames received in the last tone-length-time
int curFrameNo = 0;              // Counter inside recv_dataframes

float activation_threshold = FREQ_ACCEPT_THRESHOLD;

float getFreqValue(int freq_hz)
{
    int bin = (double)freq_hz / fftBinResolition;
    return fftResLog[bin];
}

bool isFreqActive(int freq_hz)
{
    float activityVal = getFreqValue(freq_hz);
    return activityVal > activation_threshold;
}

/**
 * Searches for the most active (loudest) frequency and calculates an
 * activation threshold at which the other freqs also count as active
 * See FREQ_THRESHOLD_MARGIN
 */
void calcActivationThreshold()
{
    float max = 0;
    for (int i = 0; i < NR_OF_FREQUENCIES; i++)
    {
        float freq = getFreqValue(BASE_FREQUENCY + i * FREQUENCY_SEPARATION);
        if (freq > max)
            freq = max;
    }

    if (max < FREQ_ACCEPT_THRESHOLD)
        activation_threshold = FREQ_ACCEPT_THRESHOLD;
    else
        activation_threshold = max - FREQ_THRESHOLD_MARGIN;
}

// dst has to be of size NR_OF_FREQUENCIES
void getFreqActivityTable(bool *dst)
{
    for (int i = 0; i < NR_OF_FREQUENCIES; i++)
    {
        dst[i] = isFreqActive(BASE_FREQUENCY + i * FREQUENCY_SEPARATION);
    }
}

// TODO: realize for arbitrary even NR_OF_FREQUENCIES (maybe as a stream or somthing ;P )
#if NR_OF_FREQUENCIES == 8
uint8_t bitArrToVal(bool *frame)
{
    uint8_t res = 0;
    uint8_t pot = 1;
    for (int i = 0; i < 8; i++)
    {
        if (frame[7 - i])
            res += pot;
        pot *= 2;
    }
    return res;
}
#elif NR_OF_FREQUENCIES == 16
uint16_t bitArrToVal(bool *frame)
{
    uint16_t res = 0;
    uint16_t pot = 1;
    for (int i = 0; i < 16; i++)
    {
        if (frame[15 - i])
            res += pot;
        pot *= 2;
    }
    return res;
}
#elif NR_OF_FREQUENCIES == 4
uint8_t bitArrToVal(bool *frameMSB, bool *frameLSB)
{
    uint8_t res = 0;
    uint8_t pot = 1;
    for (int i = 0; i < 4; i++)
    {
        if (frameLSB[3 - i])
            res += pot;
        pot *= 2;
    }
    for (int i = 0; i < 4; i++)
    {
        if (frameMSB[3 - i])
            res += pot;
        pot *= 2;
    }
    return res;
}
#else
#error NR_OF_FREQUENCIES has to be 4, 8 or 16
#endif

void findBestFit(bool **frames, int count, bool *dst)
{
    // Idea: Count how often every frame occured amongst the others, take the one with highest hit counter
    int *resCounter = (int *)calloc(1, count * sizeof(int));

    for (int idxRes = 0; idxRes < count; idxRes++)
    {
        const bool *cmpFrame = frames[idxRes]; // Makes reading this monstrosity a bit easier

        for (int idxCmp = 0; idxCmp < count; idxCmp++)
        {
            if (idxRes != idxCmp)
            { // Dont compare with itself
                if (0 == memcmp(cmpFrame, frames[idxCmp], SIZEOF_FRAME))
                {
                    // Its a match!
                    resCounter[idxRes]++;
                }
            }
        }
    }

    // Now find highest hit-count
    int maxIdx = 0;
    for (int i = 1; i < count; i++)
    {
        if (resCounter[i] > resCounter[maxIdx])
            maxIdx = i;
    }

    // Print warning, if all differ
    if (resCounter[maxIdx] == 0)
    {
        ESP_LOGW(logPrefix, "Could not find BestFit, because all input frames differ");
    }

    // Copy result (or first frame if all differ)
    memcpy(dst, frames[maxIdx], SIZEOF_FRAME);
    free(resCounter);
}

void findIdxForTimeblock(int blockNo, int *start, int *frmCount)
{
    double blockStart_ms = blockNo * (BLOCK_LENGTH);
    double blockEnd_ms = (blockNo + 1) * (BLOCK_LENGTH);

    int idxStart = blockStart_ms / fftTimestep_ms + 0.5; // +0.5 for round to start with blocks fully in BLOCK_LENGTH
    int idxEnd = blockEnd_ms / fftTimestep_ms;           // rounds down to end with block fully in BLOCK_LENGTH

    *start = idxStart;
    *frmCount = idxEnd - idxStart;
}

uint8_t *processAllFrames(bool **frames)
{
    bool resultBits[NR_OF_MESSAGES][NR_OF_FREQUENCIES];
    uint8_t *resultBytes = calloc(1, sizeof(uint8_t) * MAX_MESSAGE_SIZE);

    int idxStart, frmCount;
#if NR_OF_FREQUENCIES == 4
    for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
    {
        findIdxForTimeblock(i * 2, &idxStart, &frmCount);
        findBestFit(frames + idxStart, frmCount, resultBits[i * 2]);

        findIdxForTimeblock(i * 2 + 1, &idxStart, &frmCount);
        findBestFit(frames + idxStart, frmCount, resultBits[i * 2 + 1]);

        resultBytes[MAX_MESSAGE_SIZE - i - 1] = bitArrToVal(resultBits[i * 2], resultBits[i * 2 + 1]);
    }
#else
    for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
    {
        findIdxForTimeblock(i, &idxStart, &frmCount);
        findBestFit(frames + idxStart, frmCount, resultBits[i]);
        resultBytes[i] = bitArrToVal(resultBits[i]);
    }
#endif

    for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
    {
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c %c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
    (byte & 0x80 ? '1' : '0'),     \
        (byte & 0x40 ? '1' : '0'), \
        (byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

        ESP_LOGV(logPrefix, "0x%02X : " BYTE_TO_BINARY_PATTERN "\n", resultBytes[i], BYTE_TO_BINARY(resultBytes[i]));
    }
    return resultBytes;
}

uint8_t *soniTalkLoop(double timestep)
{
    uint8_t *result = NULL;
    bool freqTable[NR_OF_FREQUENCIES];
    soniTalkTime += timestep;
    // Translate current fft to potential data-bits
    getFreqActivityTable(freqTable);

    if (!isReceiving)
    {
        // Wait for Start sequence
        if (memcmp(st_start, freqTable, sizeof(st_start)) == 0)
        {
            if (startFrameNo == 0)
            {
                // It is the first start-frame, start counter for one byte
                soniTalkTime = 0;
                lastRTOStime = getTimeRTOS();
            }
            startFrameNo++;

            if (startFrameNo >= ONE_DATUM_BUFF_LEN * START_FRAME_ACCEPT_RATIO)
            {
                ESP_LOGV(logPrefix, "Starting to listen after %d start-frames", startFrameNo);
                // We received enough start-frames to consider it not being random noise
                isReceiving = true;
                startFrameNo = 0; // Reset for next time already
                curFrameNo = 0;
            }
        }
        else if (startFrameNo > 0 && soniTalkTime > BLOCK_LENGTH)
        {
            if (startFrameNo > 4)
                ESP_LOGW(logPrefix, "Start frame recognition timed out after %.02f with %d found frames, should be at least %d", soniTalkTime, startFrameNo, ONE_DATUM_BUFF_LEN - 1);
            soniTalkTime = 0;
            startFrameNo = 0;
        }
    }
    else
    {
        // Skip possible start frames which might still come
        if (curFrameNo == 0 && soniTalkTime < BLOCK_LENGTH)
        {
            if (memcmp(st_start, freqTable, sizeof(st_start)) == 0)
            {
                return result;
            }
        }

        // We are in receiving state, store result
        memcpy(recv_dataframes[curFrameNo], freqTable, sizeof(freqTable));
        curFrameNo++;

        // Process and stop when we reached max count of bytes
        if (curFrameNo >= FULL_DATA_BUFF_LEN)
        {
            ESP_LOGD(logPrefix, "Time Passed SoniTalk: %.02f, Time Passed FreeRTOS: %.02f, Catched frames: %d", soniTalkTime, getTimeRTOS() - lastRTOStime, curFrameNo);
            result = processAllFrames(recv_dataframes);
            isReceiving = false;
            curFrameNo = 0;
        }
    }
    return result;
}

bool init_soniTalk_receiver()
{
    // Buffer for the 16bit the Microphone
    if (sampleBuffer == NULL)
    {
        sampleBuffer = (int16_t *)malloc(sizeof(int16_t) * N_SAMPLES);
        if (sampleBuffer == NULL)
        {
            ESP_LOGE(logPrefix, "Failed to allocated %d Bytes for sampleBuffer", sizeof(int16_t) * N_SAMPLES);
            return false;
        }
    }

    // Buffer for Samples in Complex number format
    if (samplesIm_f32 == NULL)
    {
        samplesIm_f32 = (float *)malloc(sizeof(float) * N_SAMPLES * 2);
        if (samplesIm_f32 == NULL)
        {
            ESP_LOGE(logPrefix, "Failed to allocate %d Bytes for samplesIm_f32", sizeof(float) * N_SAMPLES * 2);
            return false;
        }
    }

    // Complex buffer is reused for Real-Number-Decible result
    fftResLog = samplesIm_f32;

#ifdef USE_WINDOW
    if (window == NULL)
    {
        window = (float *)malloc(sizeof(float) * N_SAMPLES);
        if (window == NULL)
        {
            ESP_LOGE(logPrefix, "Failed to allocate %d Bytes for window", sizeof(float) * N_SAMPLES);
            return false;
        }

        dsps_wind_hann_f32(window, N_SAMPLES);
    }
#endif

    // Buffer for the filtered Results (ie just contains bools for in/active frequencies of interest)
    if (recv_dataframes == NULL)
    {
        recv_dataframes = (bool **)malloc(sizeof(bool **) * FULL_DATA_BUFF_LEN);
        if (recv_dataframes == NULL)
        {
            ESP_LOGE(logPrefix, "Failed to allocated %d Bytes for recv_dataframes", sizeof(bool *) * FULL_DATA_BUFF_LEN);
            return false;
        }
        for (int i = 0; i < FULL_DATA_BUFF_LEN; i++)
        {
            recv_dataframes[i] = (bool *)malloc(SIZEOF_FRAME);
            if (recv_dataframes[i] == NULL)
            {
                ESP_LOGE(logPrefix, "Failed to allocated %d Bytes for recv_dataframes[]", SIZEOF_FRAME);
                return false;
            }
        }
    }

    int err = dsps_fft2r_init_fc32(NULL, N_SAMPLES);
    if (err)
    {
        ESP_LOGE(logPrefix, "fft init failed with code %d", err);
        return false;
    }

    // Write bit pattern for SoniTalk start and end messages
    init_start_end_blocks();

    ESP_LOGV(logPrefix, "Initialized receiver");

    return true;
}

void uninit_soniTalk_receiver()
{
    if (sampleBuffer != NULL)
    {
        free(sampleBuffer);
        sampleBuffer = NULL;
    }
    if (samplesIm_f32 != NULL)
    {
        free(samplesIm_f32);
        samplesIm_f32 = NULL;
    }
#ifdef USE_WINDOW
    if (window != NULL)
    {
        free(window);
        window = NULL;
    }
#endif
    if (recv_dataframes != NULL)
    {
        for (int i = 0; i < FULL_DATA_BUFF_LEN; i++)
        {
            free(recv_dataframes[i]);
        }
        free(recv_dataframes);
        recv_dataframes = NULL;
    }

    dsps_fft2r_deinit_fc32();

    ESP_LOGV(logPrefix, "Uninitialized receiver");
}

uint8_t *receive_get_message()
{
    uint8_t *result = NULL;
    while (result == NULL && sonitalkReceive_stop == false)
    {
        readMic_f32_Im(samplesIm_f32, sampleBuffer, sizeof(uint16_t) * N_SAMPLES);

        dsps_fft2r_fc32(samplesIm_f32, N_SAMPLES);
        dsps_bit_rev_fc32(samplesIm_f32, N_SAMPLES);
        dsps_cplx2reC_fc32(samplesIm_f32, N_SAMPLES);

        float magnitude;
        for (int i = 0; i < N_SAMPLES / 2; i++)
        {
            magnitude = ((samplesIm_f32[i * 2 + 0] * samplesIm_f32[i * 2 + 0] + samplesIm_f32[i * 2 + 1] * samplesIm_f32[i * 2 + 1]) / N_SAMPLES);
            fftResLog[i] = 10 * log10f(magnitude);
        }

        result = soniTalkLoop(fftTimestep_ms);

        if (isReceiving)
        {
            static bool ledOn = true;
            ledOn = !ledOn;
            ledSet(ledOn ? 255 : 0, 255, 0);
        }

        vTaskDelay(1);
    }
    return result;
}

uint8_t *sonitalk_receive()
{
    sonitalkReceive_isRunning = true;

    InitI2S(MODE_MIC, SAMPLE_RATE_MIC);
    init_soniTalk_receiver();

    ESP_LOGV(logPrefix, "SoniTalk receiver started");
    uint8_t *message = receive_get_message();
    ESP_LOGV(logPrefix, "SoniTalk receiver finished");

    uninit_soniTalk_receiver();

    sonitalkReceive_isRunning = false;

    return message;
}
