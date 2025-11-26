/*
 * ESP32 BoilerStat Production - ESP-IDF Native Implementation
 * 
 * Reads real boiler and zone sensor inputs via GPIO and publishes via MQTT
 * Compatible with the Python mqtt_database_logger.py data format
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "config.h"

// Configuration Constants now loaded from config.h
#define GPIO_INPUT_PIN_SEL ((1ULL<<BURNER_PIN) | (1ULL<<ZONE_1_PIN) | \
                           (1ULL<<ZONE_2_PIN) | (1ULL<<ZONE_3_PIN) | \
                           (1ULL<<ZONE_4_PIN) | (1ULL<<ZONE_5_PIN) | \
                           (1ULL<<ZONE_6_PIN))

// Debounce Configuration

// Demo Mode Configuration
#define DEMO_MODE_CYCLE_MS (60 * 1000)  // 60 second cycle for demo data
#define DEMO_BURNER_BASE_RATE 30        // Base utilization % for burner
#define DEMO_ZONE_BASE_RATE 20          // Base utilization % for zones

// WiFi Event Bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// Global Variables
static const char *TAG = "BOILERSTAT_PROD";

// Input state tracking for debouncing
typedef struct {
    int current_state;
    int stable_state;
    int debounce_counter;
    TickType_t last_change_time;
} gpio_debounce_t;

// LED Status System
typedef enum {
    LED_STATE_BOOTING = 0,
    LED_STATE_WIFI_DISCONNECTED,
    LED_STATE_MQTT_DISCONNECTED,
    LED_STATE_NTP_NOT_SYNCED,
    LED_STATE_OPERATIONAL,
    LED_STATE_ERROR
} led_status_state_t;

static gpio_debounce_t input_states[7]; // 0=boiler, 1-6=zones
static EventGroupHandle_t s_wifi_event_group;
static esp_mqtt_client_handle_t mqtt_client;
static int s_retry_num = 0;
static bool wifi_connected = false;
static bool mqtt_connected = false;
static bool ntp_synchronized = false;
static int publish_count = 1;
static bool demo_mode = false;
static uint32_t demo_seed = 0;

// LED Status Variables
static led_status_state_t current_led_state = LED_STATE_BOOTING;
static bool error_condition = false;

// Function Prototypes
static void wifi_init_sta(void);
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void mqtt_app_start(void);
static void gpio_init(void);
static void led_init(void);
static void led_flash_wifi_connected(void);
static void led_flash_mqtt_publish(void);
static void led_set_level(int level);
static void led_status_task(void *pvParameters);
static void update_led_state(void);
static void led_heartbeat_on_publish(void);
void set_led_error_state(bool error);
static void initialize_sntp(void);
static void get_formatted_time(char *time_str, size_t max_len);
static void publish_sensor_data_task(void *pvParameters);
static void gpio_read_task(void *pvParameters);
static cJSON* generate_sensor_reading(void);
static int read_debounced_gpio(gpio_num_t pin, int input_index);
static void update_input_debounce(int input_index, int raw_state);
static void ntp_sync_task(void *pvParameters);
static void generate_demo_data(int *burner, int zones[6]);
static void process_mqtt_control_message(const char *data, int data_len);
static void led_flash_mode_change(void);

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 BoilerStat Production Starting...");
    ESP_LOGI(TAG, "========================================");
    
    // Initialize LED
    led_init();
    
    // Initialize GPIO inputs
    gpio_init();
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi
    wifi_init_sta();
    
    // Initialize SNTP for time synchronization
    initialize_sntp();
    
    // Initialize MQTT
    mqtt_app_start();
    
    ESP_LOGI(TAG, "Setup complete. Starting tasks...");
    
    // Create task for reading GPIO inputs
    xTaskCreate(gpio_read_task, "gpio_read_task", 2048, NULL, 4, NULL);
    
    // Create task for publishing sensor data
    xTaskCreate(publish_sensor_data_task, "publish_task", 4096, NULL, 5, NULL);
    
    // Create task for continuous NTP synchronization
    xTaskCreate(ntp_sync_task, "ntp_sync_task", 2048, NULL, 3, NULL);
    
    // Create LED status monitoring task
    xTaskCreate(led_status_task, "led_status_task", 2048, NULL, 2, NULL);
    
    // Exit booting state - LED will now show actual system status
    current_led_state = LED_STATE_WIFI_DISCONNECTED;
}

static void gpio_init(void)
{
    ESP_LOGI(TAG, "Initializing GPIO inputs for boiler and zone sensors");
    
    // Configure input pins
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = GPIO_INPUT_PIN_SEL,
        .pull_down_en = 1,  // Enable pull-down resistors for clean low state
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    
    // Initialize input state tracking
    for (int i = 0; i < 7; i++) {
        input_states[i].current_state = 0;
        input_states[i].stable_state = 0;
        input_states[i].debounce_counter = 0;
        input_states[i].last_change_time = 0;
    }
    
    ESP_LOGI(TAG, "GPIO pins configured:");
    ESP_LOGI(TAG, "  Burner: GPIO %d", BURNER_PIN);
    ESP_LOGI(TAG, "  Zone 1: GPIO %d", ZONE_1_PIN);
    ESP_LOGI(TAG, "  Zone 2: GPIO %d", ZONE_2_PIN);
    ESP_LOGI(TAG, "  Zone 3: GPIO %d", ZONE_3_PIN);
    ESP_LOGI(TAG, "  Zone 4: GPIO %d", ZONE_4_PIN);
    ESP_LOGI(TAG, "  Zone 5: GPIO %d", ZONE_5_PIN);
    ESP_LOGI(TAG, "  Zone 6: GPIO %d", ZONE_6_PIN);
}

static void gpio_read_task(void *pvParameters)
{
    const gpio_num_t pins[] = {BURNER_PIN, ZONE_1_PIN, ZONE_2_PIN, ZONE_3_PIN, 
                              ZONE_4_PIN, ZONE_5_PIN, ZONE_6_PIN};
    
    ESP_LOGI(TAG, "GPIO reading task started");
    
    while (1) {
        // Read all input pins and update debounced states
        for (int i = 0; i < 7; i++) {
            int raw_state = gpio_get_level(pins[i]);
            update_input_debounce(i, raw_state);
        }
        
        // Small delay for debouncing
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static void update_input_debounce(int input_index, int raw_state)
{
    gpio_debounce_t *input = &input_states[input_index];
    TickType_t current_time = xTaskGetTickCount();
    
    if (raw_state != input->current_state) {
        // State change detected
        input->current_state = raw_state;
        input->debounce_counter = 1;
        input->last_change_time = current_time;
    } else {
        // Same state - increment counter if within debounce window
        TickType_t time_diff = current_time - input->last_change_time;
        if (time_diff < (DEBOUNCE_TIME_MS / portTICK_PERIOD_MS)) {
            input->debounce_counter++;
            
            // If we have enough stable readings, update the stable state
            if (input->debounce_counter >= READING_STABLE_COUNT && 
                input->stable_state != input->current_state) {
                input->stable_state = input->current_state;
                
                // Log state changes for debugging
                const char* names[] = {"Burner", "Zone1", "Zone2", "Zone3", "Zone4", "Zone5", "Zone6"};
                ESP_LOGI(TAG, "%s state changed to: %s", names[input_index], 
                        input->stable_state ? "ON" : "OFF");
            }
        }
    }
}

static int read_debounced_gpio(gpio_num_t pin, int input_index)
{
    // Invert the stable state to match old program logic (!digitalRead)
    return !input_states[input_index].stable_state;
}

static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi network: %s", WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to WiFi network: %s", WIFI_SSID);
        wifi_connected = true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to WiFi network: %s", WIFI_SSID);
        wifi_connected = false;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

static void event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        wifi_connected = false;
        mqtt_connected = false;
        ESP_LOGI(TAG,"Connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        wifi_connected = true;
        
        // Note: WiFi connection now handled by led_status_task
    }
}

static void mqtt_app_start(void)
{
    // Create unique client ID with timestamp to prevent conflicts
    char unique_client_id[64];
    time_t now;
    time(&now);
    snprintf(unique_client_id, sizeof(unique_client_id), "%s_%lld", MQTT_CLIENT_ID, (long long)now);
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.client_id = unique_client_id,
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    
    ESP_LOGI(TAG, "MQTT client ID: %s", unique_client_id);
    
    ESP_LOGI(TAG, "MQTT client configured for broker: %s", MQTT_BROKER_URI);
    ESP_LOGI(TAG, "Publishing to topic: %s", MQTT_TOPIC);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_connected = true;
        // Subscribe to control topic for mode switching
        int msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_CONTROL_TOPIC, 0);
        ESP_LOGI(TAG, "Subscribed to %s, msg_id=%d", MQTT_CONTROL_TOPIC, msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_connected = false;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA on topic %.*s", event->topic_len, event->topic);
        // Check if this is a control message
        if (strncmp(event->topic, MQTT_CONTROL_TOPIC, event->topic_len) == 0) {
            process_mqtt_control_message(event->data, event->data_len);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP with enhanced time synchronization");
    
    // Set timezone to UTC to ensure consistent timestamps
    setenv("TZ", "UTC", 1);
    tzset();
    
    // Configure SNTP for automatic periodic synchronization
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    
    // Set multiple NTP servers for redundancy
    esp_sntp_setservername(0, "192.168.1.1");        // Local router - fastest
    esp_sntp_setservername(1, "pool.ntp.org");       // Backup public server
    esp_sntp_setservername(2, "time.nist.gov");      // Second backup
    esp_sntp_setservername(3, "time.google.com");    // Third backup
    
    // Enable smooth time adjustment instead of abrupt changes
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    
    // Set sync interval to 1 hour (3600 seconds) for periodic resyncing
    esp_sntp_set_sync_interval(3600000);  // 1 hour in milliseconds
    
    esp_sntp_init();
    
    // Wait for initial time synchronization
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    
    while (++retry <= retry_count) {
        ESP_LOGI(TAG, "Waiting for initial time sync... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        
        time(&now);
        if (now > 1577836800) {  // If time is after Jan 1, 2020, consider it synchronized
            localtime_r(&now, &timeinfo);
            ntp_synchronized = true;
            ESP_LOGI(TAG, "Initial time sync successful: %04d-%02d-%02d %02d:%02d:%02d UTC", 
                    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            ESP_LOGI(TAG, "Automatic resyncing enabled every hour");
            return;
        }
    }
    
    ESP_LOGW(TAG, "Initial time sync failed after %d attempts", retry_count);
    ESP_LOGI(TAG, "Will continue attempting sync in background task");
}

static void get_formatted_time(char *time_str, size_t max_len)
{
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    gmtime_r(&now, &timeinfo);  // Use UTC time for consistency with data aggregator
    
    strftime(time_str, max_len, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

static cJSON* generate_sensor_reading(void)
{
    char time_str[20];
    get_formatted_time(time_str, sizeof(time_str));
    
    cJSON *json = cJSON_CreateObject();
    cJSON *timestamp = cJSON_CreateString(time_str);
    
    int burner_val, zone_vals[6];
    
    if (demo_mode) {
        // Generate realistic demo data
        generate_demo_data(&burner_val, zone_vals);
    } else {
        // Read actual GPIO states
        burner_val = read_debounced_gpio(BURNER_PIN, 0);
        zone_vals[0] = read_debounced_gpio(ZONE_1_PIN, 1);
        zone_vals[1] = read_debounced_gpio(ZONE_2_PIN, 2);
        zone_vals[2] = read_debounced_gpio(ZONE_3_PIN, 3);
        zone_vals[3] = read_debounced_gpio(ZONE_4_PIN, 4);
        zone_vals[4] = read_debounced_gpio(ZONE_5_PIN, 5);
        zone_vals[5] = read_debounced_gpio(ZONE_6_PIN, 6);
    }
    
    cJSON *burner = cJSON_CreateNumber(burner_val);
    cJSON *zone_1 = cJSON_CreateNumber(zone_vals[0]);
    cJSON *zone_2 = cJSON_CreateNumber(zone_vals[1]);
    cJSON *zone_3 = cJSON_CreateNumber(zone_vals[2]);
    cJSON *zone_4 = cJSON_CreateNumber(zone_vals[3]);
    cJSON *zone_5 = cJSON_CreateNumber(zone_vals[4]);
    cJSON *zone_6 = cJSON_CreateNumber(zone_vals[5]);
    cJSON *is_demo_flag = cJSON_CreateBool(demo_mode);
    
    cJSON_AddItemToObject(json, "timestamp", timestamp);
    cJSON_AddItemToObject(json, "burner", burner);
    cJSON_AddItemToObject(json, "zone_1", zone_1);
    cJSON_AddItemToObject(json, "zone_2", zone_2);
    cJSON_AddItemToObject(json, "zone_3", zone_3);
    cJSON_AddItemToObject(json, "zone_4", zone_4);
    cJSON_AddItemToObject(json, "zone_5", zone_5);
    cJSON_AddItemToObject(json, "zone_6", zone_6);
    cJSON_AddItemToObject(json, "is_demo", is_demo_flag);
    
    return json;
}

static void publish_sensor_data_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor data publishing task started");
    
    while (1) {
        if (wifi_connected && mqtt_connected && ntp_synchronized) {
            cJSON *sensor_data = generate_sensor_reading();
            char *json_string = cJSON_Print(sensor_data);
            
            if (json_string != NULL) {
                int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, json_string, 0, 0, 0);
                
                if (msg_id >= 0) {
                    cJSON *timestamp = cJSON_GetObjectItem(sensor_data, "timestamp");
                    cJSON *burner_obj = cJSON_GetObjectItem(sensor_data, "burner");
                    cJSON *z1 = cJSON_GetObjectItem(sensor_data, "zone_1");
                    cJSON *z2 = cJSON_GetObjectItem(sensor_data, "zone_2");
                    cJSON *z3 = cJSON_GetObjectItem(sensor_data, "zone_3");
                    cJSON *z4 = cJSON_GetObjectItem(sensor_data, "zone_4");
                    cJSON *z5 = cJSON_GetObjectItem(sensor_data, "zone_5");
                    cJSON *z6 = cJSON_GetObjectItem(sensor_data, "zone_6");
                    
                    ESP_LOGI(TAG, "[%d] Published at %s (%s MODE)", publish_count, timestamp->valuestring, 
                            demo_mode ? "DEMO" : "PRODUCTION");
                    ESP_LOGI(TAG, "    Burner: %d | Zones: %d %d %d %d %d %d", 
                            (int)burner_obj->valuedouble, (int)z1->valuedouble, (int)z2->valuedouble, 
                            (int)z3->valuedouble, (int)z4->valuedouble, (int)z5->valuedouble, (int)z6->valuedouble);
                    ESP_LOGI(TAG, "    JSON: %s", json_string);
                    
                    // Note: LED status now handled by led_status_task - no overlapping flashes
                    
                    publish_count++;
                } else {
                    ESP_LOGW(TAG, "[%d] Publish failed!", publish_count);
                }
                
                free(json_string);
            }
            
            cJSON_Delete(sensor_data);
        } else {
            if (!ntp_synchronized) {
                ESP_LOGW(TAG, "NTP not synchronized - skipping publish");
            } else {
                ESP_LOGW(TAG, "WiFi or MQTT not connected - skipping publish");
            }
        }
        
        vTaskDelay(PUBLISH_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

// LED Control Functions
static void led_init(void)
{
    // Configure LED GPIO pin
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_GPIO_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    
    // Start with LED OFF (since it's active-low)
    led_set_level(LED_OFF_LEVEL);
    
    ESP_LOGI(TAG, "LED initialized on GPIO %d", LED_GPIO_PIN);
}

static void led_set_level(int level)
{
    gpio_set_level(LED_GPIO_PIN, level);
}

static void led_flash_wifi_connected(void)
{
    ESP_LOGI(TAG, "WiFi connected - flashing LED 10 times");
    
    // Flash 10 times in 3 seconds = 300ms per flash cycle (150ms on, 150ms off)
    for (int i = 0; i < 10; i++) {
        led_set_level(LED_ON_LEVEL);
        vTaskDelay(150 / portTICK_PERIOD_MS);
        led_set_level(LED_OFF_LEVEL);
        vTaskDelay(150 / portTICK_PERIOD_MS);
    }
    // Ensure LED is OFF after flash sequence
    led_set_level(LED_OFF_LEVEL);
}

static void led_flash_mqtt_publish(void)
{
    // Flash 3 times in 1 second = 333ms per flash cycle (166ms on, 167ms off)
    for (int i = 0; i < 3; i++) {
        led_set_level(LED_ON_LEVEL);   // Turn LED ON (flash)
        vTaskDelay(166 / portTICK_PERIOD_MS);
        led_set_level(LED_OFF_LEVEL);  // Turn LED OFF
        if (i < 2) { // Don't delay after the last flash
            vTaskDelay(167 / portTICK_PERIOD_MS);
        }
    }
    // Ensure LED is OFF after flash sequence
    led_set_level(LED_OFF_LEVEL);
}

static void ntp_sync_task(void *pvParameters)
{
    TickType_t last_check = xTaskGetTickCount();
    const TickType_t check_interval = pdMS_TO_TICKS(30000); // Check every 30 seconds
    int sync_attempt_count = 0;
    
    while (1) {
        vTaskDelayUntil(&last_check, check_interval);
        sync_attempt_count++;
        
        if (!wifi_connected) {
            ESP_LOGW(TAG, "NTP sync skipped - WiFi not connected (attempt %d)", sync_attempt_count);
            continue;
        }
        
        if (ntp_synchronized) {
            // Already synchronized, just verify time is still valid
            time_t now;
            time(&now);
            if (now > 1577836800) {
                ESP_LOGD(TAG, "NTP sync verified - time is valid (timestamp: %lld)", (long long)now);
            } else {
                ESP_LOGW(TAG, "NTP sync lost - time reverted to invalid value: %lld", (long long)now);
                ntp_synchronized = false;
            }
        } else {
            // Not synchronized, check current time
            time_t now;
            time(&now);
            
            ESP_LOGI(TAG, "NTP sync check #%d - current timestamp: %lld", sync_attempt_count, (long long)now);
            
            if (now > 1577836800) {  // If time is after Jan 1, 2020
                struct tm timeinfo;
                localtime_r(&now, &timeinfo);
                ntp_synchronized = true;
                ESP_LOGI(TAG, "NTP sync SUCCESS: %04d-%02d-%02d %02d:%02d:%02d UTC (attempt %d)", 
                        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, sync_attempt_count);
            } else {
                ESP_LOGW(TAG, "NTP sync FAILED - time still invalid: %lld (attempt %d)", (long long)now, sync_attempt_count);
                ESP_LOGI(TAG, "NTP servers configured: 192.168.1.1, pool.ntp.org, time.nist.gov, time.google.com");
            }
        }
    }
}

static void generate_demo_data(int *burner, int zones[6])
{
    // Initialize demo seed if not already done
    if (demo_seed == 0) {
        demo_seed = esp_random();
        ESP_LOGI(TAG, "Demo mode initialized with seed: %lu", demo_seed);
    }
    
    // Get current time for cycle calculation
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t cycle_position = current_time % DEMO_MODE_CYCLE_MS;
    float cycle_fraction = (float)cycle_position / DEMO_MODE_CYCLE_MS;
    
    // Target utilization rates for each zone (zone number * 10%)
    // Zone 1: 10%, Zone 2: 20%, Zone 3: 30%, Zone 4: 40%, Zone 5: 50%, Zone 6: 60%
    float zone_target_utilization[6] = {0.10, 0.20, 0.30, 0.40, 0.50, 0.60};
    float zone_variation_range = 0.20; // ±20% variation
    
    // Burner target: 50% utilization with ±30% variation  
    float burner_target_utilization = 0.50;
    float burner_variation_range = 0.30;
    
    // Generate zone states based on target utilization rates
    for (int i = 0; i < 6; i++) {
        float target_rate = zone_target_utilization[i];
        
        // Add ±20% variation (of the target rate itself)
        float variation = (((esp_random() % 1000) / 1000.0) - 0.5) * 2.0 * (target_rate * zone_variation_range);
        float adjusted_rate = target_rate + variation;
        
        // Ensure rate stays within reasonable bounds (0-1)
        if (adjusted_rate < 0.0) adjusted_rate = 0.0;
        if (adjusted_rate > 1.0) adjusted_rate = 1.0;
        
        // Generate zone state based on simple probability
        zones[i] = ((esp_random() % 1000) / 1000.0 < adjusted_rate) ? 1 : 0;
    }
    
    // Generate burner state based on target 50% utilization with ±30% variation
    float burner_variation = (((esp_random() % 1000) / 1000.0) - 0.5) * 2.0 * (burner_target_utilization * burner_variation_range);
    float adjusted_burner_rate = burner_target_utilization + burner_variation;
    
    // Ensure rate stays within bounds
    if (adjusted_burner_rate < 0.0) adjusted_burner_rate = 0.0;
    if (adjusted_burner_rate > 1.0) adjusted_burner_rate = 1.0;
    
    // Generate burner state based on simple probability
    *burner = ((esp_random() % 1000) / 1000.0 < adjusted_burner_rate) ? 1 : 0;
    
    // Light correlation - if many zones calling, burner slightly more likely on
    int active_zones = 0;
    for (int i = 0; i < 6; i++) {
        if (zones[i]) active_zones++;
    }
    
    // If multiple zones are calling, increase burner probability slightly
    if (active_zones >= 3 && *burner == 0) {
        // 30% chance to turn burner on when 3+ zones calling
        if ((esp_random() % 100) < 30) {
            *burner = 1;
        }
    }
}

static void process_mqtt_control_message(const char *data, int data_len)
{
    // Create a null-terminated string from the message data
    char message[data_len + 1];
    strncpy(message, data, data_len);
    message[data_len] = '\0';
    
    ESP_LOGI(TAG, "Received control message: %s", message);
    
    // Parse JSON control message
    cJSON *json = cJSON_Parse(message);
    if (json == NULL) {
        ESP_LOGW(TAG, "Invalid JSON in control message");
        return;
    }
    
    // Look for demo_mode field
    cJSON *demo_mode_item = cJSON_GetObjectItem(json, "demo_mode");
    if (demo_mode_item != NULL && cJSON_IsBool(demo_mode_item)) {
        bool new_demo_mode = cJSON_IsTrue(demo_mode_item);
        
        if (new_demo_mode != demo_mode) {
            demo_mode = new_demo_mode;
            ESP_LOGI(TAG, "Mode changed to: %s", demo_mode ? "DEMO" : "PRODUCTION");
            
            // Flash LED to indicate mode change
            led_flash_mode_change();
            
            // Reset demo seed when entering demo mode
            if (demo_mode) {
                demo_seed = 0;
            }
        }
    } else {
        ESP_LOGW(TAG, "Control message missing or invalid 'demo_mode' field");
    }
    
    cJSON_Delete(json);
}

static void led_flash_mode_change(void)
{
    ESP_LOGI(TAG, "Mode change - flashing LED pattern");
    
    if (demo_mode) {
        // Entering demo mode: 5 quick flashes
        for (int i = 0; i < 5; i++) {
            led_set_level(LED_ON_LEVEL);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            led_set_level(LED_OFF_LEVEL);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    } else {
        // Entering production mode: 2 long flashes
        for (int i = 0; i < 2; i++) {
            led_set_level(LED_ON_LEVEL);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            led_set_level(LED_OFF_LEVEL);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
    
    // Ensure LED is off after pattern
    led_set_level(LED_OFF_LEVEL);
}

// LED Status System Implementation
static void update_led_state(void)
{
    if (error_condition) {
        current_led_state = LED_STATE_ERROR;
    } else if (!wifi_connected) {
        current_led_state = LED_STATE_WIFI_DISCONNECTED;
    } else if (!mqtt_connected) {
        current_led_state = LED_STATE_MQTT_DISCONNECTED;
    } else if (!ntp_synchronized) {
        current_led_state = LED_STATE_NTP_NOT_SYNCED;
    } else {
        current_led_state = LED_STATE_OPERATIONAL;
    }
}

static void led_status_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LED status monitoring task started");
    
    while (1) {
        update_led_state();
        
        switch (current_led_state) {
            case LED_STATE_BOOTING:
                // Continuous rapid blink (100ms on/off)
                led_set_level(LED_ON_LEVEL);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                led_set_level(LED_OFF_LEVEL);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                break;
                
            case LED_STATE_WIFI_DISCONNECTED:
                // 1 long blink every 3 seconds
                led_set_level(LED_ON_LEVEL);
                vTaskDelay(500 / portTICK_PERIOD_MS);  // 500ms ON
                led_set_level(LED_OFF_LEVEL);
                vTaskDelay(2500 / portTICK_PERIOD_MS); // 2.5 sec OFF (total 3 sec cycle)
                break;
                
            case LED_STATE_MQTT_DISCONNECTED:
                // 2 distinct blinks every 3 seconds
                for (int i = 0; i < 2; i++) {
                    led_set_level(LED_ON_LEVEL);
                    vTaskDelay(250 / portTICK_PERIOD_MS);  // 250ms ON
                    led_set_level(LED_OFF_LEVEL);
                    vTaskDelay(250 / portTICK_PERIOD_MS);  // 250ms OFF between blinks
                }
                vTaskDelay(2000 / portTICK_PERIOD_MS); // 2 sec pause (total 3 sec cycle)
                break;
                
            case LED_STATE_NTP_NOT_SYNCED:
                // 3 distinct blinks every 3 seconds
                for (int i = 0; i < 3; i++) {
                    led_set_level(LED_ON_LEVEL);
                    vTaskDelay(200 / portTICK_PERIOD_MS);  // 200ms ON
                    led_set_level(LED_OFF_LEVEL);
                    vTaskDelay(200 / portTICK_PERIOD_MS);  // 200ms OFF between blinks
                }
                vTaskDelay(1800 / portTICK_PERIOD_MS); // 1.8 sec pause (total 3 sec cycle)
                break;
                
            case LED_STATE_OPERATIONAL:
                // 1 fast blink every 6 seconds (all is well heartbeat)
                static uint32_t last_operational_blink = 0;
                uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                
                if (current_time - last_operational_blink >= 6000) {
                    led_set_level(LED_ON_LEVEL);
                    vTaskDelay(100 / portTICK_PERIOD_MS);  // 100ms fast blink
                    led_set_level(LED_OFF_LEVEL);
                    last_operational_blink = current_time;
                    vTaskDelay(5900 / portTICK_PERIOD_MS); // Rest of 6 sec cycle
                } else {
                    led_set_level(LED_OFF_LEVEL);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                break;
                
            case LED_STATE_ERROR:
                // SOS pattern (· · · — — — · · ·)
                // Three short blinks (dots)
                for (int i = 0; i < 3; i++) {
                    led_set_level(LED_ON_LEVEL);
                    vTaskDelay(150 / portTICK_PERIOD_MS);
                    led_set_level(LED_OFF_LEVEL);
                    vTaskDelay(150 / portTICK_PERIOD_MS);
                }
                vTaskDelay(300 / portTICK_PERIOD_MS); // Pause between groups
                
                // Three long blinks (dashes)
                for (int i = 0; i < 3; i++) {
                    led_set_level(LED_ON_LEVEL);
                    vTaskDelay(450 / portTICK_PERIOD_MS);
                    led_set_level(LED_OFF_LEVEL);
                    vTaskDelay(150 / portTICK_PERIOD_MS);
                }
                vTaskDelay(300 / portTICK_PERIOD_MS); // Pause between groups
                
                // Three short blinks (dots)
                for (int i = 0; i < 3; i++) {
                    led_set_level(LED_ON_LEVEL);
                    vTaskDelay(150 / portTICK_PERIOD_MS);
                    led_set_level(LED_OFF_LEVEL);
                    vTaskDelay(150 / portTICK_PERIOD_MS);
                }
                vTaskDelay(2000 / portTICK_PERIOD_MS); // Long pause before repeating SOS
                break;
        }
    }
}

static void led_heartbeat_on_publish(void)
{
    // Only do heartbeat in operational mode
    if (current_led_state == LED_STATE_OPERATIONAL) {
        led_set_level(LED_ON_LEVEL);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        led_set_level(LED_OFF_LEVEL);
    }
}

// Function to set error condition (can be called from anywhere in code)
void set_led_error_state(bool error)
{
    error_condition = error;
    ESP_LOGI(TAG, "LED error state %s", error ? "SET" : "CLEARED");
}