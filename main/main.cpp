#include <iostream>
#include "wifi.h"
#include "ws.h"
#include "cJSON.h"
#include <memory>

extern "C" void app_main(void) {
    Wifi wifi("Lamassu", "mohaMAD89031234@Mm!");
    wifi.startInStationMode();

    WS ws;
    ws.addUri(httpd_uri_t{
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = [](httpd_req_t* req)->esp_err_t {
            WS::ws_frame pkt = WS::getPayload(req);

            auto json = std::make_unique<cJSON>(*cJSON_Parse(reinterpret_cast<char*>(pkt.frame->payload)));

            if (!json) {
                ESP_LOGE("cJSON", "Invalid JSON");
                return ESP_ERR_NOT_FOUND;
            }

            auto command = std::make_unique<cJSON>(*cJSON_GetObjectItemCaseSensitive(json.get(),"command"));

            if (!cJSON_IsString(command.get())) {
                ESP_LOGE("cJSON", "Invalid Command sent.");
                return ESP_ERR_NOT_ALLOWED;
            }

            ESP_LOGI("cJSON","Command Receieved: %s",command->valuestring);

            // dispatch here

            return ESP_OK;
        },
        .user_ctx = NULL,
        .is_websocket = true
        });
    ws.start();
}
