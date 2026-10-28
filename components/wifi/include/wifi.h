#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

using namespace std;

class Wifi {
private:
    char* WIFI_SSID;
    char* WIFI_PASSWORD;
    static uint8_t MAXIMUM_RETRY;

    /* FreeRTOS event group to signal when we are connected*/
    static EventGroupHandle_t WIFI_EVENT_GROUP;

    /* The event group allows multiple bits for each event, but we only care about two events:
     * - we are connected to the AP with an IP
     * - we failed to connect after the maximum amount of retries */
    constexpr static uint32_t WIFI_CONNECTED_BIT{ BIT0 };
    constexpr static uint32_t WIFI_FAIL_BIT{ BIT1 };

    const char* WIFI_TAG{ "WIFI" };

    void initStationMode();
    static void eventHandlerCb(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
public:
    Wifi(char* ssid = nullptr, char* password = nullptr, uint8_t maximum_rety = 3);

    void startInStationMode();

    void setWifiSSID(char* value);

    void setWifiPassword(char* value);
};
