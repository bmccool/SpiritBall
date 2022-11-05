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
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/rmt_tx.h"

#include "led/led_strip_encoder.h"
#include "McCoolLEDs.hpp"


//#include "McCoolLEDs.h"

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      21

#define EXAMPLE_LED_NUMBERS         35
#define EXAMPLE_CHASE_SPEED_MS      10
static const char *TAG = "example";

#include "esp_check.h"

extern "C" {
    void app_main();
}


static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

void chase(rmt_channel_handle_t channel, rmt_encoder_t *encoder, const rmt_transmit_config_t *config){
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;
    while (1) {
        for (int i = 0; i < 3; i++) {
            for (int j = i; j < EXAMPLE_LED_NUMBERS; j += 3) {
                // Build RGB pixels
                hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
                led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
                led_strip_pixels[j * 3 + 0] = green;
                led_strip_pixels[j * 3 + 1] = blue;
                led_strip_pixels[j * 3 + 2] = red;
            }
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
            memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
            ESP_ERROR_CHECK(rmt_transmit(channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }
        start_rgb += 60;
    }
}

void line(rmt_channel_handle_t channel, rmt_encoder_t *encoder, const rmt_transmit_config_t *config){
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;
    uint16_t speed_ms = 7;
    while (1) {
        for (int j = 0; j < EXAMPLE_LED_NUMBERS; j += 1) {
            // Build RGB pixels
            hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
            led_strip_hsv2rgb(hue, 100, 1, &red, &green, &blue);
            led_strip_pixels[j * 3 + 0] = green;
            led_strip_pixels[j * 3 + 1] = blue;
            led_strip_pixels[j * 3 + 2] = red;
        }
        // Flush RGB values to LEDs
        ESP_ERROR_CHECK(rmt_transmit(channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
        vTaskDelay(pdMS_TO_TICKS(speed_ms));
        memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
        //ESP_ERROR_CHECK(rmt_transmit(channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
        //vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        start_rgb += 1;
    }
}

void glow(rmt_channel_handle_t channel, rmt_encoder_t *encoder, const rmt_transmit_config_t *config){
    // Each LED is it's own thing with it's own lifespan and hue
    // When an LED is "born", it derives its hue from the hue-meister
    // When an LED "dies", it is immediately reborn
    // LEDs can also have a "trajectory" that describes how fast and in what direction the hue can change over its lifetime
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;
    //uint16_t speed_ms = 7;
    while (1) {
        for (int j = 0; j < EXAMPLE_LED_NUMBERS; j += 1) {
            // Build RGB pixels
            hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
            led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
            led_strip_pixels[j * 3 + 0] = green;
            led_strip_pixels[j * 3 + 1] = blue;
            led_strip_pixels[j * 3 + 2] = red;
        }
        // Flush RGB values to LEDs
        ESP_ERROR_CHECK(rmt_transmit(channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
        
        memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
        //ESP_ERROR_CHECK(rmt_transmit(channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
        //vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        start_rgb += 1;
    }
}

void glow_as_one(rmt_channel_handle_t channel, rmt_encoder_t *encoder, const rmt_transmit_config_t *config){
    McCoolLED led(0, 100, 0, 10, 0, 0);
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t speed_ms = 100;
    while (1) {
        led_strip_hsv2rgb(led.hue, led.saturation, led.value, &red, &green, &blue);
        for (int j = 0; j < EXAMPLE_LED_NUMBERS; j += 1) {
            led_strip_pixels[j * 3 + 0] = green;
            led_strip_pixels[j * 3 + 1] = blue;
            led_strip_pixels[j * 3 + 2] = red;
        }
        // Flush RGB values to LEDs
        ESP_ERROR_CHECK(rmt_transmit(channel, encoder, led_strip_pixels, sizeof(led_strip_pixels), config));
        vTaskDelay(pdMS_TO_TICKS(speed_ms));
        memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
        led.glow();
        //std::cout << "led value: " << led.value << " ticks: " << led.tl_ticks << "/" << led.ttl_ticks << " is_alive: " << led.is_alive << std::endl;
        if (led.is_alive == false){
            led.tl_ticks = 0;
            led.is_alive = true;
            led.hue = led.hue + 20;
            led.value = 0;
        }
    }
}

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



void app_main(void)
{


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
    glow_random(led_chan, led_encoder, &tx_config);

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
