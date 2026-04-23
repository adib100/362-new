// - ADC with potentiometer to change game speed setting
// - SPI with LCD display to show leaderboard stats and update the game score
// - Lauren: SPI + LCD for stats + game score, ADC with potentiometer

#include "functions.h"

// current lcd to pin, 4 to 33 sck, 5 to 35 tx, 6 to 34 csn
// const int SPI_DISP_SCK = 26; 
// const int SPI_DISP_CSn = 25;
// const int SPI_DISP_TX = 27; 
const int SPI_DISP_SCK = 34; // 4 to 34 sck
const int SPI_DISP_CSn = 35; // 5 to 35 tx
const int SPI_DISP_TX = 37; // 6 to 37 csn
// spi0
const int ADC_CH5 = 45; 
int score = 0;
int time_left = 30;
int highscore = 0;

typedef enum {
    IDLE,
    PLAYING,
    GAME_OVER
} GameState;

typedef enum {
    SLOW,
    MEDIUM,
    FAST
} GameSpeed;

GameSpeed game_speed = SLOW;
GameState game_status = IDLE;


// #include "pico/stdlib.h"
// #include "hardware/spi.h"
// #include "hardware/gpio.h"
// #include "hardware/timer.h"
// #include "hardware/adc.h"

// /*
// Function initializations:
// */

// // For main to call:
// void init_disp_spi(); // Initializes display, score, and time. Call at beginning.
// void init_adc(); // Initializes ADC. Call at beginning.
// void read_adc(); // Gets game speed from potentiometer. Call when first start game.
// void display_welcome(); // Call at beginning.
// void display_score_isr(); // Updates score on display. Call when first start game and when score changes.
// void init_display_timer(); // Starts timer. Call when first start game.
// void display_time_isr(); // Updates time on display. Call when first start game.
// void display_game_over(); // Call if press incorrect or not in time

// // Helper functions:
// void display_game_over();
// void cd_init();
// void cd_display1(const char *str);
// void cd_display2(const char *str);
// void send_spi_cmd(spi_inst_t* spi, uint16_t value);
// void send_spi_data(spi_inst_t* spi, uint16_t value);

// // Global variables - PUT IN MAIN LATER
// const int SPI_DISP_SCK = 26; 
// const int SPI_DISP_CSn = 25;
// const int SPI_DISP_TX = 27;
// const int ADC_CH5 = 45; 
// int score; // already in
// int time_left; // already in
// int highscore; // already in
// typedef enum {
//     SLOW,
//     MEDIUM,
//     FAST
// } GameSpeed;
// GameSpeed game_speed = SLOW;

/*
####################
------- ADC -------
####################
*/

void init_adc() 
{
    // Reset ADC in case of unknown state
    // reset_unreset_block_num_wait_blocking(RESET_ADC);

    // Enable ADC
    adc_hw->cs = ADC_CS_EN_BITS;

    // Check that ADC is ready
    // while (!(adc_hw->cs & ADC_CS_READY_BITS)) {
    //     tight_loop_contents();
    // }

    // Initialize GPIO 45 (ADC channel 5) for use as ADC pin
    // adc_gpio_init(45);
    gpio_set_function(ADC_CH5, GPIO_FUNC_NULL);
    gpio_disable_pulls(ADC_CH5);
    gpio_set_input_enabled(ADC_CH5, false);

    // Select channel 5 as ADC input
    // adc_select_input(5);
    hw_write_masked(&adc_hw->cs, 5 << ADC_CS_AINSEL_LSB, ADC_CS_AINSEL_BITS);
}

void read_adc() {
    // Start single-shot conversion
    hw_set_bits(&adc_hw->cs, ADC_CS_START_ONCE_BITS);

    // Wait for FIFO to have data then get result from FIFO
    while (!(adc_hw->cs & ADC_CS_READY_BITS))
        tight_loop_contents();

    uint16_t potent = (uint16_t)adc_hw->result;
    
    if(potent < 1365)
    {
        game_speed = SLOW;
    }
    else if(potent >= 1365 & potent < 2730)
    {
        game_speed = MEDIUM;
    }
    else
    {
        game_speed = FAST;
    } 

    // Above equivalent:
    // // Wait for the ADC FIFO to have data
    // adc_fifo_get_blocking();
    // // Get ADC result from FIFO 
    // return adc_fifo_get();
}


/*
####################
------- SPI -------
####################
*/

// Timer 0, alarm 0 used for keeping track of game time
void init_display_timer()
{
    // Enable interrupt for alarm
    hw_set_bits(&timer_hw->inte, 1u << 0);
    // Set handler for alarm
    irq_set_exclusive_handler(TIMER0_IRQ_0, display_time_isr);
    // Enable alarm interrupt
    irq_set_enabled(TIMER0_IRQ_0, true);
    // Set time for alarm
    uint64_t target = timer_hw->timerawl + (10000);
    // Write time to alarm to arm
    timer_hw->alarm[0] = (uint32_t) target;
}

void send_spi_cmd(spi_inst_t* spi, uint16_t value) {
    // Wait until SPI isn't busy
    while(spi_is_busy(spi));

    // Write value
    // Allowed to use SDK or need to write directly to register?
    // spi_write16_blocking(spi, &value, 1);
    spi_get_hw(spi)->dr = value;
}

void send_spi_data(spi_inst_t* spi, uint16_t value) {
    send_spi_cmd(spi, value | 0x100);
}

void cd_init() {
    // Sleep 1ms to let power to display stabilize
    sleep_us(1);

    //Perform function set command: 8-bit interface, 2 line display mode, 11 dots per char
    uint16_t value = 0xF << 2;
    send_spi_cmd(spi0, value);
    // Page 16 of LCD datasheet
    // 0 0 1 DL N F - - = 001111XX = 0xF << 2
    // DL high for 8-bit bus mode
    // N high for 2-line display mode
    // F high for 11 dots display mode

    // Sleep 40us
    sleep_us(40);

    // Perform display on/off command: turn on display, turn off cursor, turn off cursor blink
    value = 0xC;
    send_spi_cmd(spi0, value);
    // Page 15 of LCD datasheet
    // 0 0 0 0 1 D C B = 00001100 = 0xC
    // D high for display  on
    // C low for cursor off
    // B low for cursor blink off

    // Sleep 40us
    sleep_us(40);

    // Perform clear display commmand
    value = 1; 
    send_spi_cmd(spi0, value);
    // Page 15 of LCD datasheet
    // 00000001
    // Clears display data by writing 20H to all DDRAM address, set DDRAM 
    // address to addresscounter, return cursor to orginal space

    // Sleep 2ms
    sleep_ms(2);

    // Perform entry mode set command
    // Move cursor to the right and increment DDRAM address by 1 (don't shift display)
    value = 0x6; 
    send_spi_cmd(spi0, value);
    // Datasheet page 15
    // 0 0 0 0 0 1 I/D SH = 00000110 = 0x6
    // I/D high for cursor moves to right, DDRAM address increased by 1
    // SH low for no entire shift display

    // Sleep 40us
    sleep_us(40);
}

void init_disp_spi()
{
    for(int pin = SPI_DISP_CSn; pin <= SPI_DISP_TX; pin++)
    {
        gpio_set_function(pin, GPIO_FUNC_SPI); // GPIO_FUNC_SPI = 1
    }
    spi_init(spi0, 10000);
    spi_set_format(spi0, 9, 0, 0, SPI_MSB_FIRST);

    init_display_timer();
    cd_init();
}

void display_welcome()
{
    char *str1 = "Push button to start";
    cd_display1(str1);
    char *str2 = "High Score: %d", highscore;
    cd_display2(str2);
}

void display_time_isr()
{
    char *str = "Time left: %d s", time_left;
    cd_display1(str);

    // If time_left = 0, update GameState to GAME_OVER
    if(time_left == 0)
    {
        game_status = GAME_OVER;
        display_game_over();
        return;
    }

    // Acknowledge interrupt for ALARM0 on TIMER1
    timer_hw->intr = 1u << 0;
    uint64_t target = timer_hw->timerawl + (10000);
    timer_hw->alarm[0] = (uint32_t) target;

    time_left = time_left - 1;
}

void display_game_over()
{
    char *str1 = "Game Over!";
    cd_display1(str1);
    char *str2 = "Final Score: %d", score;
    cd_display2(str2);
}

void display_score_isr()
{
    char *str = "Your score: %d", score;
    cd_display1(str);
}

// First line display of LCD
void cd_display1(char *str) {
    // Send command to move cursor to first line (starts at address 00H)
    uint16_t value = (0x80) | (0x00);
    send_spi_cmd(spi0, value);
    // Datasheet page 16/17
    // DDRAM controls cursor position
    // 1 AC6 AC5 AC4 AC3 AC2 AC1 AC0 = 0x80 | AC
    // AC is DDRAM address

    // Send each character (16) in str to the display using send_spi_data
    for(int let = 0; let < 16; let++)
    {
        send_spi_data(spi0, str[let]);
    }
}

// Second line display of LCD
void cd_display2(char *str) {
    // Send command to move cursor to second line (starts at address 40H)
    uint16_t value = (0x80) | (0x40); 
    send_spi_cmd(spi0, value);
    // Datasheet page 16/17
    // DDRAM controls cursor position
    // 1 AC6 AC5 AC4 AC3 AC2 AC1 AC0 = 0x80 | AC
    // AC is DDRAM address

    // Send each character (16) in str to the display using send_spi_data
    for(int let = 0; let < 16; let++)
    {
        send_spi_data(spi0, str[let]);
    }
}
