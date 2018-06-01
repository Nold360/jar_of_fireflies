/*
 * Jar of Fireflies Project "Jah's Fireflies"
 * By Gerrit 'Nold' Pannek 2015
 * Inspired by: http://www.instructables.com/id/Jar-of-Fireflies
 *              http://jason-webb.info/wiki/index.php?title=Jar_of_Fireflies
 *              https://gist.github.com/adnbr/9289235
 *
 * Optimized for Attiny13
 * PWM-Port: PB0, PB1
 * LED-Ground: PB3, PB4, PB2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
  
#define F_CPU 960000
 
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
 
//********* Configuration values
//PWM-Value, should be 255, because we might run at low voltage
#define PWM_VALUE 255
 
//Waiting time while fading
#define MIN_FADE_WAIT 2
#define MAX_FADE_WAIT 4
 
//How long will we wait for next round?
#define MIN_WAIT 200
#define MAX_WAIT 700
 
//How often can the LEDs flicker max?
#define MAX_FLICKER 3
 
//How many LEDs have (at least) to glow per round?
#define MIN_LED_COUNT 1
 
//Defines the chance that a LED gets activated
//So the chance is 1 to N for every single LED
// Minimum: 2
#define CHANCE 2
 
 
//Only change these on different uC or schematic:
#define LED1        PB2
#define LED2        PB3
#define LED3        PB4
#define PWM1_DDRB   DDB0
#define PWM2_DDRB   DDB1
 
 
//Don't change these:
#define ON  1
#define OFF 0
 
#define TRUE 1
#define FALSE 0
 
 
 
/*
 * Delay helper function
 */
static void sleep(int ms) {
    int i;
    for(i=0; i<ms; i++) {
        _delay_ms(1);
    }
}
 
/*
 *  Set a LED-Port on/off
 *  Off means "high" in our case, because we control the LED-Ground
 */
static void set_port(int port, uint8_t value) {
    if(value == OFF) {
        PORTB |= (1<<port);
    } else {
        PORTB &= ~(1<<port);
    }
}
 
/*
 * Choose a random PWM-Port
 * Returns 1 or 0
 *
 * Note: Disabling one Port removes a bug where disabled LEDs
 *       were still glowing.
 */
static int get_pwm_port(void) {
    uint8_t port = rand()%2;
    if(port == 0) {
        DDRB |= (1 << PWM1_DDRB);
        DDRB &= ~(1 << PWM2_DDRB);
    } else {
        DDRB |= (1 << PWM2_DDRB);
        DDRB &= ~(1 << PWM1_DDRB);
    }
    return port;
}
 
/*
 * Wet PWM-Register to specific value
 */
static void set_pwm_value(uint8_t value, uint8_t port) {
    if(port == 0) {
        OCR0A = value;
    } else {
        OCR0B = value;
    }
}
 
/*
 *Disable all LEDs
 */
static void set_leds_off(void) {
        set_port(LED1, OFF);
        set_port(LED2, OFF);
        set_port(LED3, OFF);
        set_pwm_value(0, 0);
        set_pwm_value(0, 1);
}
 
/*
 * Randomly activate available LEDs
 */
static void set_active_leds(void) {
    uint8_t led_count = 0;
    do {
        if(rand()%CHANCE == 0) {
            set_port(LED1, ON);
            led_count++;
        }
        if(rand()%CHANCE == 0) {
            set_port(LED2, ON);
            led_count++;
        }
        if(rand()%CHANCE == 0) {
            set_port(LED3, ON);
            led_count++;
        }
    } while(led_count < MIN_LED_COUNT);
}
 
/*
 * Generate random value between min and max
 * Will make sure that value fits in uint8_t if required
 */
static int get_rand(int min, int max, uint8_t get_uint) {
    int value = rand() % (max - min) + min;
     
    //To prevent overflow of uint8_t
    if(value > 255 && get_uint == TRUE) {
        return 255;
    } else {
        return value;
    }
}
 
int main(void) {
    //Setup
    uint8_t dir, i;
    uint8_t pwm_port;
    uint8_t flicker, fade_wait;
     
    //Init randomness using the answer..
    srand ( 42 );
     
    // Set PORTB to output
    DDRB = 0xFF;
     
    //PWM:
    // Set Timer 0 prescaler to clock/8.
    TCCR0B |= (1 << CS01);
     
    // Set to 'Fast PWM' mode
    TCCR0A |= (1 << WGM01) | (1 << WGM00);
     
    // Clear OC0A/OC0B output on compare match, upwards counting.
    TCCR0A |= (1 << COM0A1) | (1 << COM0B1);
     
    //Turn all LEDs off...
    set_leds_off();
     
    while(1) {
        //Which PWM-Port will we drive
        pwm_port = get_pwm_port();
 
        //Reactivate some random firefly LEDs
        //Let's have at least one active
        set_active_leds();
         
        //How oft will we flicker?
        flicker = get_rand(0, MAX_FLICKER+1, TRUE);
 
        //How long will fading last?
        fade_wait = get_rand(MIN_FADE_WAIT, MAX_FADE_WAIT, TRUE);
 
        //Normally I would do it in one loop, but this double-loop
        //saves 4 Bytes
        do {
            i = 1;
            dir = ON;
            while(i > 0) {
                set_pwm_value(i, pwm_port);
                 
                //Change direction on reached max. value
                if(i == PWM_VALUE)
                    dir = OFF;
                 
                //Fade up or down
                if(dir == ON) {
                    i++;
                } else {
                    i--;
                }
                 
                //Sleep a little bit
                sleep(fade_wait);
            }
             
            //Don't mess with unsigned ;-)
            if(flicker > 0) {
                flicker--;
            }
        } while(flicker > 0);
 
        //Disable all fireflies
        set_leds_off();
        sleep(get_rand(MIN_WAIT, MAX_WAIT, FALSE));
    }
}
