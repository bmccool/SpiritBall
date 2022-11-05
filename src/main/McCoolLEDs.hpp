#pragma once
#include <stdint.h>
#include <cstdlib> // rand
#include <vector> // std::vector
#include <string.h> /* memset */
#include <iostream> // cin, cout
#include "driver/rmt_tx.h" // TODO/ I'd rather this file not know about the driver implementation...

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

class McCoolLED {
    private:
    public:
        uint16_t hue;
        uint16_t saturation;
        uint16_t value;

        uint32_t red = 0;
        uint32_t green = 0;
        uint32_t blue = 0;

        uint16_t ttl_ticks;
        uint16_t tl_ticks;
        int16_t hue_trajectory;
        bool is_alive;

        uint16_t position;
        // xyz?

        McCoolLED(uint16_t hue, uint16_t saturation, uint16_t value, uint16_t ttl_ticks, int16_t hue_trajectory, uint16_t position): hue(hue), saturation(saturation), value(value), ttl_ticks(ttl_ticks), hue_trajectory(hue_trajectory), position(position){ is_alive = true; tl_ticks = 0;}

        bool glow(){
            if(is_alive == false){ return(false); }
            if(tl_ticks < (ttl_ticks / 2)){
                // Glowing Brighter
                value += 1;
            }
            else{
                // Glowing Dimmer
                value -= 1;
            }
            tl_ticks += 1;

            if(tl_ticks > ttl_ticks){
                is_alive = false;
            }
            return(true);
        }
};

class McCoolLEDClump {
    private:
    public:
        // Some concept of sp
        int x;
        int y;
        int z;

        // RMT only for now
        rmt_channel_handle_t channel;
        rmt_encoder_t *encoder;
        const rmt_transmit_config_t *config;

        int num_pixels;
        uint8_t* led_strip_pixels;

        std::vector<McCoolLED> leds;

        McCoolLEDClump(rmt_channel_handle_t channel, rmt_encoder_t *encoder, const rmt_transmit_config_t *config, int num_leds): channel(channel), encoder(encoder), config(config), num_pixels(num_leds){ led_strip_pixels = new uint8_t[num_leds * 3]; }

        void clear_strip(){
            // Dumb for now, just get it done
            for(int i = 0; i < num_pixels * 3; i++){
                led_strip_pixels[i] = 0;
            }
        }

        void dump_pixels(){
            // Dumb for now, just get it done
            for(int i = 0; i < num_pixels * 3; i++){
                std::cout << +led_strip_pixels[i] << " ";
            }
            std:: cout << std::endl;
        }

        unsigned int transmit_time(){
            return (0);
        }

        void glow_random(int concurrent_leds, int max_value, int max_ttl)
        {
            
            uint32_t red = 0;
            uint32_t green = 0;
            uint32_t blue = 0;
            //uint16_t speed_ms = 100;
            // TODO speed is how long we wait after sending LED data before sending it again.
            // Instead of blocking in this function, return the recommended time to wait so the caller can do it

            //for (int i = 0; i < concurrent_leds; i++){
            //    leds.emplace_back();
            //}
            clear_strip();
            if (leds.size() < 1){
                //std::cout << "Creating LED" << std::endl;
                leds.emplace_back(0, 100, 0, 10, 0, 0);
            }
            led_strip_hsv2rgb(leds[0].hue, leds[0].saturation, leds[0].value, &red, &green, &blue);
            //std::cout << red << " " << green << " " << blue << ":::" << leds[0].hue << " " << leds[0].saturation << " " << leds[0].value << std::endl;
            //std::cout << leds[0].is_alive << " " << leds[0].tl_ticks << " " << leds[0].hue << ":::" << leds[0].value << std::endl;
            //std::cout << "Num Pixels: " << num_pixels << std::endl;
            
            for (int j = 0; j < num_pixels; j += 1) {
                led_strip_pixels[j * 3 + 0] = green;
                led_strip_pixels[j * 3 + 1] = blue;
                led_strip_pixels[j * 3 + 2] = red;
            }
            // Flush RGB values to LEDs
            //dump_pixels();
            ESP_ERROR_CHECK(rmt_transmit(channel, encoder, led_strip_pixels, 105, config));

            leds[0].glow();
            if (leds[0].is_alive == false){
                leds[0].tl_ticks = 0;
                leds[0].is_alive = true;
                leds[0].hue = leds[0].hue + 20;
                leds[0].value = 0;
            }
        }

        void load_strip(){
            uint32_t red = 0;
            uint32_t green = 0;
            uint32_t blue = 0;
            for (int i = 0; i < leds.size(); i++){
                led_strip_hsv2rgb(leds[i].hue, leds[i].saturation, leds[i].value, &red, &green, &blue);
                led_strip_pixels[leds[i].position * 3 + 0] = green;
                led_strip_pixels[leds[i].position * 3 + 1] = blue;
                led_strip_pixels[leds[i].position * 3 + 2] = red;
            }
        }

        void glow_random_2(int concurrent_leds, int max_value, int max_ttl)
        {
            // TODO speed is how long we wait after sending LED data before sending it again.
            // Instead of blocking in this function, return the recommended time to wait so the caller can do it

            //for (int i = 0; i < concurrent_leds; i++){
            //    leds.emplace_back();
            //}
            uint16_t position;
            if (leds.size() == 0){
                position = rand() % 35;
                leds.emplace_back(0, 100, 0, 10, 0, position);
            }else if (leds.size() < concurrent_leds){
                if (rand() % 2 == 1){
                    position = rand() % 35;
                    leds.emplace_back(rand() % 360, 0, 0, 10, 0, position); // TODO check if it exists first
                }
            }

            clear_strip();
            load_strip();
        
            // Flush RGB values to LEDs
            //dump_pixels();
            ESP_ERROR_CHECK(rmt_transmit(channel, encoder, led_strip_pixels, 105, config));

            for (int i = 0; i < leds.size(); i++){
                leds[i].glow();
            }
            for (int i = 0; i < leds.size(); i++){
                if (leds[i].is_alive == false){
                    leds.erase(leds.begin() + i); // TODO won't this arrange the array while I'm working on it?  Find a better way to to do this.
                }
            }




        }
        
};