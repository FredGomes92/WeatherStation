#include <stdio.h>             //for basic printf commands
#include <string.h>            //for handling strings
#include "freertos/FreeRTOS.h" //for delay,mutexs,semphrs rtos operations
#include "esp_system.h"        //esp_init funtions esp_err_t
#include "esp_wifi.h"          //esp_wifi_init functions and wifi operations
#include "esp_log.h"           //for showing logs
#include "esp_event.h"         //for wifi event
#include "nvs_flash.h"         //non volatile storage
#include "lwip/err.h"          //light weight ip packets error handling
#include "lwip/sys.h"          //system applications for light weight ip apps

// MQTT
#include "mqtt_client.h" //provides important functions to connect with MQTT
// #include "protocol_examples_common.h" //important for running different protocols in code
#include "esp_event.h"         //managing events of mqtt
#include "nvs_flash.h"         //storing mqtt and wifi configs and settings
#include "freertos/FreeRTOS.h" //it is important too if you want to run mqtt task independently and provides threads funtionality
#include "freertos/task.h"     //MQTT communication often involves asynchronous operations, and FreeRTOS helps handle those tasks effectively
#include "esp_log.h"           //log out put, do not use printf everywhere


#define IO_USERNAME "FredGomes"
#define IO_KEY "aio_CPhM80yOobEsMaTDXxbcaDRtDisK"


extern const uint8_t emqxsl_ca_crt_start[]   asm("_binary_emqxsl_ca_crt_start");
extern const uint8_t emqxsl_ca_crt_end[]   asm("_binary_emqxsl_ca_crt_end");


const char *ssid = "Proximus-Home-261468";
const char *pass = "yjks2nu57nka3swp";
const char *ca_cert = "";

int retry_num = 0;

char *TAG = "EHH";
char *TAG3 = "UHH";


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{ // here esp_mqtt_event_handle_t is a struct which receieves struct event from mqtt app start funtion
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    if (event->event_id == MQTT_EVENT_CONNECTED)
    {
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, "your topic", 0); // in mqtt we require a topic to subscribe and client is from event client and 0 is quality of service it can be 1 or 2
        ESP_LOGI(TAG3, "sent subscribe successful");

        int msg_id = esp_mqtt_client_publish(client, "sensor/sensor1", "Hello from ESP", 0, 1, 0);
        esp_mqtt_client_publish(client, "sensor/sensor1", "Hello from ESP 2", 0, 1, 0);
        esp_mqtt_client_publish(client, "sensor/sensor1", "Hello from ESP 3", 0, 1, 0);
    }
    else if (event->event_id == MQTT_EVENT_DISCONNECTED)
    {
        ESP_LOGI(TAG3, "MQTT_EVENT_DISCONNECTED"); // if disconnected
    }
    else if (event->event_id == MQTT_EVENT_SUBSCRIBED)
    {
        char *topic = event->topic;
        // printf("Topic = %s",topic);
        ESP_LOGI(TAG3, "MQTT_EVENT_SUBSCRIBED");
    }
    else if (event->event_id == MQTT_EVENT_UNSUBSCRIBED) // when subscribed
    {
        ESP_LOGI(TAG3, "MQTT_EVENT_UNSUBSCRIBED");
    }
    else if (event->event_id == MQTT_EVENT_DATA) // when unsubscribed
    {
        ESP_LOGI(TAG3, "MQTT_EVENT_DATA");
    }
    else if (event->event_id == MQTT_EVENT_ERROR) // when any error
    {
        ESP_LOGI(TAG3, "MQTT_EVENT_ERROR");
    }
    else if (event->event_id == MQTT_EVENT_ERROR)
    {
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        ESP_LOGE(TAG, "Error Code: %d", event->error_handle->error_type);
        ESP_LOGE(TAG, "Error Reason: %s", esp_err_to_name(event->error_handle->error_type));
        // Handle error, add delay, and attempt to reconnect...
    }
}

static void mqtt_initialize(void)
{ /*Depending on your website or cloud there could be more parameters in mqtt_cfg.*/



    const esp_mqtt_client_config_t mqtt_cfg = {
        //.broker.address.uri = "mqtt://io.adafruit.com",
        // .broker.address.uri = "mqtt://broker.emqx.io",
        .broker.address.uri = "mqtts://n71af333.ala.us-east-1.emqxsl.com",
        .credentials.username = "MyWeatherStation",                    // IO_USERNAME,
        .credentials.authentication.password = "Myweatherstation123.", // IO_KEY,
        /*your adafruit password*/                                     // your adafruit io password
        .broker.address.port = 8883, //8883,
        .broker.verification.certificate = (const char *)emqxsl_ca_crt_start,
    };

    //  esp_mqtt_client_config_t mqtt_cfg = {
    //     .broker.address.uri = "mqtt://io.adafruit.com",
    //     .credentials.username = IO_USERNAME,
    //     .credentials.authentication.password = IO_KEY,
    // };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg); // sending struct as a parameter in init client function

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_mqtt_client_start(client);
}
static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START)
    {
        printf("WIFI CONNECTING....\n");
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        printf("WiFi CONNECTED\n");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("WiFi lost connection\n");
        if (retry_num < 5)
        {
            esp_wifi_connect();
            retry_num++;
            printf("Retrying to Connect...\n");
        }
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        printf("Wifi got IP...\n\n");
    }
}

void wifi_connection()
{
    esp_netif_init();                                                                   // network interdace initialization
    esp_event_loop_create_default();                                                    // responsible for handling and dispatching events
    esp_netif_create_default_wifi_sta();                                                // sets up necessary data structs for wifi station interface
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();                    // sets up wifi wifi_init_config struct with default values
    esp_wifi_init(&wifi_initiation);                                                    // wifi initialised with dafault wifi_initiation
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL); // creating event handler register for wifi
    // wifi_config_t wifi_configuration = {
    //     .sta = {
    //         .ssid = {0},
    //         .password = {0},
    //     },
    // };

    wifi_config_t wifi_configuration = {0};

    strlcpy((char *)wifi_configuration.sta.ssid, ssid, sizeof(wifi_configuration.sta.ssid));
    strlcpy((char *)wifi_configuration.sta.password, pass, sizeof(wifi_configuration.sta.password));

    // Copy ssid to wifi_configuration.sta.ssid
    // for (int i = 0; ssid[i] != '\0'; ++i)
    // {
    //     wifi_configuration.sta.ssid[i] = (uint8_t)ssid[i];
    // }

    // // Copy pass to wifi_configuration.sta.password
    // for (int i = 0; pass[i] != '\0'; ++i)
    // {
    //     wifi_configuration.sta.password[i] = (uint8_t)pass[i];
    // }

    // Print ssid and password for verification
    printf("SSID: %s\n", wifi_configuration.sta.ssid);
    printf("Password: %s\n", wifi_configuration.sta.password);

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration); // setting up configs when event ESP_IF_WIFI_STA
    esp_wifi_start();                                          // start connection with configurations provided in funtion
    esp_wifi_set_mode(WIFI_MODE_STA);                          // station mode selected
    esp_wifi_connect();                                        // connect with saved ssid and pass
    printf("wifi_init_softap finished. SSID:%s  password:%s", ssid, pass);
}



void app_main(void)
{

    nvs_flash_init(); // this is important in wifi case to store configurations , code will not work if this is not added
    wifi_connection();

    vTaskDelay(3000 / portTICK_PERIOD_MS); // delay is important cause we need to let it connect to wifi
    mqtt_initialize();                     // MQTT start app as shown above most important code for MQTT

    vTaskDelay(1000);
}