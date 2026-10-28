#include "esp_http_server.h"
#include "esp_log.h"
#include <forward_list>
#include <memory>

using namespace std;

class WS {
private:
    forward_list<const httpd_uri_t*> uris;

    struct async_resp_arg {
        httpd_handle_t hd;
        int fd;
        const char* data;
    };
    
public:
    static const char* WS_TAG;
    
    struct ws_frame
    {
        std::unique_ptr<httpd_ws_frame_t> frame;
        std::unique_ptr<uint8_t[]> buffer;
    };

    httpd_handle_t start();

    esp_err_t stop(httpd_handle_t server);

    static ws_frame getPayload(httpd_req_t* req);

    static esp_err_t sendData(httpd_req_t* req, const char* data);

    void addUri(const httpd_uri_t& uri);
};