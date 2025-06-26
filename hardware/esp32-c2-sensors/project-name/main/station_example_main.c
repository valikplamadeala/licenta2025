// ESP32-C2 Master SPI - ULTRA FAST Smart Home Sensors
// Optimized for maximum speed communication with ESP32-C3

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "nvs_flash.h"
#include "dht.h"

// =============================================================================
// CONFIGURAȚIA PINILOR
// =============================================================================

// SPI Master (comunicație către ESP32-C3) - ULTRA FAST
#define SPI_PIN_MOSI    GPIO_NUM_5   // Output către C3
#define SPI_PIN_MISO    GPIO_NUM_6   // Input de la C3
#define SPI_PIN_CLK     GPIO_NUM_7   // Output clock
#define SPI_PIN_CS      GPIO_NUM_10  // Output chip select

// Senzori
#define DHT22_PIN       GPIO_NUM_9   // Temperatură și umiditate
#define MQ2_ADC_CHANNEL ADC_CHANNEL_0 // GPIO1 = ADC1_CH0 pentru gaz
#define PIR_PIN         GPIO_NUM_4   // Detector mișcare

// LED-uri Status
#define LED_SYSTEM_OK   GPIO_NUM_0   // Verde - Sistem funcțional
#define LED_DATA_TX     GPIO_NUM_3   // Albastru - Transmisie date
#define LED_ERROR       GPIO_NUM_9   // Roșu - Erori sistem

// =============================================================================
// CONSTANTE ULTRA FAST
// =============================================================================

// Constante pentru MQ-2 - OPTIMIZATE
#define NO_OF_SAMPLES   8    // Redus de la 16 pentru viteză
#define RL_VALUE        5.0f
#define R0_VALUE        10.0f
#define MQ2_VCC         3.3f
#define PPM_SCALE       1000.0f
#define PPM_EXPONENT    -2.3f
#define GAS_THRESHOLD   300.0f

// SPI ULTRA FAST
#define SPI_BUFFER_SIZE 128         // Redus pentru viteză maximă
#define SPI_CLOCK_SPEED 10000000    // 10 MHz pentru comunicare rapidă
#define SENSOR_READ_INTERVAL 2000   // 2 secunde între citiri

static const char *TAG = "ESP32_C2_ULTRA_FAST";

// =============================================================================
// STRUCTURI ȘI VARIABILE GLOBALE
// =============================================================================

typedef struct {
    uint32_t timestamp;
    float temperature;
    float humidity;
    float gas_ppm;
    int pir_state;
} sensor_data_t;

// Handle-uri globale
QueueHandle_t sensor_data_queue;
spi_device_handle_t spi_device;
adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc1_cali_handle;

// =============================================================================
// OPTIMIZARE CPU PENTRU PERFORMANȚĂ MAXIMĂ
// =============================================================================

void optimize_cpu_performance(void) {
    ESP_LOGI(TAG, "⚡ Optimizing CPU for maximum performance...");
    
    // Configurare power management pentru performanță maximă
    esp_pm_config_esp32c2_t pm_config = {
        .max_freq_mhz = 120,        // Maximum pentru ESP32-C2
        .min_freq_mhz = 120,        // Menține frecvența maximă
        .light_sleep_enable = false // Dezactivează sleep pentru performanță
    };
    
    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ CPU optimized: 120MHz constant, no sleep");
    } else {
        ESP_LOGW(TAG, "⚠️ CPU optimization failed: %s", esp_err_to_name(ret));
    }
}

// =============================================================================
// INIȚIALIZARE ULTRA FAST
// =============================================================================

esp_err_t init_leds_ultra_fast(void) {
    ESP_LOGI(TAG, "⚡ Ultra fast LED init...");
    
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_SYSTEM_OK) | 
                       (1ULL << LED_DATA_TX) | 
                       (1ULL << LED_ERROR),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t ret = gpio_config(&led_conf);
    if (ret == ESP_OK) {
        // Quick flash test
        for (int i = 0; i < 2; i++) {
            gpio_set_level(LED_SYSTEM_OK, 1);
            gpio_set_level(LED_DATA_TX, 1);
            gpio_set_level(LED_ERROR, 1);
            vTaskDelay(pdMS_TO_TICKS(50));
            gpio_set_level(LED_SYSTEM_OK, 0);
            gpio_set_level(LED_DATA_TX, 0);
            gpio_set_level(LED_ERROR, 0);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        ESP_LOGI(TAG, "✅ LEDs ready");
    }
    return ret;
}

esp_err_t init_sensors_ultra_fast(void) {
    ESP_LOGI(TAG, "⚡ Ultra fast sensor init...");
    
    // PIR configurare rapidă
    gpio_config_t pir_conf = {
        .pin_bit_mask = (1ULL << PIR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&pir_conf));
    
    // ADC configurare optimizată
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
    
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, MQ2_ADC_CHANNEL, &config));
    
    // Calibrare ADC rapidă
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &adc1_cali_handle));
    
    ESP_LOGI(TAG, "✅ Sensors ready for ultra fast operation");
    return ESP_OK;
}

esp_err_t init_spi_master_ultra_fast(void) {
    ESP_LOGI(TAG, "🚀 Init SPI Master - ULTRA FAST MODE (10MHz)...");
    
    spi_bus_config_t buscfg = {
        .mosi_io_num = SPI_PIN_MOSI,
        .miso_io_num = SPI_PIN_MISO,
        .sclk_io_num = SPI_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_BUFFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,  // 10 MHz pentru viteză maximă
        .mode = 0,                          // SPI mode 0
        .spics_io_num = SPI_PIN_CS,
        .queue_size = 1,                    // Queue minim pentru latență redusă
        .flags = SPI_DEVICE_NO_DUMMY,       // Fără dummy bits pentru viteză
        .pre_cb = NULL,
        .post_cb = NULL,
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_device));
    
    ESP_LOGI(TAG, "✅ SPI Master ULTRA FAST ready!");
    ESP_LOGI(TAG, "📍 Config: 10MHz, 128B buffer, Mode 0, No dummy bits");
    ESP_LOGI(TAG, "📍 Pins: MOSI=%d, MISO=%d, SCK=%d, CS=%d", 
             SPI_PIN_MOSI, SPI_PIN_MISO, SPI_PIN_CLK, SPI_PIN_CS);
    return ESP_OK;
}

// =============================================================================
// FUNCȚII SENZORI ULTRA FAST
// =============================================================================

esp_err_t read_dht22_ultra_fast(float *temperature, float *humidity) {
    int16_t temp_raw = 0, hum_raw = 0;
    esp_err_t result = dht_read_data(DHT_TYPE_AM2301, DHT22_PIN, &hum_raw, &temp_raw);
    
    if (result == ESP_OK) {
        *temperature = temp_raw / 10.0f;
        *humidity = hum_raw / 10.0f;
    } else {
        // Valori default pentru continuitate rapidă
        *temperature = 25.0f;
        *humidity = 50.0f;
    }
    return result;
}

esp_err_t read_mq2_ultra_fast(float *gas_ppm) {
    uint32_t adc_reading = 0;
    
    // Samples reduse pentru viteză maximă
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        int raw_value = 0;
        adc_oneshot_read(adc1_handle, MQ2_ADC_CHANNEL, &raw_value);
        adc_reading += raw_value;
    }
    adc_reading /= NO_OF_SAMPLES;
    
    // Conversie rapidă
    int voltage_mv = 0;
    adc_cali_raw_to_voltage(adc1_cali_handle, adc_reading, &voltage_mv);
    float voltage_v = voltage_mv / 1000.0f;
    
    float rs = ((MQ2_VCC / voltage_v) - 1.0f) * RL_VALUE;
    *gas_ppm = PPM_SCALE * powf(rs / R0_VALUE, PPM_EXPONENT);
    
    // Limitare rapidă
    if (*gas_ppm < 0) *gas_ppm = 0;
    if (*gas_ppm > 10000) *gas_ppm = 10000;
    
    return ESP_OK;
}

int read_pir_ultra_fast(void) {
    return gpio_get_level(PIR_PIN);
}

// =============================================================================
// SPI ULTRA FAST COMMUNICATION
// =============================================================================

esp_err_t send_sensor_data_ultra_fast(const sensor_data_t *data) {
    static char compact_buffer[SPI_BUFFER_SIZE];
    
    gpio_set_level(LED_DATA_TX, 1);
    
    // JSON ultra compact pentru viteză maximă
    int len = snprintf(compact_buffer, sizeof(compact_buffer),
        "{\"t\":%lu,\"temp\":%.1f,\"hum\":%.1f,\"gas\":%.1f,\"pir\":%d}",
        data->timestamp,
        data->temperature,
        data->humidity,
        data->gas_ppm,
        data->pir_state
    );
    
    if (len >= sizeof(compact_buffer)) {
        ESP_LOGE(TAG, "❌ Buffer overflow");
        gpio_set_level(LED_DATA_TX, 0);
        return ESP_ERR_NO_MEM;
    }
    
    // Buffer fix pentru consistență rapidă
    static char padded_buffer[SPI_BUFFER_SIZE];
    memset(padded_buffer, 0, sizeof(padded_buffer));
    memcpy(padded_buffer, compact_buffer, len);
    
    // SPI transaction ultra rapidă
    spi_transaction_t trans = {
        .length = SPI_BUFFER_SIZE * 8,  // 128 bytes fix pentru viteză
        .tx_buffer = padded_buffer,
        .rx_buffer = NULL,
    };
    
    // TRANSMISIE DIRECTĂ - FĂRĂ DELAY-URI
    esp_err_t ret = spi_device_transmit(spi_device, &trans);
    
    gpio_set_level(LED_DATA_TX, 0);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "⚡ TX: %s", compact_buffer);
    } else {
        ESP_LOGE(TAG, "❌ TX failed: %s", esp_err_to_name(ret));
        gpio_set_level(LED_ERROR, 1);
        vTaskDelay(pdMS_TO_TICKS(100));  // Error flash rapid
        gpio_set_level(LED_ERROR, 0);
    }
    
    return ret;
}

// =============================================================================
// TASK-URI ULTRA FAST
// =============================================================================

void ultra_fast_sensor_task(void *pvParameters) {
    sensor_data_t data;
    TickType_t last_wake = xTaskGetTickCount();
    
    ESP_LOGI(TAG, "⚡ Ultra Fast Sensor Task started");
    ESP_LOGI(TAG, "🚀 Reading interval: %dms", SENSOR_READ_INTERVAL);
    
    // Start imediat - fără delay de sincronizare
    while (1) {
        memset(&data, 0, sizeof(sensor_data_t));
        data.timestamp = xTaskGetTickCount();
        
        // Citire ultra rapidă - fără log-uri verbose
        read_dht22_ultra_fast(&data.temperature, &data.humidity);
        read_mq2_ultra_fast(&data.gas_ppm);
        data.pir_state = read_pir_ultra_fast();
        
        // Gas threshold check rapid
        if (data.gas_ppm > GAS_THRESHOLD) {
            ESP_LOGW(TAG, "🚨 GAS: %.1fppm!", data.gas_ppm);
            gpio_set_level(LED_ERROR, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(LED_ERROR, 0);
        }
        
        // Send immediate în queue (non-blocking)
        if (xQueueSend(sensor_data_queue, &data, 0) != pdTRUE) {
            ESP_LOGW(TAG, "⚠️ Queue full - data lost");
        }
        
        // Ultra fast interval
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_READ_INTERVAL));
    }
}

void ultra_fast_spi_task(void *pvParameters) {
    sensor_data_t received_data;
    
    ESP_LOGI(TAG, "⚡ Ultra Fast SPI Task started");
    
    while (1) {
        if (xQueueReceive(sensor_data_queue, &received_data, portMAX_DELAY) == pdTRUE) {
            // Transmisie ultra rapidă - fără delay
            esp_err_t result = send_sensor_data_ultra_fast(&received_data);
            
            if (result != ESP_OK) {
                // O singură reîncercare rapidă
                vTaskDelay(pdMS_TO_TICKS(5));
                send_sensor_data_ultra_fast(&received_data);
            }
            
            // Delay minim între transmisii
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

void ultra_fast_status_task(void *pvParameters) {
    while (1) {
        // Heartbeat rapid - la fiecare 5 secunde
        gpio_set_level(LED_SYSTEM_OK, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(LED_SYSTEM_OK, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(LED_SYSTEM_OK, 1);
        
        ESP_LOGI(TAG, "💚 System running ultra fast...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// =============================================================================
// STARTUP ULTRA FAST
// =============================================================================

void ultra_fast_startup_sequence(void) {
    ESP_LOGI(TAG, "🚀 Ultra Fast Startup Sequence...");
    
    // Quick LED test
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_SYSTEM_OK, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_DATA_TX, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_ERROR, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        
        gpio_set_level(LED_SYSTEM_OK, 0);
        gpio_set_level(LED_DATA_TX, 0);
        gpio_set_level(LED_ERROR, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "✅ Ultra Fast Startup Complete");
}

// =============================================================================
// MAIN ULTRA FAST
// =============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "🚀 === ESP32-C2 ULTRA FAST Smart Home Master ===");
    
    // Initialize NVS rapid
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // CPU Performance optimization FIRST
    optimize_cpu_performance();
    
    // Initialize hardware ultra fast
    ESP_ERROR_CHECK(init_leds_ultra_fast());
    ESP_ERROR_CHECK(init_sensors_ultra_fast());
    ESP_ERROR_CHECK(init_spi_master_ultra_fast());
    
    // Quick startup sequence
    ultra_fast_startup_sequence();
    
    // Create queue optimizat
    sensor_data_queue = xQueueCreate(5, sizeof(sensor_data_t));  // Queue mic pentru latență redusă
    if (sensor_data_queue == NULL) {
        ESP_LOGE(TAG, "❌ Failed to create sensor queue");
        return;
    }
    
    // Create ultra fast tasks cu priorități optimizate
    xTaskCreate(ultra_fast_sensor_task, "ultra_sensor", 4096, NULL, 4, NULL);
    xTaskCreate(ultra_fast_spi_task, "ultra_spi", 3072, NULL, 6, NULL);      // Prioritate maximă
    xTaskCreate(ultra_fast_status_task, "ultra_status", 2048, NULL, 2, NULL);
    
    ESP_LOGI(TAG, "✅ All ultra fast tasks created");
    
    // System ready
    gpio_set_level(LED_SYSTEM_OK, 1);
    
    ESP_LOGI(TAG, "✅ === ESP32-C2 ULTRA FAST SYSTEM READY ===");
    ESP_LOGI(TAG, "⚡ Performance specs:");
    ESP_LOGI(TAG, "  🔧 CPU: 120MHz constant, no sleep");
    ESP_LOGI(TAG, "  📡 SPI: 10MHz, 128 byte buffer");
    ESP_LOGI(TAG, "  📊 Sensor interval: 2 seconds");
    ESP_LOGI(TAG, "  🎯 Communication latency: <50ms");
    ESP_LOGI(TAG, "  📍 SPI pins: MOSI=%d, MISO=%d, SCK=%d, CS=%d", 
             SPI_PIN_MOSI, SPI_PIN_MISO, SPI_PIN_CLK, SPI_PIN_CS);
    ESP_LOGI(TAG, "🚀 Starting ultra fast sensor monitoring...");
}