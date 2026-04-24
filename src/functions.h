#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

void init_gpio();
void gpio_callback();

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

static int duty_cycle = 0;
static int dir = 0;
static int color = 0;

void display_init_pins();
void display_init_timer();
void display_char_print(const char message[]);
void keypad_init_pins();
void keypad_init_timer();
void init_wavetable(void);
void set_freq(int chan, float f);
//extern KeyEvents kev;
void drum_machine();

void init_disp_spi(); // Initializes display, score, and time. Call at beginning.
void init_adc(); // Initializes ADC. Call at beginning.
void read_adc(); // Gets game speed from potentiometer. Call when first start game.
void display_welcome(); // Call at beginning.
void display_score_isr(); // Updates score on display. Call when first start game and when score changes.
void init_display_timer(); // Starts timer. Call when first start game.
void display_time_isr(); // Updates time on display. Call when first start game.
void display_game_over(); // Call if press incorrect or not in time
void display_game_over();
void cd_init();
void cd_display1(char *str);
void cd_display2(char *str);
void send_spi_cmd(spi_inst_t* spi, uint16_t value);
void send_spi_data(spi_inst_t* spi, uint16_t value);
