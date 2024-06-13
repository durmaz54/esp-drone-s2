#include <stdio.h>
#include "BNO08x.hpp"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "esp_mac.h"

#include "esp_wifi.h"
#include "esp_private/wifi.h"
#include "esp_task_wdt.h"


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "motor.h"
#include "dz_pid.h"

#define UDP_IP "192.168.4.2"
#define PORT 1234
#define KEEPALIVE_IDLE 7200
#define KEEPALIVE_INTERVAL 75
#define KEEPALIVE_COUNT 9

#define TIMER_PERIOD_MS 10


static const char *TAG = "DRONE";

int rc_throttle = -1;
int rc_yaw = 0;
int rc_roll = 0;
int rc_pitch = 0;

int16_t yaw_ref, roll_ref, pitch_ref;



void parse_and_assign(const uint8_t* data, int length) {
    if (length >= 18 && data[0] == 0x02 && data[length-1] == 0x03) { // 18 bytes: 4 * 4 bytes for data + 2 bytes for start/end
        int temp_throttle, temp_yaw, temp_roll, temp_pitch;
        memcpy(&temp_throttle, data + 1, 4);
        memcpy(&temp_yaw, data + 5, 4);
        memcpy(&temp_roll, data + 9, 4);
        memcpy(&temp_pitch, data + 13, 4);
        
        // Verileri ata
        rc_throttle = ntohl(temp_throttle);
        rc_yaw = ntohl(temp_yaw);
        rc_roll = ntohl(temp_roll);
        rc_pitch = ntohl(temp_pitch);

        ESP_LOGI(TAG, "Throttle: %d, Yaw: %d, Roll: %d, Pitch: %d", rc_throttle, rc_yaw, rc_roll, rc_pitch);        
    } else {
        ESP_LOGE(TAG, "Invalid packet received");
    }
}


static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);

        rc_throttle = -1;
    }
}

void wifi_init(){


        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_ap();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));

wifi_config_t wifi_config = {
    .ap = {
        .ssid = CONFIG_ESP_WIFI_SSID,
        .password = CONFIG_ESP_WIFI_PASSWORD,
        .ssid_len = strlen(CONFIG_ESP_WIFI_SSID),
        .channel = 4,
        .authmode = WIFI_AUTH_WPA2_PSK,
        .ssid_hidden = 0,
        .max_connection = 4,
        .beacon_interval = 100,
        .pairwise_cipher = WIFI_CIPHER_TYPE_NONE,
        .ftm_responder = 0,
        .pmf_cfg = { .required = false ,
        },
        .sae_pwe_h2e = WPA3_SAE_PWE_UNSPECIFIED
    }
};


        if (strlen(CONFIG_ESP_WIFI_PASSWORD) == 0)
        {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "wifi_init_softap finished. SSID");
    

    }


static void wifi_task(void *pvParameters)
{
    char addr_str[128];
    uint8_t rx_buffer[128];
    int addr_family = AF_INET;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

    if (addr_family == AF_INET)
    {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0)
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1)
    {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
        if (source_addr.ss_family == PF_INET)
        {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        ESP_LOGI(TAG, "SOC = %d", sock);

        while (1)
        {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0)
            {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            else if (len == 0)
            {
                ESP_LOGI(TAG, "Connection closed");
                rc_throttle = -1;
                break;
            }
            else
            {
                rx_buffer[len] = 0;
                ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
                parse_and_assign(rx_buffer, len);

            }
        }

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}


int16_t between_1100_1800(int16_t value)
{
    if (value < 1000)
    {
        return 1000;
    }
    if (value > 1800)
    {
        return 1800;
    }
    return value;
}

int16_t yawCalculate(int16_t yaw) {
    yaw = yaw % 360;
    if(yaw > 180){
        yaw -= 360;
    }
    else if(yaw < -180){
        yaw += 360;
    }
    return yaw;
}


void imu_task( void *pvParameters){
    bno08x_config_t imu_config;
    imu_config.spi_peripheral = SPI2_HOST;
    imu_config.io_mosi = GPIO_NUM_10; //11
    imu_config.io_miso = GPIO_NUM_12; //13
    imu_config.io_sclk = GPIO_NUM_11; //12
    imu_config.io_cs = GPIO_NUM_9; //10
    imu_config.io_int = GPIO_NUM_14; //15
    imu_config.io_rst = GPIO_NUM_13; //14
    //imu_config.io_wake = GPIO_NUM_4;
    imu_config.sclk_speed = 2000000UL;

    BNO08x imu(imu_config);
    
    
    imu.initialize();
    imu.enable_rotation_vector(1);

    BMotor bmotor;
    bmotor_init();
    bmotor.motor1 = 1000;
    bmotor.motor2 = 1000;
    bmotor.motor3 = 1000;
    bmotor.motor4 = 1000;

    bmotor_write(&bmotor);
    vTaskDelay(5000 / portTICK_PERIOD_MS);



    while (1)
    {
        if(imu.data_available())
        {
            yaw_ref = imu.get_yaw_deg();
            roll_ref = imu.get_roll_deg();
            pitch_ref = imu.get_pitch_deg();
            break;
        }
    }
    

    while(1)
    {
        if(imu.data_available() && rc_throttle >= 1000)
        {
            int16_t roll = (int16_t)imu.get_roll_deg();
            int16_t pitch = (int16_t)imu.get_pitch_deg();
            int16_t yaw = (int16_t)imu.get_yaw_deg();

            yaw = yaw - yaw_ref;
            roll = roll - roll_ref;
            pitch = pitch - pitch_ref;
            yaw = yawCalculate(yaw);

            int16_t roll_pid = pidRollCalculate(rc_roll, roll);
            int16_t pitch_pid = pidPitchCalculate(rc_pitch, pitch);
            int16_t yaw_pid = pidYawCalculate(rc_yaw, yaw);

            // ESP_LOGI(TAG, "Roll_pid: %d, Pitch_pid: %d, Yaw_pid: %d", roll_pid, pitch_pid, yaw_pid);
            // ESP_LOGI(TAG, "Roll: %d, Pitch: %d, Yaw: %d", roll, pitch, yaw);

            
            bmotor.motor1 = (rc_throttle - roll_pid + pitch_pid - yaw_pid);
            bmotor.motor2 =  (rc_throttle - roll_pid - pitch_pid + yaw_pid);
            bmotor.motor3 =  (rc_throttle + roll_pid - pitch_pid - yaw_pid);
            bmotor.motor4 =  (rc_throttle + roll_pid + pitch_pid + yaw_pid);
 
            // bmotor.motor1 = rc_throttle;
            // bmotor.motor2 = rc_throttle;
            // bmotor.motor3 = rc_throttle;
            // bmotor.motor4 = rc_throttle;


            bmotor.motor1 = between_1100_1800(bmotor.motor1);
            bmotor.motor2 = between_1100_1800(bmotor.motor2);
            bmotor.motor3 = between_1100_1800(bmotor.motor3);
            bmotor.motor4 = between_1100_1800(bmotor.motor4);
            ESP_LOGI(TAG, "Motor1: %d, Motor2: %d, Motor3: %d, Motor4: %d", bmotor.motor1, bmotor.motor2, bmotor.motor3, bmotor.motor4);

            bmotor_write(&bmotor);
        }
        else if (rc_throttle < 1000){
            pidRollReset();
            pidPitchReset();
            pidYawReset();
            bmotor.motor1 = 1000;
            bmotor.motor2 = 1000;
            bmotor.motor3 = 1000;
            bmotor.motor4 = 1000;

            ESP_LOGI(TAG, "Motor1: %d, Motor2: %d, Motor3: %d, Motor4: %d", bmotor.motor1, bmotor.motor2, bmotor.motor3, bmotor.motor4);
            bmotor_write(&bmotor);
        }
    }

}


extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    esp_task_wdt_deinit();

    wifi_init();
    xTaskCreate(imu_task, "imu_task", 4096, NULL, 5, NULL);
    xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);

}