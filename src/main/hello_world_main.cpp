/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <iostream> // cin, cout
#include <string.h> /* memset */
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" // QueueHandle_t
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"

#include "led/led_strip_encoder.h"
#include "McCoolLEDs.hpp"


//#include "McCoolLEDs.h"


#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      21
#define GPIO_INPUT_BUTTON_SW2       (gpio_num_t)17
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_BUTTON_SW2))

#define EXAMPLE_LED_NUMBERS         35
#define EXAMPLE_CHASE_SPEED_MS      10
#define ESP_INTR_FLAG_DEFAULT       0
#define NUM_PATTERNS                3
static const char *TAG = "example";

#include "esp_check.h"

extern "C" {
    void app_main();
}

static int loop_state = 0;


static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

void glow_random(rmt_channel_handle_t channel, rmt_encoder_t *encoder, const rmt_transmit_config_t *config){
    //uint16_t speed_ms = 100;
    McCoolLEDClump clump(channel, encoder, config, 35);
    ESP_LOGI(TAG, "Start LED Glow Random with CLUMP");
    while (1) {
        //std::cout << "clump.glow_random()" << std::endl;
        clump.glow_random_2(10, 10, 100); // Flush RGB values to LEDs
        //vTaskDelay(pdMS_TO_TICKS(speed_ms));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void pattern_loop(rmt_channel_handle_t channel, rmt_encoder_t *encoder, const rmt_transmit_config_t *config){
    //uint16_t speed_ms = 100;
    McCoolLEDClump clump(channel, encoder, config, 35);
    ESP_LOGI(TAG, "Starting Pattern Loop with CLUMP");

    //int pattern = 1;
    while (1) {
        switch(loop_state){
            case 3:
                clump.glow_random_2(10, 10, 100);
                vTaskDelay(pdMS_TO_TICKS(100)); // 0 for no delay, still needed to reschedule
                break;
            case 1:
                clump.rainbow_chase();
                vTaskDelay(pdMS_TO_TICKS(0)); // 0 for no delay, still needed to reschedule
                break;
            case 2:
                clump.glow_as_one(35, 10, 100);
                vTaskDelay(pdMS_TO_TICKS(100)); // 0 for no delay, still needed to reschedule
                break;
            case 0:
                clump.chase(1, 3);
                vTaskDelay(pdMS_TO_TICKS(40)); // 0 for no delay, still needed to reschedule
                break;
        }
        
    }
}

//// GPIO
static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            if (gpio_get_level((gpio_num_t)io_num) == 1){
                loop_state += 1;
                if (loop_state > NUM_PATTERNS){
                    loop_state = 0;
                }
            }
            std::cout << "GPIO[" << io_num << "] interrupt, value: " << gpio_get_level((gpio_num_t)io_num) << ",  loop_state = " << loop_state << std::endl;
        }
    }
}

void setup_gpio_inputs(uint64_t pin_sel){
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins
    io_conf.pin_bit_mask = pin_sel;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //disable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    //disable pull-down mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_BUTTON_SW2, gpio_isr_handler, (void*) GPIO_INPUT_BUTTON_SW2);
    //remove isr handler for gpio number.
    //gpio_isr_handler_remove(GPIO_INPUT_IO_0);
}

/// END GPIO

void app_main(void)
{

    ESP_LOGI(TAG, "Setting up GPIO");
    setup_gpio_inputs(GPIO_INPUT_PIN_SEL);

    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_channel_handle_t led_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
        .flags = { 0, 0, 0, 0 }
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");
    rmt_encoder_handle_t led_encoder = NULL;
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
        .flags = { 0 }
    };

    //chase(led_chan, led_encoder, &tx_config);
    //line(led_chan, led_encoder, &tx_config);
    //glow_as_one(led_chan, led_encoder, &tx_config);
    //glow_random(led_chan, led_encoder, &tx_config);
    pattern_loop(led_chan, led_encoder, &tx_config);

    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%uMB %s flash\n", flash_size / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
