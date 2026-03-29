#include <stdio.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "protocol_examples_common.h"
#include "abl_link.h"

static const char *TAG = "VOLCASYNC_LINK";

static void log_link_state(abl_link link)
{
    abl_link_session_state session = abl_link_create_session_state();
    if (!session) {
        ESP_LOGE(TAG, "Failed to create Ableton Link session state");
        return;
    }

    abl_link_capture_audio_session_state(link, session);

    uint64_t now_us = esp_timer_get_time();
    double tempo = abl_link_tempo(session);
    double beat = abl_link_beat_at_time(session, now_us, 4.0);
    double phase = abl_link_phase_at_time(session, now_us, 4.0);
    uint64_t peers = abl_link_num_peers(link);

    ESP_LOGI(TAG,
             "peers=%" PRIu64 " tempo=%.3f beat=%.3f phase=%.3f now_us=%" PRIu64,
             peers, tempo, beat, phase, now_us);

    abl_link_destroy_session_state(session);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "Connecting to Wi-Fi...");
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Wi-Fi connected");

    abl_link link = abl_link_create(120.0);
    if (!link) {
        ESP_LOGE(TAG, "Failed to create Ableton Link instance");
        return;
    }

    abl_link_enable(link, true);
    ESP_LOGI(TAG, "Ableton Link enabled");

    while (1) {
        log_link_state(link);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
