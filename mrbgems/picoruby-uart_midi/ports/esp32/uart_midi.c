/*
 * PicoRuby UART/Serial MIDI - ESP32 port
 *
 * Generic UART MIDI implementation. Originally part of
 * picoruby-sam2695; lifted here as Phase 5b of the picoruby-midi
 * standardization plan so any UART-attached MIDI device (5-pin DIN,
 * SAM2695, future synths) shares one driver.
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "../../include/uart_midi.h"

static const char *TAG       = "UART_MIDI";
static const char *INPUT_TAG = "UART_MIDI_INPUT";

/* UART selection. Override at build time with -DUART_MIDI_UART_NUM=... */
#ifndef UART_MIDI_UART_NUM
#define UART_MIDI_UART_NUM    UART_NUM_2
#endif

#define UART_TX_BUF_SIZE      0    /* TX uses synchronous writes */
#define UART_RX_BUF_SIZE      256

/* Input task pinning */
#define INPUT_TASK_STACK_SIZE 4096
#define INPUT_TASK_PRIORITY   1
#define INPUT_TASK_CORE       0

/* Driver state */
static volatile uart_midi_status_t g_status     = UART_MIDI_NOT_INITIALIZED;
static SemaphoreHandle_t           g_uart_mutex = NULL;

/* Input task state */
static TaskHandle_t      g_input_task        = NULL;
static volatile bool     g_input_running     = false;
static volatile bool     g_input_was_started = false;
static QueueHandle_t     g_uart_queue        = NULL;

static uart_midi_device_info_t g_device_info = {
    .baud_rate = UART_MIDI_DEFAULT_BAUD_RATE,
    .uart_num  = UART_MIDI_UART_NUM,
    .tx_pin    = -1,
    .rx_pin    = -1
};

/* Number of MIDI bytes a USB-MIDI CIN actually carries on the wire. */
static int wire_bytes_for_cin(uint8_t cin)
{
    switch (cin) {
        case UART_MIDI_CIN_NOTE_OFF:
        case UART_MIDI_CIN_NOTE_ON:
        case UART_MIDI_CIN_POLY_KEY:
        case UART_MIDI_CIN_CONTROL_CHANGE:
        case UART_MIDI_CIN_PITCH_BEND:
        case UART_MIDI_CIN_SYSCOMMON_3:
        case UART_MIDI_CIN_SYSEX_END_3:
            return 3;
        case UART_MIDI_CIN_PROGRAM_CHANGE:
        case UART_MIDI_CIN_CHANNEL_PRESSURE:
        case UART_MIDI_CIN_SYSCOMMON_2:
        case UART_MIDI_CIN_SYSEX_END_2:
            return 2;
        case UART_MIDI_CIN_SINGLE_BYTE:
        case UART_MIDI_CIN_SYSCOMMON_1:
            return 1;
        case UART_MIDI_CIN_SYSEX_START:
            return 3;  /* SysEx start uses all 3 data bytes */
        default:
            return 0;
    }
}

int UART_MIDI_init(int tx_pin, int rx_pin, uint32_t baud)
{
    if (g_status == UART_MIDI_READY) {
        ESP_LOGD(TAG, "Already initialized");
        return 0;
    }

    if (tx_pin < 0) {
        ESP_LOGE(TAG, "Invalid TX pin: %d", tx_pin);
        g_status = UART_MIDI_ERROR;
        return -1;
    }

    if (baud == 0) baud = UART_MIDI_DEFAULT_BAUD_RATE;

    g_device_info.tx_pin    = tx_pin;
    g_device_info.rx_pin    = rx_pin;
    g_device_info.baud_rate = baud;

    if (g_uart_mutex == NULL) {
        g_uart_mutex = xSemaphoreCreateMutex();
        if (g_uart_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create mutex");
            g_status = UART_MIDI_ERROR;
            return -1;
        }
    }

    uart_config_t uart_config = {
        .baud_rate = (int)baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_driver_install(UART_MIDI_UART_NUM, UART_RX_BUF_SIZE,
                                        UART_TX_BUF_SIZE, 20, &g_uart_queue, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        g_status = UART_MIDI_ERROR;
        return -1;
    }

    ret = uart_param_config(UART_MIDI_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(ret));
        g_status = UART_MIDI_ERROR;
        return -1;
    }

    int actual_rx = (rx_pin >= 0) ? rx_pin : UART_PIN_NO_CHANGE;
    ret = uart_set_pin(UART_MIDI_UART_NUM, tx_pin, actual_rx,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        g_status = UART_MIDI_ERROR;
        return -1;
    }

    g_status = UART_MIDI_READY;
    ESP_LOGI(TAG, "UART_MIDI initialized (UART%d, TX=%d, RX=%d, %lu baud)",
             UART_MIDI_UART_NUM, tx_pin, rx_pin, (unsigned long)baud);
    return 0;
}

void UART_MIDI_deinit(void)
{
    if (g_status == UART_MIDI_NOT_INITIALIZED) return;

    uart_driver_delete(UART_MIDI_UART_NUM);

    if (g_uart_mutex != NULL) {
        vSemaphoreDelete(g_uart_mutex);
        g_uart_mutex = NULL;
    }

    g_status = UART_MIDI_NOT_INITIALIZED;
    ESP_LOGI(TAG, "UART_MIDI deinitialized");
}

uart_midi_status_t UART_MIDI_get_status(void)
{
    return g_status;
}

bool UART_MIDI_get_device_info(uart_midi_device_info_t *info)
{
    if (g_status != UART_MIDI_READY || info == NULL) return false;
    memcpy(info, &g_device_info, sizeof(*info));
    return true;
}

int UART_MIDI_send_packet(uint8_t cable, uint8_t cin,
                          uint8_t midi1, uint8_t midi2, uint8_t midi3)
{
    (void)cable;  /* UART is a single MIDI cable */

    if (g_status != UART_MIDI_READY) {
        ESP_LOGW(TAG, "UART_MIDI not ready");
        return -1;
    }

    int num_bytes = wire_bytes_for_cin(cin);
    if (num_bytes == 0) {
        ESP_LOGD(TAG, "Unknown CIN: 0x%02X", cin);
        return -1;
    }

    uint8_t midi_data[3] = { midi1, midi2, midi3 };

    if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire UART mutex");
        return -1;
    }

    int written = uart_write_bytes(UART_MIDI_UART_NUM, midi_data, num_bytes);
    xSemaphoreGive(g_uart_mutex);

    if (written != num_bytes) {
        ESP_LOGW(TAG, "UART write failed: expected %d, wrote %d",
                 num_bytes, written);
        return -1;
    }

    ESP_LOGD(TAG, "Sent MIDI: CIN=0x%02X, data=[0x%02X, 0x%02X, 0x%02X], bytes=%d",
             cin, midi1, midi2, midi3, num_bytes);
    return 0;
}

int UART_MIDI_send_raw(const uint8_t *data, size_t len)
{
    if (g_status != UART_MIDI_READY) return -1;
    if (data == NULL || len == 0) return 0;

    if (xSemaphoreTake(g_uart_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire UART mutex");
        return -1;
    }

    int written = uart_write_bytes(UART_MIDI_UART_NUM, data, len);
    xSemaphoreGive(g_uart_mutex);
    return written < 0 ? -1 : written;
}

/* ----------------------------------------------------------------------------
 * Input task
 * -------------------------------------------------------------------------- */

static void uart_midi_input_task(void *arg)
{
    (void)arg;
    uart_event_t event;

    ESP_LOGI(INPUT_TAG, "Input task started on Core %d", xPortGetCoreID());

    while (g_input_running) {
        if (xQueueReceive(g_uart_queue, &event, pdMS_TO_TICKS(100))) {
            switch (event.type) {
                case UART_DATA:
                    /* Bytes will be read by the consumer via
                     * UART_MIDI_read_bytes(). The event itself just
                     * tells us something arrived. */
                    ESP_LOGD(INPUT_TAG, "UART data event: %d bytes", event.size);
                    break;
                case UART_FIFO_OVF:
                    ESP_LOGW(INPUT_TAG, "UART FIFO overflow");
                    uart_flush_input(UART_MIDI_UART_NUM);
                    xQueueReset(g_uart_queue);
                    break;
                case UART_BUFFER_FULL:
                    ESP_LOGW(INPUT_TAG, "UART buffer full");
                    uart_flush_input(UART_MIDI_UART_NUM);
                    xQueueReset(g_uart_queue);
                    break;
                default:
                    break;
            }
        }
    }

    g_input_running = false;
    ESP_LOGI(INPUT_TAG, "Input task stopped");
    vTaskDelete(NULL);
}

int UART_MIDI_Input_init(void)
{
    if (g_status != UART_MIDI_READY) {
        ESP_LOGE(INPUT_TAG, "UART_MIDI not initialized");
        return -1;
    }
    if (g_device_info.rx_pin < 0) {
        ESP_LOGE(INPUT_TAG, "RX pin not configured");
        return -1;
    }
    ESP_LOGI(INPUT_TAG, "Input initialized");
    return 0;
}

void UART_MIDI_Input_deinit(void)
{
    UART_MIDI_Input_stop();
}

int UART_MIDI_Input_start(void)
{
    if (UART_MIDI_Input_init() != 0) return -1;
    if (g_input_running) return 0;

    g_input_running = true;
    BaseType_t ret = xTaskCreatePinnedToCore(
        uart_midi_input_task,
        "uart_midi_input",
        INPUT_TASK_STACK_SIZE,
        NULL,
        INPUT_TASK_PRIORITY,
        &g_input_task,
        INPUT_TASK_CORE
    );

    if (ret != pdTRUE) {
        ESP_LOGE(INPUT_TAG, "Failed to create input task");
        g_input_running = false;
        return -1;
    }

    g_input_was_started = true;
    ESP_LOGI(INPUT_TAG, "Input task started (Core %d, Priority %d)",
             INPUT_TASK_CORE, INPUT_TASK_PRIORITY);
    return 0;
}

void UART_MIDI_Input_stop(void)
{
    if (!g_input_running) return;
    g_input_running = false;
    vTaskDelay(pdMS_TO_TICKS(150));
    g_input_task = NULL;
    ESP_LOGI(INPUT_TAG, "Input task stopped");
}

bool UART_MIDI_Input_is_running(void)
{
    return g_input_running;
}

bool UART_MIDI_Input_was_started(void)
{
    return g_input_was_started;
}

int UART_MIDI_bytes_available(void)
{
    if (g_status != UART_MIDI_READY) return 0;
    size_t length = 0;
    uart_get_buffered_data_len(UART_MIDI_UART_NUM, &length);
    return (int)length;
}

int UART_MIDI_read_bytes(uint8_t *out_buffer, size_t max_len)
{
    if (g_status != UART_MIDI_READY || out_buffer == NULL || max_len == 0) {
        return 0;
    }
    int read = uart_read_bytes(UART_MIDI_UART_NUM, out_buffer, max_len, 0);
    return read > 0 ? read : 0;
}
