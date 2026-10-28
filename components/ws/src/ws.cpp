#include "ws.h"

const char* WS::WS_TAG = "WS";

httpd_handle_t WS::start() {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(WS_TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Registering the ws handler
        ESP_LOGI(WS_TAG, "Registering URI handlers");

        for (const httpd_uri_t* uri : uris) {
            httpd_register_uri_handler(server, uri);
        }

        return server;
    }

    ESP_LOGI(WS_TAG, "Error starting server!");

    return NULL;
}

esp_err_t WS::stop(httpd_handle_t server) {
    return httpd_stop(server);
}

WS::ws_frame WS::getPayload(httpd_req_t* req) {
    WS::ws_frame ws_pkt{};

    // httpd_ws_frame_t* ws_pkt = new httpd_ws_frame_t{};
    // std::unique_ptr<httpd_ws_frame_t> ws_pkt = std::make_unique<httpd_ws_frame_t>();
    ws_pkt.frame = std::make_unique<httpd_ws_frame_t>();

    // uint8_t* buf = NULL;
    // std::unique_ptr<uint8_t[]> buf = nullptr;
    ws_pkt.buffer = nullptr;

    memset(ws_pkt.frame.get(), 0, sizeof(httpd_ws_frame_t));
    ws_pkt.frame->type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, ws_pkt.frame.get(), 0);
    if (ret != ESP_OK) {
        ESP_LOGE(WS_TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        ESP_ERROR_CHECK(ret);
    }
    ESP_LOGI(WS_TAG, "frame len is %d", ws_pkt.frame->len);

    if (ws_pkt.frame->len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        // buf = new uint8_t[ws_pkt->len + 1];
        ws_pkt.buffer = std::make_unique<uint8_t[]>(ws_pkt.frame->len + 1);
        if (!ws_pkt.buffer) {
            ESP_LOGE(WS_TAG, "Failed to calloc memory for buf");
            return ws_pkt;
        }
        ws_pkt.frame->payload = ws_pkt.buffer.get();

        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, ws_pkt.frame.get(), ws_pkt.frame->len);
        if (ret != ESP_OK) {
            ESP_LOGE(WS_TAG, "httpd_ws_recv_frame failed with %d", ret);
            // delete[] buf;
            return ws_pkt;
        }
        ESP_LOGI(WS_TAG, "Got packet with message: %s", ws_pkt.frame->payload);
    }
    ESP_LOGI(WS_TAG, "Packet type: %d", ws_pkt.frame->type);

    // delete[] buf;

    return ws_pkt;
}

esp_err_t WS::sendData(httpd_req_t* req, const char* data) {
    async_resp_arg* resp_arg = new async_resp_arg{};
    if (resp_arg == NULL) {
        return ESP_ERR_NO_MEM;
    }

    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    resp_arg->data = data;

    esp_err_t ret = httpd_queue_work(req->handle, [](void* arg)->void {
        async_resp_arg* resp_arg = static_cast<async_resp_arg*>(arg);

        httpd_handle_t hd = resp_arg->hd;
        int fd = resp_arg->fd;

        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

        ws_pkt.payload = (uint8_t*)resp_arg->data;
        ws_pkt.len = strlen(resp_arg->data);
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;

        httpd_ws_send_frame_async(hd, fd, &ws_pkt);
        delete resp_arg;
        }, resp_arg);

    if (ret != ESP_OK) {
        delete resp_arg;
    }

    return ret;
}

void WS::addUri(const httpd_uri_t& uri) {
    uris.push_front(&uri);
}
