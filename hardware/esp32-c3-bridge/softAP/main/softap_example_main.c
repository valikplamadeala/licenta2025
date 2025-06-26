// ESP32-C3 SPI Slave + CoAP Client/Server - PARTEA 1
// Adaugam functionalitatea CoAP peste codul existent

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_slave.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

// ===== NOILE INCLUDE-URI PENTRU CoAP =====
#include "coap3/coap.h"
#include "esp_netif.h"

// =============================================================================
// CONFIGURAȚII - EXACT CA ÎNAINTE
// =============================================================================

// WiFi Configuration
#define EXAMPLE_ESP_WIFI_SSID      "esp32"
#define EXAMPLE_ESP_WIFI_PASS      "esp32wifi"
#define EXAMPLE_ESP_WIFI_CHANNEL   1
#define EXAMPLE_MAX_STA_CONN       4

// SPI Pins - EXACT CA ÎN ESP32-C2
#define SPI_PIN_MOSI    GPIO_NUM_5
#define SPI_PIN_MISO    GPIO_NUM_6
#define SPI_PIN_CLK     GPIO_NUM_7
#define SPI_PIN_CS      GPIO_NUM_10

// LED Pins STATUS
#define LED_WIFI_STATUS    GPIO_NUM_0
#define LED_DATA_RECEIVED  GPIO_NUM_1
#define LED_DATA_SEND      GPIO_NUM_2
#define LED_ERROR          GPIO_NUM_3

// ===== NOI: GPIO PINS PENTRU SMART HOME DEVICES =====
#define KITCHEN_LIGHT_PIN  GPIO_NUM_4   // Bucătărie
#define LIVING_LIGHT_PIN   GPIO_NUM_8   // Living
#define BEDROOM_LIGHT_PIN  GPIO_NUM_9   // Dormitor
#define COOLER_PIN         GPIO_NUM_18  // Cooler/Ventilator

// Buffer size
#define SPI_BUFFER_SIZE 512

// ===== NOI: CoAP CONFIGURAȚII =====
#define COAP_SERVER_PORT    5683
#define RASPBERRY_PI_IP     "192.168.4.2"  // IP-ul Raspberry Pi în subnet-ul nostru
#define COAP_MAX_PDU_SIZE   1024

static const char *TAG = "ESP32_C3_COAP";

// =============================================================================
// VARIABILE GLOBALE - EXTINSE
// =============================================================================

QueueHandle_t spi_data_queue;
QueueHandle_t coap_send_queue;  // NOU: Queue pentru datele de trimis prin CoAP

static uint8_t spi_receive_buffer[SPI_BUFFER_SIZE];
static uint8_t spi_send_buffer[SPI_BUFFER_SIZE];

// ===== NOI: VARIABILE PENTRU SMART HOME DEVICES =====
typedef struct {
    bool kitchen_light;
    bool living_light;
    bool bedroom_light;
    bool cooler;
} device_states_t;

static device_states_t device_states = {
    .kitchen_light = false,
    .living_light = false,
    .bedroom_light = false,
    .cooler = false
};

// ===== NOI: STRUCTURA PENTRU DATELE SENZORILOR =====
typedef struct {
    float temperature;
    float humidity;
    float gas_ppm;
    int pir_state;
    uint32_t timestamp;
} sensor_data_t;

// ===== NOI: VARIABILE PENTRU CoAP =====
static coap_context_t *coap_context = NULL;
// Removed unused coap_session variable

// =============================================================================
// FUNCȚII PENTRU SMART HOME DEVICES - NOI
// =============================================================================

esp_err_t init_smart_home_devices(void) {
    ESP_LOGI(TAG, "🏠 Initializing Smart Home Devices...");
    
    // Configurare pini pentru device-uri
    gpio_config_t device_conf = {
        .pin_bit_mask = (1ULL << KITCHEN_LIGHT_PIN) | 
                       (1ULL << LIVING_LIGHT_PIN) | 
                       (1ULL << BEDROOM_LIGHT_PIN) | 
                       (1ULL << COOLER_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t ret = gpio_config(&device_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to configure device GPIOs");
        return ret;
    }
    
    // Toate device-urile OFF la început
    gpio_set_level(KITCHEN_LIGHT_PIN, 0);
    gpio_set_level(LIVING_LIGHT_PIN, 0);
    gpio_set_level(BEDROOM_LIGHT_PIN, 0);
    gpio_set_level(COOLER_PIN, 0);
    
    ESP_LOGI(TAG, "✅ Smart Home Devices initialized");
    ESP_LOGI(TAG, "📍 Kitchen: GPIO%d, Living: GPIO%d, Bedroom: GPIO%d, Cooler: GPIO%d", 
             KITCHEN_LIGHT_PIN, LIVING_LIGHT_PIN, BEDROOM_LIGHT_PIN, COOLER_PIN);
    
    return ESP_OK;
}

void set_device_state(const char* device, bool state) {
    ESP_LOGI(TAG, "🏠 Setting %s to %s", device, state ? "ON" : "OFF");
    
    if (strcmp(device, "kitchen") == 0) {
        device_states.kitchen_light = state;
        gpio_set_level(KITCHEN_LIGHT_PIN, state ? 1 : 0);
        
    } else if (strcmp(device, "living") == 0) {
        device_states.living_light = state;
        gpio_set_level(LIVING_LIGHT_PIN, state ? 1 : 0);
        
    } else if (strcmp(device, "bedroom") == 0) {
        device_states.bedroom_light = state;
        gpio_set_level(BEDROOM_LIGHT_PIN, state ? 1 : 0);
        
    } else if (strcmp(device, "cooler") == 0) {
        device_states.cooler = state;
        gpio_set_level(COOLER_PIN, state ? 1 : 0);
        
    } else {
        ESP_LOGW(TAG, "⚠️ Unknown device: %s", device);
        return;
    }
    
    // LED feedback pentru device control
    gpio_set_level(LED_DATA_SEND, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LED_DATA_SEND, 0);
}

bool get_device_state(const char* device) {
    if (strcmp(device, "kitchen") == 0) {
        return device_states.kitchen_light;
    } else if (strcmp(device, "living") == 0) {
        return device_states.living_light;
    } else if (strcmp(device, "bedroom") == 0) {
        return device_states.bedroom_light;
    } else if (strcmp(device, "cooler") == 0) {
        return device_states.cooler;
    }
    return false;
}

// =============================================================================
// FUNCȚII PARSING SIMPLE - ÎMBUNĂTĂȚITE
// =============================================================================

sensor_data_t parse_sensor_json(const char* json_str) {
    sensor_data_t data = {0};
    ESP_LOGI(TAG, "🔍 === PARSING SENSOR DATA ===");
    ESP_LOGI(TAG, "Raw JSON: %s", json_str);
    
    // Parsing simplu pentru valorile principale
    char *temp_ptr = strstr(json_str, "\"temp\":");
    if (!temp_ptr) temp_ptr = strstr(json_str, "\"temperature\":");
    if (temp_ptr) {
        temp_ptr = strchr(temp_ptr, ':');
        if (temp_ptr && sscanf(temp_ptr + 1, "%f", &data.temperature) == 1) {
            ESP_LOGI(TAG, "🌡️ Temperature: %.1f°C", data.temperature);
        }
    }
    
    char *hum_ptr = strstr(json_str, "\"hum\":");
    if (!hum_ptr) hum_ptr = strstr(json_str, "\"humidity\":");
    if (hum_ptr) {
        hum_ptr = strchr(hum_ptr, ':');
        if (hum_ptr && sscanf(hum_ptr + 1, "%f", &data.humidity) == 1) {
            ESP_LOGI(TAG, "💧 Humidity: %.1f%%", data.humidity);
        }
    }
    
    char *gas_ptr = strstr(json_str, "\"gas\":");
    if (!gas_ptr) gas_ptr = strstr(json_str, "\"gas_ppm\":");
    if (gas_ptr) {
        gas_ptr = strchr(gas_ptr, ':');
        if (gas_ptr && sscanf(gas_ptr + 1, "%f", &data.gas_ppm) == 1) {
            ESP_LOGI(TAG, "💨 Gas: %.1f ppm", data.gas_ppm);
        }
    }
    
    char *pir_ptr = strstr(json_str, "\"pir\":");
    if (!pir_ptr) pir_ptr = strstr(json_str, "\"pir_state\":");
    if (pir_ptr) {
        pir_ptr = strchr(pir_ptr, ':');
        if (pir_ptr && sscanf(pir_ptr + 1, "%d", &data.pir_state) == 1) {
            ESP_LOGI(TAG, "👁️ PIR: %s", data.pir_state ? "MOTION" : "CLEAR");
        }
    }
    
    data.timestamp = xTaskGetTickCount();
    ESP_LOGI(TAG, "🔍 =========================");
    
    return data;
}

// =============================================================================
// FUNCȚII INIȚIALIZARE - PĂSTRATE EXACT
// =============================================================================

esp_err_t init_leds_simple(void) {
    ESP_LOGI(TAG, "🔧 Init LED-uri...");
    
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_WIFI_STATUS) | (1ULL << LED_DATA_RECEIVED) | 
                       (1ULL << LED_DATA_SEND) | (1ULL << LED_ERROR),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t ret = gpio_config(&led_conf);
    
    // Toate LED-urile OFF
    gpio_set_level(LED_WIFI_STATUS, 0);
    gpio_set_level(LED_DATA_RECEIVED, 0);
    gpio_set_level(LED_DATA_SEND, 0);
    gpio_set_level(LED_ERROR, 0);
    
    return ret;
}

esp_err_t init_spi_slave_fixed(void) {
    ESP_LOGI(TAG, "🔧 Init SPI Slave - FIXED VERSION...");
    ESP_LOGI(TAG, "Pins: MOSI=%d, MISO=%d, SCK=%d, CS=%d", 
             SPI_PIN_MOSI, SPI_PIN_MISO, SPI_PIN_CLK, SPI_PIN_CS);
    
    // Configurare bus SPI - EXACT CA ÎN MASTER
    spi_bus_config_t buscfg = {
        .mosi_io_num = SPI_PIN_MOSI,
        .miso_io_num = SPI_PIN_MISO,
        .sclk_io_num = SPI_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_BUFFER_SIZE,
    };
    
    // Configurare slave - COMPATIBILĂ CU MASTER
    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = SPI_PIN_CS,
        .queue_size = 3,
        .flags = 0,
        .post_setup_cb = NULL,
        .post_trans_cb = NULL,
    };
    
    // Inițializare SPI Slave
    esp_err_t ret = spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ SPI Slave Init FAILED: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "✅ SPI Slave initialized successfully!");
    return ESP_OK;
}

// =============================================================================
// CoAP CLIENT FUNCTIONS - PARTEA 2
// =============================================================================

esp_err_t init_coap_client(void) {
    ESP_LOGI(TAG, "📡 Initializing CoAP Client...");
    
    // Inițializare CoAP
    coap_startup();
    coap_set_log_level(COAP_LOG_ERR);  // Reduce la minimum logging-ul CoAP
    
    // Crează context CoAP cu configurație optimizată
    coap_context = coap_new_context(NULL);
    if (!coap_context) {
        ESP_LOGE(TAG, "❌ Failed to create CoAP context");
        return ESP_FAIL;
    }
    
    // Configurează timeout-uri mai mici pentru conexiuni rapide
    coap_context_set_max_idle_sessions(coap_context, 2);  // Max 2 sesiuni idle
    coap_context_set_session_timeout(coap_context, 10);   // Timeout 10 secunde
    
    ESP_LOGI(TAG, "✅ CoAP Client initialized successfully");
    return ESP_OK;
}

esp_err_t send_sensor_data_coap(const char* resource_path, float value) {
    if (!coap_context) {
        ESP_LOGE(TAG, "❌ CoAP context not initialized");
        return ESP_FAIL;
    }
    
    // Crează adresa pentru Raspberry Pi
    coap_address_t dst_addr;
    coap_address_init(&dst_addr);
    dst_addr.addr.sin.sin_family = AF_INET;
    dst_addr.addr.sin.sin_port = htons(COAP_SERVER_PORT);
    
    // Convertește IP-ul Raspberry Pi
    if (inet_pton(AF_INET, RASPBERRY_PI_IP, &dst_addr.addr.sin.sin_addr) != 1) {
        ESP_LOGE(TAG, "❌ Invalid Raspberry Pi IP: %s", RASPBERRY_PI_IP);
        return ESP_FAIL;
    }
    
    // Crează sesiune CoAP
    coap_session_t *session = coap_new_client_session(coap_context, NULL, &dst_addr, COAP_PROTO_UDP);
    if (!session) {
        ESP_LOGE(TAG, "❌ Failed to create CoAP session");
        return ESP_FAIL;
    }
    
    // Crează PDU pentru PUT request
    coap_pdu_t *pdu = coap_pdu_init(COAP_MESSAGE_CON, COAP_REQUEST_CODE_PUT, 
                                   coap_new_message_id(session), coap_session_max_pdu_size(session));
    if (!pdu) {
        ESP_LOGE(TAG, "❌ Failed to create CoAP PDU");
        coap_session_release(session);
        return ESP_FAIL;
    }
    
    // Adaugă calea resursei
    coap_add_option(pdu, COAP_OPTION_URI_PATH, strlen(resource_path), (const uint8_t*)resource_path);
    
    // Convertește valoarea în string simplu
    char value_str[32];
    snprintf(value_str, sizeof(value_str), "%.2f", value);
    
    // Adaugă payload-ul
    coap_add_data(pdu, strlen(value_str), (const uint8_t*)value_str);
    
    // Trimite request-ul
    coap_mid_t message_id = coap_send(session, pdu);
    if (message_id == COAP_INVALID_MID) {
        ESP_LOGE(TAG, "❌ Failed to send CoAP message");
        coap_session_release(session);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "📤 Sent %s = %.2f to Raspberry Pi", resource_path, value);
    
    // LED feedback pentru transmisie
    gpio_set_level(LED_DATA_SEND, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LED_DATA_SEND, 0);
    
    // Cleanup
    coap_session_release(session);
    return ESP_OK;
}

esp_err_t send_all_sensor_data(sensor_data_t *data) {
    ESP_LOGI(TAG, "📊 Sending all sensor data to Raspberry Pi...");
    
    esp_err_t result = ESP_OK;
    
    // Trimite temperatura
    if (send_sensor_data_coap("esp32_temperature", data->temperature) != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to send temperature");
        result = ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(500));  // Delay mai mare între trimiteri
    
    // Trimite umiditatea
    if (send_sensor_data_coap("esp32_humidity", data->humidity) != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to send humidity");
        result = ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(500));  // Delay mai mare între trimiteri
    
    // Trimite gazul
    if (send_sensor_data_coap("esp32_gaz", data->gas_ppm) != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to send gas data");
        result = ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Dacă gaz periculos, trimite alertă
    if (data->gas_ppm > 300.0f) {
        char alert_msg[64];
        snprintf(alert_msg, sizeof(alert_msg), "DANGER: %.1f ppm", data->gas_ppm);
        
        // Trimite alertă fără funcție separată (inline)
        coap_address_t dst_addr;
        coap_address_init(&dst_addr);
        dst_addr.addr.sin.sin_family = AF_INET;
        dst_addr.addr.sin.sin_port = htons(COAP_SERVER_PORT);
        inet_pton(AF_INET, RASPBERRY_PI_IP, &dst_addr.addr.sin.sin_addr);
        
        coap_session_t *session = coap_new_client_session(coap_context, NULL, &dst_addr, COAP_PROTO_UDP);
        if (session) {
            coap_pdu_t *pdu = coap_pdu_init(COAP_MESSAGE_CON, COAP_REQUEST_CODE_PUT, 
                                           coap_new_message_id(session), coap_session_max_pdu_size(session));
            if (pdu) {
                coap_add_option(pdu, COAP_OPTION_URI_PATH, 15, (const uint8_t*)"esp32_gas_alert");
                coap_add_data(pdu, strlen(alert_msg), (const uint8_t*)alert_msg);
                coap_send(session, pdu);
                ESP_LOGW(TAG, "🚨 GAS ALERT sent: %s", alert_msg);
            }
            coap_session_release(session);
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "✅ All sensor data sent successfully");
    } else {
        gpio_set_level(LED_ERROR, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(LED_ERROR, 0);
    }
    
    return result;
}

// =============================================================================
// TASK PENTRU CoAP CLIENT DATA SENDING
// =============================================================================

void coap_client_task(void *pvParameters) {
    ESP_LOGI(TAG, "📡 CoAP Client Task started");
    
    sensor_data_t received_data;
    
    while (1) {
        // Așteaptă date din coap_send_queue
        if (xQueueReceive(coap_send_queue, &received_data, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "📊 Received sensor data for CoAP transmission");
            
            // Validează datele
            if (received_data.temperature > -50.0f && received_data.temperature < 100.0f &&
                received_data.humidity >= 0.0f && received_data.humidity <= 100.0f &&
                received_data.gas_ppm >= 0.0f && received_data.gas_ppm < 10000.0f) {
                
                // Trimite datele prin CoAP
                send_all_sensor_data(&received_data);
                
            } else {
                ESP_LOGW(TAG, "⚠️ Invalid sensor data, skipping transmission");
                ESP_LOGW(TAG, "T:%.1f H:%.1f G:%.1f", 
                         received_data.temperature, received_data.humidity, received_data.gas_ppm);
            }
        }
        
        // Delay scurt pentru a nu suprasolicita rețeaua
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// =============================================================================
// CoAP SERVER FUNCTIONS - PARTEA 3
// =============================================================================

// Handler pentru device control (kitchen, living, bedroom, cooler)
// Semnătura corectă pentru ESP-IDF CoAP v3
static void coap_device_handler(coap_resource_t *resource, coap_session_t *session,
                               const coap_pdu_t *request, const coap_string_t *query,
                               coap_pdu_t *response) {
    
    // Obține numele device-ului din resursa CoAP
    coap_str_const_t *uri_path = coap_resource_get_uri_path(resource);
    if (!uri_path || uri_path->length == 0) {
        ESP_LOGW(TAG, "⚠️ Empty URI path in device request");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        return;
    }
    
    // Convertește URI path la string
    char device_name[32] = {0};
    size_t copy_len = (uri_path->length < sizeof(device_name) - 1) ? uri_path->length : sizeof(device_name) - 1;
    memcpy(device_name, uri_path->s, copy_len);
    device_name[copy_len] = '\0';
    
    ESP_LOGI(TAG, "🏠 Device request for: %s", device_name);
    
    // Verifică dacă este GET sau PUT
    coap_pdu_code_t method = coap_pdu_get_code(request);
    
    if (method == COAP_REQUEST_CODE_GET) {
        // GET - returnează starea curentă
        bool current_state = get_device_state(device_name);
        const char* state_str = current_state ? "on" : "off";
        
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
        coap_add_data(response, strlen(state_str), (const uint8_t*)state_str);
        
        ESP_LOGI(TAG, "📤 GET %s -> %s", device_name, state_str);
        
    } else if (method == COAP_REQUEST_CODE_PUT) {
        // PUT - setează starea nouă
        size_t payload_len;
        const uint8_t *payload;
        
        if (coap_get_data(request, &payload_len, &payload) && payload_len > 0) {
            // Convertește payload la string
            char command[16] = {0};
            size_t cmd_len = (payload_len < sizeof(command) - 1) ? payload_len : sizeof(command) - 1;
            memcpy(command, payload, cmd_len);
            command[cmd_len] = '\0';
            
            ESP_LOGI(TAG, "📥 PUT %s -> %s", device_name, command);
            
            // Validează comanda
            if (strcmp(command, "on") == 0) {
                set_device_state(device_name, true);
                coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);
                coap_add_data(response, 2, (const uint8_t*)"on");
                ESP_LOGI(TAG, "✅ Device %s turned ON", device_name);
                
            } else if (strcmp(command, "off") == 0) {
                set_device_state(device_name, false);
                coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);
                coap_add_data(response, 3, (const uint8_t*)"off");
                ESP_LOGI(TAG, "✅ Device %s turned OFF", device_name);
                
            } else {
                ESP_LOGW(TAG, "⚠️ Invalid command: %s", command);
                coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
                coap_add_data(response, 13, (const uint8_t*)"Invalid state");
            }
        } else {
            ESP_LOGW(TAG, "⚠️ Empty payload in PUT request");
            coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
            coap_add_data(response, 12, (const uint8_t*)"Empty payload");
        }
    } else {
        ESP_LOGW(TAG, "⚠️ Unsupported method for device control");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_NOT_ALLOWED);  // Fixat numele constantei
    }
}

esp_err_t init_coap_server(void) {
    ESP_LOGI(TAG, "🏠 Initializing CoAP Server for device control...");
    
    if (!coap_context) {
        ESP_LOGE(TAG, "❌ CoAP context not initialized");
        return ESP_FAIL;
    }
    
    // Crează adresa server
    coap_address_t server_addr;
    coap_address_init(&server_addr);
    server_addr.addr.sin.sin_family = AF_INET;
    server_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
    server_addr.addr.sin.sin_port = htons(COAP_SERVER_PORT);
    
    // Bind server endpoint
    coap_endpoint_t *endpoint = coap_new_endpoint(coap_context, &server_addr, COAP_PROTO_UDP);
    if (!endpoint) {
        ESP_LOGE(TAG, "❌ Failed to create CoAP server endpoint");
        return ESP_FAIL;
    }
    
    // Crează resurse pentru fiecare device
    
    // Kitchen light resource
    coap_resource_t *kitchen_resource = coap_resource_init(coap_make_str_const("kitchen"), 0);
    if (kitchen_resource) {
        coap_register_handler(kitchen_resource, COAP_REQUEST_GET, coap_device_handler);
        coap_register_handler(kitchen_resource, COAP_REQUEST_PUT, coap_device_handler);
        coap_add_resource(coap_context, kitchen_resource);
        ESP_LOGI(TAG, "✅ Kitchen resource created");
    }
    
    // Living room light resource
    coap_resource_t *living_resource = coap_resource_init(coap_make_str_const("living"), 0);
    if (living_resource) {
        coap_register_handler(living_resource, COAP_REQUEST_GET, coap_device_handler);
        coap_register_handler(living_resource, COAP_REQUEST_PUT, coap_device_handler);
        coap_add_resource(coap_context, living_resource);
        ESP_LOGI(TAG, "✅ Living resource created");
    }
    
    // Bedroom light resource
    coap_resource_t *bedroom_resource = coap_resource_init(coap_make_str_const("bedroom"), 0);
    if (bedroom_resource) {
        coap_register_handler(bedroom_resource, COAP_REQUEST_GET, coap_device_handler);
        coap_register_handler(bedroom_resource, COAP_REQUEST_PUT, coap_device_handler);
        coap_add_resource(coap_context, bedroom_resource);
        ESP_LOGI(TAG, "✅ Bedroom resource created");
    }
    
    // Cooler resource
    coap_resource_t *cooler_resource = coap_resource_init(coap_make_str_const("cooler"), 0);
    if (cooler_resource) {
        coap_register_handler(cooler_resource, COAP_REQUEST_GET, coap_device_handler);
        coap_register_handler(cooler_resource, COAP_REQUEST_PUT, coap_device_handler);
        coap_add_resource(coap_context, cooler_resource);
        ESP_LOGI(TAG, "✅ Cooler resource created");
    }
    
    ESP_LOGI(TAG, "✅ CoAP Server initialized on port %d", COAP_SERVER_PORT);
    ESP_LOGI(TAG, "📍 Available resources: /kitchen, /living, /bedroom, /cooler");
    
    return ESP_OK;
}

// =============================================================================
// TASK PENTRU CoAP SERVER
// =============================================================================

void coap_server_task(void *pvParameters) {
    ESP_LOGI(TAG, "🏠 CoAP Server Task started");
    
    TickType_t last_check = xTaskGetTickCount();
    const TickType_t check_interval = pdMS_TO_TICKS(100);  // Check la fiecare 100ms
    
    while (1) {
        if (coap_context) {
            // Procesează mesajele CoAP primite
            int result = coap_io_process(coap_context, 50);  // Timeout 50ms
            
            if (result < 0) {
                ESP_LOGW(TAG, "⚠️ CoAP IO process error: %d", result);
                gpio_set_level(LED_ERROR, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_set_level(LED_ERROR, 0);
            }
            
            // Cleanup periodic
            TickType_t now = xTaskGetTickCount();
            if ((now - last_check) >= check_interval) {
                coap_check_notify(coap_context);
                last_check = now;
            }
        }
        
        // Delay mic pentru a nu bloca task-ul
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// =============================================================================
// FUNCTII COAP COMPLETE - INIT GENERAL
// =============================================================================

esp_err_t init_coap_complete(void) {
    ESP_LOGI(TAG, "📡 Initializing complete CoAP functionality...");
    
    // Inițializare CoAP client
    if (init_coap_client() != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to initialize CoAP client");
        return ESP_FAIL;
    }
    
    // Inițializare CoAP server
    if (init_coap_server() != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to initialize CoAP server");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✅ Complete CoAP functionality initialized");
    ESP_LOGI(TAG, "📤 Client: Sends sensor data to %s:5683", RASPBERRY_PI_IP);
    ESP_LOGI(TAG, "📥 Server: Receives device commands on port 5683");
    
    return ESP_OK;
}

// =============================================================================
// WIFI - SIMPLIFICAT DIN CODUL ORIGINAL
// =============================================================================

static void wifi_event_handler_simple(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "📶 Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);
        gpio_set_level(LED_WIFI_STATUS, 1);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "📶 Station "MACSTR" left, AID=%d", MAC2STR(event->mac), event->aid);
        // Nu oprim LED-ul WiFi complet, dar îl clipim
        for (int i = 0; i < 3; i++) {
            gpio_set_level(LED_WIFI_STATUS, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(LED_WIFI_STATUS, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void wifi_init_simple(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler_simple,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                    .required = false,
            },
        },
    };
    
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "📶 WiFi AP started: %s", EXAMPLE_ESP_WIFI_SSID);
    ESP_LOGI(TAG, "📍 ESP32-C3 IP: 192.168.4.1");
    ESP_LOGI(TAG, "📍 Raspberry Pi should connect and get IP: 192.168.4.x");
}

// =============================================================================
// TASK-URI MODIFICATE - PARTEA 4 FINALĂ
// =============================================================================

void spi_slave_task(void *pvParameters) {
    ESP_LOGI(TAG, "📡 SPI Slave Task started - WITH CoAP INTEGRATION");
    
    // Inițializare buffer-e
    memset(spi_receive_buffer, 0, SPI_BUFFER_SIZE);
    memset(spi_send_buffer, 0, SPI_BUFFER_SIZE);
    
    int transaction_count = 0;
    
    while (1) {
        transaction_count++;
        
        // Pregătire tranzacție SPI
        spi_slave_transaction_t trans = {
            .length = SPI_BUFFER_SIZE * 8,  // Length în biți
            .rx_buffer = spi_receive_buffer,
            .tx_buffer = spi_send_buffer,
        };
        
        ESP_LOGI(TAG, "📡 Transaction %d - Waiting for SPI data...", transaction_count);
        
        // Așteaptă date de la master (ESP32-C2)
        esp_err_t ret = spi_slave_transmit(SPI2_HOST, &trans, portMAX_DELAY);
        
        if (ret == ESP_OK) {
            // Calculează lungimea reală a datelor primite
            int received_bytes = trans.length / 8;
            
            ESP_LOGI(TAG, "🎉 SPI DATA RECEIVED! (%d bytes)", received_bytes);
            
            // LED feedback
            gpio_set_level(LED_DATA_RECEIVED, 1);
            
            if (received_bytes > 0) {
                // Asigură-te că string-ul este null-terminated
                if (received_bytes < SPI_BUFFER_SIZE) {
                    spi_receive_buffer[received_bytes] = '\0';
                } else {
                    spi_receive_buffer[SPI_BUFFER_SIZE - 1] = '\0';
                }
                
                ESP_LOGI(TAG, "📋 Raw data: %s", (char*)spi_receive_buffer);
                
                // Trimite la queue pentru procesare
                if (xQueueSend(spi_data_queue, spi_receive_buffer, pdMS_TO_TICKS(100)) == pdTRUE) {
                    ESP_LOGI(TAG, "✅ Data sent to processing queue");
                } else {
                    ESP_LOGW(TAG, "❌ Processing queue full!");
                }
            }
            
            // LED OFF după 200ms
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(LED_DATA_RECEIVED, 0);
            
        } else {
            ESP_LOGE(TAG, "❌ SPI Slave error: %s", esp_err_to_name(ret));
            gpio_set_level(LED_ERROR, 1);
            vTaskDelay(pdMS_TO_TICKS(300));
            gpio_set_level(LED_ERROR, 0);
        }
        
        // Clear buffer pentru următoarea tranzacție
        memset(spi_receive_buffer, 0, SPI_BUFFER_SIZE);
        
        // Delay scurt pentru stabilitate
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void data_processing_task(void *pvParameters) {
    char received_data[SPI_BUFFER_SIZE];
    ESP_LOGI(TAG, "🔄 Data Processing Task started - WITH CoAP INTEGRATION");
    
    while (1) {
        // Așteaptă date din queue
        if (xQueueReceive(spi_data_queue, received_data, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "🔄 Processing received data...");
            
            // Verifică dacă datele par valide
            if (strlen(received_data) > 0) {
                // Verifică dacă este JSON
                if (strchr(received_data, '{') && strchr(received_data, '}')) {
                    ESP_LOGI(TAG, "✅ Valid JSON detected - parsing for CoAP transmission");
                    
                    // Parse datele senzorilor
                    sensor_data_t sensor_data = parse_sensor_json(received_data);
                    
                    // Trimite către CoAP client task pentru transmisie
                    if (xQueueSend(coap_send_queue, &sensor_data, pdMS_TO_TICKS(1000)) == pdTRUE) {
                        ESP_LOGI(TAG, "📤 Sensor data queued for CoAP transmission");
                    } else {
                        ESP_LOGW(TAG, "⚠️ CoAP send queue full!");
                    }
                    
                } else {
                    ESP_LOGW(TAG, "⚠️ Data received but not JSON format");
                    ESP_LOGI(TAG, "Raw: %s", received_data);
                }
            } else {
                ESP_LOGW(TAG, "⚠️ Empty data received");
            }
        }
    }
}

void system_status_task(void *pvParameters) {
    ESP_LOGI(TAG, "💚 System Status Task started - COMPLETE SYSTEM");
    
    int heartbeat_counter = 0;
    
    while (1) {
        heartbeat_counter++;
        
        // Heartbeat la fiecare 10 secunde
        ESP_LOGI(TAG, "💚 System alive #%d - SPI->CoAP Bridge ready", heartbeat_counter);
        ESP_LOGI(TAG, "📊 Device states: Kitchen:%s Living:%s Bedroom:%s Cooler:%s",
                 device_states.kitchen_light ? "ON" : "OFF",
                 device_states.living_light ? "ON" : "OFF", 
                 device_states.bedroom_light ? "ON" : "OFF",
                 device_states.cooler ? "ON" : "OFF");
        
        // Blink LED sistem
        gpio_set_level(LED_WIFI_STATUS, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_WIFI_STATUS, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_WIFI_STATUS, 1);
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// =============================================================================
// MAIN FUNCTION - COMPLETĂ CU TOATE COMPONENTELE
// =============================================================================

void app_main(void)
{
    ESP_LOGI(TAG, "🚀 === ESP32-C3 SPI->CoAP SMART HOME BRIDGE ===");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // ===== HARDWARE INITIALIZATION =====
    ESP_LOGI(TAG, "🔧 === HARDWARE INITIALIZATION ===");
    
    ESP_ERROR_CHECK(init_leds_simple());
    ESP_ERROR_CHECK(init_smart_home_devices());
    
    // Test LED-uri
    ESP_LOGI(TAG, "🔄 Testing all LEDs...");
    for (int i = 0; i < 2; i++) {
        // Status LEDs
        gpio_set_level(LED_WIFI_STATUS, 1);
        gpio_set_level(LED_DATA_RECEIVED, 1);
        gpio_set_level(LED_DATA_SEND, 1);
        gpio_set_level(LED_ERROR, 1);
        // Device LEDs
        gpio_set_level(KITCHEN_LIGHT_PIN, 1);
        gpio_set_level(LIVING_LIGHT_PIN, 1);
        gpio_set_level(BEDROOM_LIGHT_PIN, 1);
        gpio_set_level(COOLER_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(300));
        
        // All OFF
        gpio_set_level(LED_WIFI_STATUS, 0);
        gpio_set_level(LED_DATA_RECEIVED, 0);
        gpio_set_level(LED_DATA_SEND, 0);
        gpio_set_level(LED_ERROR, 0);
        gpio_set_level(KITCHEN_LIGHT_PIN, 0);
        gpio_set_level(LIVING_LIGHT_PIN, 0);
        gpio_set_level(BEDROOM_LIGHT_PIN, 0);
        gpio_set_level(COOLER_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    
    // ===== COMMUNICATION INITIALIZATION =====
    ESP_LOGI(TAG, "📡 === COMMUNICATION INITIALIZATION ===");
    
    // Initialize WiFi AP
    ESP_LOGI(TAG, "📶 Starting WiFi AP...");
    wifi_init_simple();
    vTaskDelay(pdMS_TO_TICKS(2000));  // Așteaptă WiFi să se stabilizeze
    
    // Initialize SPI Slave
    ESP_LOGI(TAG, "📡 Starting SPI Slave...");
    ESP_ERROR_CHECK(init_spi_slave_fixed());
    
    // Initialize CoAP (Client + Server)
    ESP_LOGI(TAG, "🌐 Starting CoAP...");
    ESP_ERROR_CHECK(init_coap_complete());
    
    // ===== QUEUES CREATION =====
    ESP_LOGI(TAG, "📦 === CREATING QUEUES ===");
    
    spi_data_queue = xQueueCreate(10, SPI_BUFFER_SIZE);
    if (spi_data_queue == NULL) {
        ESP_LOGE(TAG, "❌ Failed to create SPI data queue!");
        return;
    }
    
    coap_send_queue = xQueueCreate(5, sizeof(sensor_data_t));
    if (coap_send_queue == NULL) {
        ESP_LOGE(TAG, "❌ Failed to create CoAP send queue!");
        return;
    }
    
    // ===== TASKS CREATION =====
    ESP_LOGI(TAG, "⚙️ === CREATING TASKS ===");
    
    // SPI communication tasks
    xTaskCreate(spi_slave_task, "spi_slave", 6144, NULL, 6, NULL);
    xTaskCreate(data_processing_task, "data_proc", 4096, NULL, 4, NULL);
    
    // CoAP communication tasks  
    xTaskCreate(coap_client_task, "coap_client", 8192, NULL, 5, NULL);
    xTaskCreate(coap_server_task, "coap_server", 8192, NULL, 5, NULL);
    
    // System monitoring
    xTaskCreate(system_status_task, "status", 3072, NULL, 3, NULL);
    
    // ===== SYSTEM READY =====
    ESP_LOGI(TAG, "✅ === SYSTEM READY ===");
    ESP_LOGI(TAG, "🔗 === COMMUNICATION PATHS ===");
    ESP_LOGI(TAG, "📥 ESP32-C2 → SPI → ESP32-C3");
    ESP_LOGI(TAG, "📤 ESP32-C3 → CoAP Client → Raspberry Pi");
    ESP_LOGI(TAG, "📲 Raspberry Pi → CoAP → ESP32-C3 → Device Control");
    ESP_LOGI(TAG, "🌐 === NETWORK INFO ===");
    ESP_LOGI(TAG, "📍 ESP32-C3 AP: %s (192.168.4.1)", EXAMPLE_ESP_WIFI_SSID);
    ESP_LOGI(TAG, "📍 Raspberry Pi IP: %s", RASPBERRY_PI_IP);
    ESP_LOGI(TAG, "📍 CoAP Server Port: %d", COAP_SERVER_PORT);
    ESP_LOGI(TAG, "🏠 === SMART HOME DEVICES ===");
    ESP_LOGI(TAG, "💡 Kitchen Light: GPIO%d", KITCHEN_LIGHT_PIN);
    ESP_LOGI(TAG, "💡 Living Light: GPIO%d", LIVING_LIGHT_PIN);
    ESP_LOGI(TAG, "💡 Bedroom Light: GPIO%d", BEDROOM_LIGHT_PIN);
    ESP_LOGI(TAG, "❄️ Cooler: GPIO%d", COOLER_PIN);
    ESP_LOGI(TAG, "🚀 === BRIDGE ACTIVE - READY FOR OPERATION ===");
    
    // Start system status indicator
    gpio_set_level(LED_WIFI_STATUS, 1);
}