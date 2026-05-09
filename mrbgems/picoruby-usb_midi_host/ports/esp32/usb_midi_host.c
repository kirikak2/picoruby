/*
 * PicoRuby USB-MIDI ESP32 Port Implementation
 *
 * Thread-safe communication between USB Host task (Core 0)
 * and PicoRuby task (Core 1)
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "../../include/usb_midi_host.h"
#include "../../../picoruby-midi/include/midi.h"

static const char *TAG = "USB_MIDI_HOST";

/* TX Queue for MIDI OUT */
#define MIDI_TX_QUEUE_SIZE 32
static QueueHandle_t g_tx_queue = NULL;

/* RX Ring Buffer for MIDI IN */
static usb_midi_host_rx_buffer_t g_rx_buffer;

/* Device state */
static volatile usb_midi_host_status_t g_status = USB_MIDI_HOST_DISCONNECTED;
static usb_midi_host_device_info_t g_device_info;
static SemaphoreHandle_t g_info_mutex = NULL;

/* Ring buffer mask (size must be power of 2) */
#define RX_BUFFER_MASK (USB_MIDI_HOST_RX_BUFFER_SIZE - 1)

/*
 * Initialize USB MIDI subsystem
 */
int USB_MIDI_HOST_init(void)
{
    /* Create TX queue */
    if (g_tx_queue == NULL) {
        g_tx_queue = xQueueCreate(MIDI_TX_QUEUE_SIZE, sizeof(usb_midi_host_tx_event_t));
        if (g_tx_queue == NULL) {
            ESP_LOGE(TAG, "Failed to create TX queue");
            return -1;
        }
    }

    /* Create info mutex */
    if (g_info_mutex == NULL) {
        g_info_mutex = xSemaphoreCreateMutex();
        if (g_info_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create mutex");
            return -1;
        }
    }

    /* Lazy-allocate the 1KB ring buffer in internal RAM. Keeping it
     * inline as a static array enlarged the .bss enough to disturb LCD
     * init on Tab5 under idf.py monitor; see CLAUDE.md. */
    if (g_rx_buffer.data == NULL) {
        g_rx_buffer.data = (volatile uint8_t *)heap_caps_malloc(
            USB_MIDI_HOST_RX_BUFFER_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (g_rx_buffer.data == NULL) {
            ESP_LOGE(TAG, "Failed to allocate RX buffer (%d bytes)",
                     USB_MIDI_HOST_RX_BUFFER_SIZE);
            return -1;
        }
    }

    /* Initialize RX buffer only if not already connected
     * (preserves data if device connected before Ruby calls init) */
    if (g_status != USB_MIDI_HOST_CONNECTED) {
        g_rx_buffer.head = 0;
        g_rx_buffer.tail = 0;
        memset((void *)g_rx_buffer.data, 0, USB_MIDI_HOST_RX_BUFFER_SIZE);

        /* Reset device info and status only if not connected */
        memset(&g_device_info, 0, sizeof(g_device_info));
        g_status = USB_MIDI_HOST_DISCONNECTED;
        ESP_LOGI(TAG, "USB MIDI initialized");
    } else {
        ESP_LOGI(TAG, "USB MIDI already connected, preserving state");
    }
    return 0;
}

/*
 * Deinitialize USB MIDI subsystem
 */
void USB_MIDI_HOST_deinit(void)
{
    if (g_tx_queue != NULL) {
        vQueueDelete(g_tx_queue);
        g_tx_queue = NULL;
    }

    if (g_info_mutex != NULL) {
        vSemaphoreDelete(g_info_mutex);
        g_info_mutex = NULL;
    }

    g_status = USB_MIDI_HOST_DISCONNECTED;
    ESP_LOGI(TAG, "USB MIDI deinitialized");
}

/*
 * Get current connection status
 */
usb_midi_host_status_t USB_MIDI_HOST_get_status(void)
{
    return g_status;
}

/*
 * Get connected device info
 */
bool USB_MIDI_HOST_get_device_info(usb_midi_host_device_info_t *info)
{
    if (g_status != USB_MIDI_HOST_CONNECTED || info == NULL) {
        return false;
    }

    if (g_info_mutex != NULL && xSemaphoreTake(g_info_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(info, &g_device_info, sizeof(usb_midi_host_device_info_t));
        xSemaphoreGive(g_info_mutex);
        return true;
    }

    /* Fallback: copy without mutex protection if mutex doesn't exist */
    if (g_info_mutex == NULL) {
        memcpy(info, &g_device_info, sizeof(usb_midi_host_device_info_t));
        return true;
    }

    return false;
}

/*
 * Send a single USB-MIDI packet
 */
static uint32_t g_send_fail_count = 0;  // Track consecutive send failures

int USB_MIDI_HOST_send_packet(uint8_t cable, uint8_t cin,
                         uint8_t midi1, uint8_t midi2, uint8_t midi3)
{
    if (g_status != USB_MIDI_HOST_CONNECTED || g_tx_queue == NULL) {
        g_send_fail_count++;
        if (g_send_fail_count == 1 || (g_send_fail_count % 100) == 0) {
            ESP_LOGW(TAG, "send_packet failed: status=%d, queue=%p (fail_count=%lu)",
                     g_status, g_tx_queue, g_send_fail_count);
        }
        return -1;
    }
    g_send_fail_count = 0;  // Reset on success

    usb_midi_host_tx_event_t event;
    event.packet[0] = (cable << 4) | (cin & 0x0F);
    event.packet[1] = midi1;
    event.packet[2] = midi2;
    event.packet[3] = midi3;

    if (xQueueSend(g_tx_queue, &event, pdMS_TO_TICKS(10)) != pdTRUE) {
        ESP_LOGW(TAG, "TX queue full");
        return -1;
    }

    return 0;
}

/*
 * Send raw data (multiple 4-byte packets)
 */
int USB_MIDI_HOST_send_raw(const uint8_t *data, size_t len)
{
    if (g_status != USB_MIDI_HOST_CONNECTED || g_tx_queue == NULL) {
        return -1;
    }

    /* Must be multiple of 4 bytes */
    if (len % 4 != 0) {
        return -1;
    }

    int sent = 0;
    for (size_t i = 0; i < len; i += 4) {
        usb_midi_host_tx_event_t event;
        memcpy(event.packet, &data[i], 4);

        if (xQueueSend(g_tx_queue, &event, pdMS_TO_TICKS(10)) != pdTRUE) {
            ESP_LOGW(TAG, "TX queue full at packet %d", sent);
            break;
        }
        sent++;
    }

    return sent;
}

/*
 * Get number of bytes available in RX buffer
 */
int USB_MIDI_HOST_bytes_available(void)
{
    uint32_t head = g_rx_buffer.head;
    uint32_t tail = g_rx_buffer.tail;
    return (int)((head - tail) & RX_BUFFER_MASK);
}

/*
 * Read packets from RX buffer
 */
int USB_MIDI_HOST_read_packet(uint8_t *out_buffer, size_t max_len)
{
    if (out_buffer == NULL || max_len < 4) {
        return 0;
    }

    int available = USB_MIDI_HOST_bytes_available();
    if (available < 4) {
        return 0;
    }

    /* Read in 4-byte chunks */
    size_t to_read = (available < (int)max_len) ? available : max_len;
    to_read = (to_read / 4) * 4;  /* Align to 4 bytes */

    uint32_t tail = g_rx_buffer.tail;
    for (size_t i = 0; i < to_read; i++) {
        out_buffer[i] = g_rx_buffer.data[(tail + i) & RX_BUFFER_MASK];
    }

    /* Memory barrier before updating tail */
    __sync_synchronize();
    g_rx_buffer.tail = (tail + to_read) & RX_BUFFER_MASK;

    return (int)to_read;
}

/*
 * Bridge functions (called from USB Host task on Core 0)
 */

/*
 * Push received data to RX buffer
 * Called from midi_in_transfer_callback
 * Uses overwriting ring buffer - old data is discarded when full
 */
void USB_MIDI_HOST_push_rx_data(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return;
    }

    uint32_t head = g_rx_buffer.head;
    uint32_t tail = g_rx_buffer.tail;

    /* Write data to buffer */
    for (size_t i = 0; i < len; i++) {
        g_rx_buffer.data[(head + i) & RX_BUFFER_MASK] = data[i];
    }

    uint32_t new_head = (head + len) & RX_BUFFER_MASK;

    /* Check if we've overwritten unread data */
    uint32_t used_before = (head - tail) & RX_BUFFER_MASK;
    uint32_t used_after = (new_head - tail) & RX_BUFFER_MASK;

    /* If buffer would overflow, advance tail to discard old data */
    if (used_after >= USB_MIDI_HOST_RX_BUFFER_SIZE - 1 || used_after < used_before) {
        /* Advance tail to make room, keeping some margin */
        uint32_t new_tail = (new_head + 4) & RX_BUFFER_MASK;  /* Keep 4 bytes margin */
        ESP_LOGD(TAG, "RX buffer overwrite, discarding old data");
        __sync_synchronize();
        g_rx_buffer.tail = new_tail;
    }

    /* Memory barrier before updating head */
    __sync_synchronize();
    g_rx_buffer.head = new_head;
}

/*
 * Pop TX packet from queue
 * Called from USB Host task main loop
 */
bool USB_MIDI_HOST_pop_tx_packet(uint8_t *out_packet)
{
    if (g_tx_queue == NULL || out_packet == NULL) {
        return false;
    }

    usb_midi_host_tx_event_t event;
    if (xQueueReceive(g_tx_queue, &event, 0) == pdTRUE) {
        memcpy(out_packet, event.packet, 4);
        return true;
    }

    return false;
}

/*
 * Get number of packets waiting in TX queue
 */
int USB_MIDI_HOST_tx_queue_depth(void)
{
    if (g_tx_queue == NULL) {
        return -1;
    }
    return (int)uxQueueMessagesWaiting(g_tx_queue);
}

/*
 * Notify device connection
 */
void USB_MIDI_HOST_notify_connected(const usb_midi_host_device_info_t *info)
{
    if (info == NULL) {
        return;
    }

    /* Ensure mutex exists (may be called before USB_MIDI_HOST_init) */
    if (g_info_mutex == NULL) {
        g_info_mutex = xSemaphoreCreateMutex();
        if (g_info_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create mutex in notify_connected");
            /* Still set status even without mutex protection */
            memcpy(&g_device_info, info, sizeof(usb_midi_host_device_info_t));
            g_status = USB_MIDI_HOST_CONNECTED;
            ESP_LOGI(TAG, "MIDI device connected (no mutex): %s", g_device_info.product);
            return;
        }
    }

    if (xSemaphoreTake(g_info_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(&g_device_info, info, sizeof(usb_midi_host_device_info_t));
        g_status = USB_MIDI_HOST_CONNECTED;
        xSemaphoreGive(g_info_mutex);
        ESP_LOGI(TAG, "MIDI device connected: %s", g_device_info.product);
    } else {
        /* Mutex take failed, but still try to set status */
        ESP_LOGW(TAG, "Mutex take failed, setting status without lock");
        g_status = USB_MIDI_HOST_CONNECTED;
    }

    /* Auto-restart MIDI input task if it was previously started */
    if (MIDI_Input_was_started() && !MIDI_Input_is_running()) {
        ESP_LOGI(TAG, "Auto-restarting MIDI input task on reconnect");
        int ret = MIDI_Input_start();
        if (ret != 0) {
            ESP_LOGW(TAG, "Failed to auto-restart MIDI input task");
        }
    }
}

/*
 * Notify device disconnection
 */
void USB_MIDI_HOST_notify_disconnected(void)
{
    /* Set status first to stop new sends */
    g_status = USB_MIDI_HOST_DISCONNECTED;

    /* Stop MIDI clock timer if running */
    if (MIDI_Clock_is_running()) {
        ESP_LOGI(TAG, "Stopping MIDI clock timer due to disconnect");
        MIDI_Clock_stop();
    }

    /* Stop MIDI input task if running */
    if (MIDI_Input_is_running()) {
        ESP_LOGI(TAG, "Stopping MIDI input task due to disconnect");
        MIDI_Input_stop();
    }

    /* Clear RX buffer */
    g_rx_buffer.head = 0;
    g_rx_buffer.tail = 0;

    /* Clear TX queue */
    if (g_tx_queue != NULL) {
        xQueueReset(g_tx_queue);
    }

    /* Clear device info */
    if (g_info_mutex != NULL && xSemaphoreTake(g_info_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memset(&g_device_info, 0, sizeof(usb_midi_host_device_info_t));
        xSemaphoreGive(g_info_mutex);
    }

    ESP_LOGI(TAG, "MIDI device disconnected");
}
