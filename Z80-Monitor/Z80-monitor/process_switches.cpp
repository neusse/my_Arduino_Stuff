#include <Arduino.h>
#include "constants.h"
#include "typedefs.h"
#include "commands.h"

#define STATE_WAIT 0
#define STATE_CHECK1 1
#define STATE_CHECK2 2
#define STATE_WAIT_RELEASE 3
int switch_state[3];

#define ACTION_NONE 0
#define ACTION_RUN1 1
#define ACTION_RUN2 2
int switch_action[3];
switch_functions_t switch_functions;

unsigned long debounce_time[3];
unsigned long DEBOUNCE_TIME1 = 50;
unsigned long DEBOUNCE_TIME2 = 1500;

void flash_delay() {
    delay(100);
}

void flash_led(int num_flashes) {
    for (int i = 0; i < num_flashes; i++) {
        if (i > 0) flash_delay();
        digitalWrite(USER_LED, HIGH);
        flash_delay();
        digitalWrite(USER_LED, LOW);
    }
}

void check_switch(const int pin_number) {
    if (switch_functions.short_press == nullptr && switch_functions.long_press == nullptr) return;
    int index = pin_number - USER_SW1;

    if (digitalRead(pin_number) == HIGH) {
        switch (switch_state[index]) {
            case STATE_WAIT:
                debounce_time[index] = millis();
                switch_state[index]++;
                break;
            case STATE_CHECK1:
                if ((millis() - debounce_time[index]) > DEBOUNCE_TIME1) {
                    if (switch_functions.short_press != nullptr) {
                        flash_led(1);
                        switch_action[index] = ACTION_RUN1;
                    }
                    switch_state[index]++;
                }
                break;
            case STATE_CHECK2:
                if ((millis() - debounce_time[index]) > DEBOUNCE_TIME2) {
                    if (switch_functions.long_press != nullptr) {
                        flash_led(2);
                        switch_action[index] = ACTION_RUN2;
                    }
                    switch_state[index]++;
                }
                break;
            case STATE_WAIT_RELEASE:
            default:
                delay(DEBOUNCE_TIME1);
                break;
        }
    } else {
        switch(switch_action[index]) {
            case ACTION_RUN1:
                if (switch_functions.short_press != nullptr) (*switch_functions.short_press)();
                break;
            case ACTION_RUN2:
                if (switch_functions.long_press != nullptr) (*switch_functions.long_press)();
                break;
        }
        switch_state[index] = STATE_WAIT;
        switch_action[index] = ACTION_NONE;
    }
}

void process_switches() {
    select_command(USER_SW1, &switch_functions);
    check_switch(USER_SW1);

    select_command(USER_SW2, &switch_functions);
    check_switch(USER_SW2);

    select_command(USER_SW3, &switch_functions);
    check_switch(USER_SW3);
}