#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "functions.h"

extern int SPI_DISP_SCK;
extern int SPI_DISP_CSn;
extern int SPI_DISP_TX;
// spi0
extern int ADC_CH5;
extern score;
extern time_left;
extern highscore;
extern GameSpeed game_speed;

const uint btns[] = {5, 11, 5};
const uint leds[] = {6, 12, 10};
#define NUM_MOLES 3

#define DEBOUNCE_MS 200
#define GAME_DURATION_MS 30000

volatile int last_button_pressed = -1;
volatile bool hit_registered = false;
uint32_t last_interrupt_time = 0;
#define BUZZER_PIN 26
 
static void play_tone(uint32_t freq_hz, uint32_t duration_ms) {
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    if (freq_hz == 0) {
        pwm_set_enabled(slice, false);
        sleep_ms(duration_ms);
        pwm_set_enabled(slice, true);
        return;
    }
    uint32_t sys_clk = clock_get_hz(clk_sys);
    uint32_t div = 1;
    uint32_t wrap = sys_clk / freq_hz - 1;
    while (wrap > 65535 && div < 256) { div++; wrap = sys_clk / (div * freq_hz) - 1; }
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&cfg, div);
    pwm_config_set_wrap(&cfg, (uint16_t)wrap);
    pwm_init(slice, &cfg, true);
    pwm_set_gpio_level(BUZZER_PIN, (uint16_t)(wrap / 2));
    sleep_ms(duration_ms);
    pwm_set_gpio_level(BUZZER_PIN, 0);
}
 
static void play_correct_sound(void) {
    play_tone(784,  80);
    play_tone(1047, 120);
    play_tone(0,    30);
    play_tone(1319, 160);
}
 
static void play_wrong_sound(void) {
    play_tone(330, 150);
    play_tone(0,    40);
    play_tone(220, 200);
}

void gpio_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    if (!(events & GPIO_IRQ_EDGE_FALL)) return;
    if (current_time - last_interrupt_time < DEBOUNCE_MS) return;

    last_button_pressed = gpio;
    hit_registered = true;
    last_interrupt_time = current_time;
}

void setup_hardware() {
    stdio_init_all();

    for(int i = 0; i < NUM_MOLES; i++) {
        gpio_init(leds[i]);
        gpio_set_dir(leds[i], GPIO_OUT);
        gpio_put(leds[i], 0);

        gpio_init(btns[i]);
        gpio_set_dir(btns[i], GPIO_IN);
        gpio_pull_up(btns[i]);
        
        gpio_set_irq_enabled_with_callback(btns[i], GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    }

    // adc_init();
    // adc_gpio_init(26);
    // adc_select_input(0);
    // srand(adc_read());
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

int main() {
    setup_hardware();
    
    printf("Whac-A-Mole! Starting in 3 seconds...\n");
    sleep_ms(3000);

    init_adc();
    read_adc();

    // init_chardisp_pins();
    init_disp_spi();
    cd_init();
    cd_display1("ECE 362 is the  ");
    cd_display2("course for you! ");

    // display_welcome();
    // sleep_ms(3000);

    int score = 0;
    uint32_t start_time = to_ms_since_boot(get_absolute_time());

    while (to_ms_since_boot(get_absolute_time()) - start_time < GAME_DURATION_MS) {
        int mole_idx = rand() % NUM_MOLES;
        uint active_led = leds[mole_idx];
        uint target_btn = btns[mole_idx];

        gpio_put(active_led, 1);
        hit_registered = false; 

        uint32_t mole_start = to_ms_since_boot(get_absolute_time());
        uint32_t mole_window;
        switch (game_speed) {
        case SLOW:   mole_window = 5000; break;
        case MEDIUM: mole_window = 1000; break;
        case FAST:   mole_window =  100; break;
        }
        while (to_ms_since_boot(get_absolute_time()) - mole_start < mole_window) {
            if (hit_registered) {
                if (last_button_pressed == target_btn) {
                    score++;
                    printf("WHACKED! Score: %d\n", score);
                } else {
                    printf("MISSED!\n");
                }
                hit_registered = false;
                break; 
            }
            tight_loop_contents();
        }

        gpio_put(active_led, 0);
        sleep_ms(200 + (rand() % 500));
    }

    printf("\n--- GAME OVER ---\n");
    printf("Final Score: %d\n", score);
    
    // LEDs stay OFF at the end now.
    // We keep the loop alive so the program doesn't "exit" and lose the Serial connection.
    while(true) {
        tight_loop_contents();
    }
}