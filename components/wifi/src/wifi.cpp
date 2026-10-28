#include "wifi.h"

uint8_t Wifi::MAXIMUM_RETRY{ 1 };
EventGroupHandle_t Wifi::WIFI_EVENT_GROUP;

Wifi::Wifi(char* ssid, char* password, uint8_t maximum_rety) {
    WIFI_SSID = ssid;
    WIFI_PASSWORD = password;
    MAXIMUM_RETRY = maximum_rety;
}

void Wifi::initStationMode() {
    WIFI_EVENT_GROUP = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &Wifi::eventHandlerCb,
        NULL,
        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &Wifi::eventHandlerCb,
        NULL,
        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta {
            // Scan all channels before choosing the AP
            .scan_method = WIFI_ALL_CHANNEL_SCAN,

            // Don't lock to a specific BSSID
            .bssid_set = false,

            // Start scanning from any channel
            .channel = 0,

            // If multiple APs have the same SSID, choose the strongest
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
        },
    };
    memcpy(wifi_config.sta.ssid, WIFI_SSID, strlen(WIFI_SSID));
    memcpy(wifi_config.sta.password, WIFI_PASSWORD, strlen(WIFI_PASSWORD));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(WIFI_EVENT_GROUP,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFI_TAG, "connected to ap SSID:%s password:%s",
            WIFI_SSID, WIFI_PASSWORD);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
            WIFI_SSID, WIFI_PASSWORD);
    } else {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
    }
}

void Wifi::eventHandlerCb(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    uint8_t RETRIED_COUNT{ 0 };
    const char* tag{ "WIFI event handler" };

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (RETRIED_COUNT < MAXIMUM_RETRY) {
            esp_wifi_connect();
            RETRIED_COUNT++;
            ESP_LOGI(tag, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(WIFI_EVENT_GROUP, WIFI_FAIL_BIT);
        }
        ESP_LOGI(tag, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(tag, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        RETRIED_COUNT = 0;
        xEventGroupSetBits(WIFI_EVENT_GROUP, WIFI_CONNECTED_BIT);
    }
}

void Wifi::startInStationMode() {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_log_level_set(WIFI_TAG, ESP_LOG_DEBUG);

    ESP_LOGI(WIFI_TAG, "ESP_WIFI_MODE_STA");
    initStationMode();
}

void Wifi::setWifiSSID(char* value) {
    WIFI_SSID = value;
}

void Wifi::setWifiPassword(char* value) {
    WIFI_PASSWORD = value;
}
