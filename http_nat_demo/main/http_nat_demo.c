#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_https_server.h"
  // Use HTTPS server

#define WIFI_SSID "ESP32_AP"
#define WIFI_PASS "12345678"

static const char *TAG = "HTTPS_NAT";

//Self-signed certificate and key (PEM format)
//here self signed certificates are used by openssl 
extern const uint8_t server_cert_pem[] asm("_binary_server_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_server_cert_pem_end");
extern const uint8_t server_key_pem[] asm("_binary_server_key_pem_start");
extern const uint8_t server_key_pem_end[] asm("_binary_server_key_pem_end");
//only visible to this file and constant one(immutable)
 static const char *resp_str = "<h1>Hello from ESP32 HTTPS Server!</h1>";

// HTTP handler
esp_err_t root_get_handler(httpd_req_t *req) {    
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

httpd_uri_t root_uri = {
    .uri = "/get",//GET(get the reponse)
    .method = HTTP_GET,
    .handler = root_get_handler,
};

// Start HTTPS server
void start_https_server(void) {
    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();//here internal mbedls is done 
conf.servercert = server_cert_pem;
conf.servercert_len = (server_cert_pem_end - server_cert_pem);
conf.prvtkey_pem = server_key_pem;
conf.prvtkey_len = (server_key_pem_end - server_key_pem);

    httpd_handle_t server = NULL;
    if (httpd_ssl_start(&server, &conf) == ESP_OK) {
        httpd_register_uri_handler(server, &root_uri);
        ESP_LOGI(TAG, "HTTPS server started");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTPS server");
    }
}


// Wi-Fi AP initialization(by using softap)
//Here i have implemented NVS as for safe state fallbacks without any error getting caused by any function
esp_err_t wifi_init_softap(void) {
    esp_err_t ret=nvs_flash_init();
     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND|| ret ==  ESP_ERR_NOT_FOUND ) {
        ret=nvs_flash_erase();
        if(ret!=ESP_OK)
         ESP_LOGE(TAG, "Failed to erasef, error code: %s", esp_err_to_name(ret));
        ret = nvs_flash_init(); // Reinitializing the  NVS
    };
    if(ret!=ESP_OK){
     ESP_LOGE(TAG, "Failed to initialize NVS, error code: %s", esp_err_to_name(ret));
        return ret;
    } 
    ret=esp_netif_init();
    if(ret!=ESP_OK){
     ESP_LOGE(TAG, "Failed to initialize netif, error code: %s", esp_err_to_name(ret));
        return ret;
    }
    ret=esp_event_loop_create_default();
    if(ret!=ESP_OK){
     ESP_LOGE(TAG, "Failed to create the event loop by default, error code: %s", esp_err_to_name(ret));
        return ret; 
    }
    esp_netif_create_default_wifi_ap();//here it will call the default Access point

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();//default configration
    ret=esp_wifi_init(&cfg);
    if(ret!=ESP_OK){
       ESP_LOGE(TAG, "Wi-Fi initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK//first WPA2 if not then falls back to WPA
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;//if no password then no auth
    }

    esp_err_t err1=esp_wifi_set_mode(WIFI_MODE_AP);
    if(err1!=ESP_OK)
    printf("Error %d at %s:%d\n", err1, __FILE__, __LINE__);
    ret=esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if(ret!=ESP_OK){
      ESP_LOGE(TAG, "Wi-Fi set configuration  failed: %s", esp_err_to_name(ret));
      return ret;
    }
   //ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ret = esp_wifi_start(); 
    if (ret != ESP_OK) {
     ESP_LOGE(TAG, "Wi-Fi start failed: %s", esp_err_to_name(ret));
      return ret;
    //abort();
}

    ESP_LOGI(TAG, "Wi-Fi SoftAP initialized. SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
return ESP_OK;
}

//app_main
void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_softap();
    start_https_server();
}

