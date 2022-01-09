/*
 * 
 * Simulated Toaster
 * Leo King
 * May 19, 2021 
 * 
 */
// **** Include libraries here ****
// Standard libraries
#include <stdio.h>

//Support Library
#include "BOARD.h"
#include "Ascii.h"
#include "Adc.h"
#include "Oled.h"
#include "Leds.h"
#include "Buttons.h"

// Microchip libraries
#include <xc.h>
#include <sys/attribs.h>

typedef enum {
    SETUP, SELECTOR_CHANGE_PENDING, COOKING, RESET_PENDING, ALERT
} OvenState;

typedef enum {
    BAKE, TOAST, BROIL
} Mode;

typedef enum {
    TIME, TEMPERATURE
} Selector;

typedef struct {
    OvenState state;
    //add more members to this struct
    Mode mode;
    Selector selector;
    uint8_t ADCevent;
    uint8_t Buttonevent;
    uint16_t TimerTicks;
    uint16_t temperature;
    uint16_t BtnPressTime;
    uint16_t StartTime;
    uint16_t timeremaining;
} OvenData;

// **** Declare any data types here ****

// **** Define any module-level, global, or external variables here ****
#define Window 3
#define long_press 5
#define Min_Temp 300
#define BakeTemp 50
#define Min_Time 1
#define NONE 20
#define AdcMask 0x3FC
#define Shift1 1
#define Shift2 2
#define sec 60
#define ALLON 0xFF
#define reset 0
#define LD1_8 8

char str[100] = "";
char timeStr1[10] = "";
char timeStr2[10] = "";
char *OvenTopOn = "\x01\x01\x01\x01\x01"; //OVEN_TOP_On       5x
char *OvenTopOff = "\x02\x02\x02\x02\x02"; //OVEN_TOP_OFF      5x
char *OvenBottomOn = "\x03\x03\x03\x03\x03"; //OVEN_TOP_On       5x
char *OvenBottomOff = "\x04\x04\x04\x04\x04"; //OVEN_BOTTOM_OFF   5x
static uint16_t btn = BUTTON_EVENT_NONE;
static uint16_t btn3 = BUTTON_EVENT_NONE;
static uint16_t btn4 = BUTTON_EVENT_NONE;
static uint16_t freeRunningTime = 0;
static uint16_t start_time = 0;
static uint8_t TimerTickEvent = FALSE;
static uint8_t Seconds;
static uint8_t Minutes;
static uint8_t CookSecs;
static uint8_t CookMins;
static uint16_t TimerTicks;
static uint16_t secondsremoved = 0;

OvenData Toaster = {SETUP, BAKE, TIME, NONE, FALSE, Min_Time, BakeTemp, TIME, Min_Time, Min_Time};

// **** Helper functions here ****

void updateOvenLED(void) {
    uint16_t counter = LEDS_GET();
    uint16_t perLED = ((Minutes * sec) + Seconds) / LD1_8;
    if (perLED == 0) {
        perLED = Min_Time;
        counter = (counter << Shift1) & ALLON;
        LEDS_SET(counter);
    } else if ((secondsremoved % perLED) == 0) {
        counter = (counter << Shift1) & ALLON;
        LEDS_SET(counter);
    }
}

/*This function will update OLED to reflect the state .*/
void updateOvenOLED(OvenData ovenData) {
    //update OLED here
    Minutes = ovenData.StartTime / sec;
    Seconds = ovenData.StartTime % sec;
    CookMins = ovenData.timeremaining / sec;
    CookSecs = ovenData.timeremaining % sec;
    uint16_t Bake_temperature = ovenData.temperature + Min_Temp;
    OledClear(OLED_COLOR_BLACK);
    if (ovenData.mode == BAKE) {
        if (ovenData.state == COOKING) {
            sprintf(str, "|%s|  Mode: Bake", OvenTopOn);
            OledDrawString(str);
            sprintf(str, "\n|     |   Time: %d:%02d", CookMins, CookSecs);
            OledDrawString(str);
            sprintf(str, "\n\n|-----|   Temp: %d%sF", Bake_temperature, DEGREE_SYMBOL);
            OledDrawString(str);
            sprintf(str, "\n\n\n|%s|", OvenBottomOn);
            OledDrawString(str);
        } else if (ovenData.selector == TIME) {
            sprintf(str, "|%s|  Mode: Bake", OvenTopOff);
            OledDrawString(str);
            sprintf(str, "\n|     |  >Time: %d:%02d", Minutes, Seconds);
            OledDrawString(str);
            sprintf(str, "\n\n|-----|   Temp: %d%sF", Bake_temperature, DEGREE_SYMBOL);
            OledDrawString(str);
            sprintf(str, "\n\n\n|%s|", OvenBottomOff);
            OledDrawString(str);
        } else {
            sprintf(str, "|%s|  Mode: Bake", OvenTopOff);
            OledDrawString(str);
            sprintf(str, "\n|     |   Time: %d:%02d", Minutes, Seconds);
            OledDrawString(str);
            sprintf(str, "\n\n|-----|  >Temp: %d%sF", Bake_temperature, DEGREE_SYMBOL);
            OledDrawString(str);
            sprintf(str, "\n\n\n|%s|", OvenBottomOff);
            OledDrawString(str);
        }
    } else if (ovenData.mode == TOAST) {
        ovenData.selector = TIME;
        if (ovenData.state == COOKING) {
            sprintf(str, "|%s|  Mode: TOAST", OvenTopOn);
            OledDrawString(str);
            sprintf(str, "\n|     |   Time: %d:%02d", CookMins, CookSecs);
            OledDrawString(str);
            sprintf(str, "\n\n|-----|");
            OledDrawString(str);
            sprintf(str, "\n\n\n|%s|", OvenBottomOn);
            OledDrawString(str);
        } else {
            sprintf(str, "|%s|  Mode: TOAST", OvenTopOff);
            OledDrawString(str);
            sprintf(str, "\n|     |   Time: %d:%02d", Minutes, Seconds);
            OledDrawString(str);
            sprintf(str, "\n\n|-----|");
            OledDrawString(str);
            sprintf(str, "\n\n\n|%s|", OvenBottomOff);
            OledDrawString(str);
        }
    } else if (ovenData.mode == BROIL) {
        ovenData.selector = TIME;
        if (ovenData.state == COOKING) {
            sprintf(str, "|%s|  Mode: BROIL", OvenTopOn);
            OledDrawString(str);
            sprintf(str, "\n|     |   Time: %d:%02d", CookMins, CookSecs);
            OledDrawString(str);
            sprintf(str, "\n\n|-----|   Temp: 500%sF", DEGREE_SYMBOL);
            OledDrawString(str);
            sprintf(str, "\n\n\n|%s|", OvenBottomOn);
            OledDrawString(str);
        } else {
            sprintf(str, "|%s|  Mode: BROIL", OvenTopOff);
            OledDrawString(str);
            sprintf(str, "\n|     |   Time: %d:%02d", Minutes, Seconds);
            OledDrawString(str);
            sprintf(str, "\n\n|-----|   Temp: 500%sF", DEGREE_SYMBOL);
            OledDrawString(str);
            sprintf(str, "\n\n\n|%s|", OvenBottomOff);
            OledDrawString(str);
        }
    }
    OledUpdate();
}

/*This function will execute the state machine.  
 * It should ONLY run if an event flag has been set.*/
void runOvenSM(void) {
    //write your SM logic here.
    Toaster.BtnPressTime = freeRunningTime - start_time;
    switch (Toaster.state) {
        case SETUP:
            if (Toaster.ADCevent == TRUE) {
                updateOvenOLED(Toaster);
            } else if (Toaster.ADCevent == NONE) {
                updateOvenOLED(Toaster);
            }
            if (btn4 == BUTTON_EVENT_4DOWN) {
                start_time = freeRunningTime;
                Toaster.state = COOKING;
                Toaster.timeremaining = Toaster.StartTime;
                updateOvenOLED(Toaster);
                LEDS_SET(ALLON);
            } else if (btn3 == BUTTON_EVENT_3DOWN) {
                start_time = freeRunningTime;
                Toaster.state = SELECTOR_CHANGE_PENDING;
            }
            break;
        case SELECTOR_CHANGE_PENDING:
            if (btn3 == BUTTON_EVENT_3UP) {
                if (Toaster.BtnPressTime < long_press) {
                    if (Toaster.mode == BAKE) {
                        Toaster.mode = TOAST;
                    } else if (Toaster.mode == TOAST) {
                        Toaster.mode = BROIL;
                    } else {
                        Toaster.mode = BAKE;
                    }
                    updateOvenOLED(Toaster);
                    Toaster.state = SETUP;
                } else if (Toaster.BtnPressTime > long_press) {
                    if ((Toaster.mode == BAKE) && (Toaster.selector == TIME)) {
                        Toaster.selector = TEMPERATURE;
                        updateOvenOLED(Toaster);
                    } else if ((Toaster.mode == BAKE) && (Toaster.selector == TEMPERATURE)) {
                        Toaster.selector = TIME;
                        updateOvenOLED(Toaster);
                    }
                    Toaster.state = SETUP;
                }
            }
            break;
        case COOKING:
            if (btn4 == BUTTON_EVENT_4DOWN) {
                start_time = freeRunningTime;
                Toaster.state = RESET_PENDING;
            } else if (Toaster.timeremaining > 0) {
                updateOvenLED();
                updateOvenOLED(Toaster);
            } else {
                Toaster.timeremaining = Toaster.StartTime;
                Toaster.state = SETUP;
                updateOvenOLED(Toaster);
                LEDS_SET(0);
            }
            break;
        case RESET_PENDING:
            if (btn4 == BUTTON_EVENT_4UP) {
                Toaster.state = COOKING;
            } else if (Toaster.BtnPressTime >= long_press) {
                Toaster.state = SETUP;
                updateOvenOLED(Toaster);
                LEDS_SET(0);
            } else {
                updateOvenLED();
                updateOvenOLED(Toaster);
            } break;
    }   
        
}

int main() {
    BOARD_Init();

    //initalize timers and timer ISRs:
    // <editor-fold defaultstate="collapsed" desc="TIMER SETUP">

    // Configure Timer 2 using PBCLK as input. We configure it using a 1:16 prescalar, so each timer
    // tick is actually at F_PB / 16 Hz, so setting PR2 to F_PB / 16 / 100 yields a .01s timer.

    T2CON = 0; // everything should be off
    T2CONbits.TCKPS = 0b100; // 1:16 prescaler
    PR2 = BOARD_GetPBClock() / 16 / 100; // interrupt at .5s intervals
    T2CONbits.ON = 1; // turn the timer on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T2IF = 0; //clear the interrupt flag before configuring
    IPC2bits.T2IP = 4; // priority of  4
    IPC2bits.T2IS = 0; // subpriority of 0 arbitrarily 
    IEC0bits.T2IE = 1; // turn the interrupt on

    // Configure Timer 3 using PBCLK as input. We configure it using a 1:256 prescaler, so each timer
    // tick is actually at F_PB / 256 Hz, so setting PR3 to F_PB / 256 / 5 yields a .2s timer.

    T3CON = 0; // everything should be off
    T3CONbits.TCKPS = 0b111; // 1:256 prescaler
    PR3 = BOARD_GetPBClock() / 256 / 5; // interrupt at .5s intervals
    T3CONbits.ON = 1; // turn the timer on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T3IF = 0; //clear the interrupt flag before configuring
    IPC3bits.T3IP = 4; // priority of  4
    IPC3bits.T3IS = 0; // subpriority of 0 arbitrarily 
    IEC0bits.T3IE = 1; // turn the interrupt on;

    // </editor-fold>
    

    //initialize state machine (and anything else you need to init) here
    OledInit();
    LEDS_INIT();
    AdcInit();
    ButtonsInit();
    while (1) {
        // Add main loop code here:
        // check for events
        // on event, run runOvenSM()
        // clear event flags
        if (Toaster.ADCevent == TRUE) {
            runOvenSM();
            Toaster.ADCevent = FALSE;
        } else if (Toaster.ADCevent == NONE) {
            runOvenSM();
        }
        if (Toaster.Buttonevent == TRUE) {
            runOvenSM();
            Toaster.Buttonevent = FALSE;
        }
        if (TimerTickEvent == TRUE) {
            runOvenSM();
            TimerTickEvent = FALSE;
        }
    }
}

/*The 5hz timer is used to update the free-running timer and to generate TIMER_TICK events*/
void __ISR(_TIMER_3_VECTOR, ipl4auto) TimerInterrupt5Hz(void) {
    // Clear the interrupt flag.
    IFS0CLR = 1 << 12;

    //add event-checking code here
    freeRunningTime++;

    if ((freeRunningTime - start_time) % long_press == 0) {
        Toaster.TimerTicks = 0;
        Toaster.timeremaining--;
        secondsremoved++;
        TimerTickEvent = TRUE;
    }

}

/*The 100hz timer is used to check for button and ADC events*/
void __ISR(_TIMER_2_VECTOR, ipl4auto) TimerInterrupt100Hz(void) {
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;

    //add event-checking code here
    btn = ButtonsCheckEvents();
    if (AdcChanged() == TRUE) {
        Toaster.ADCevent = TRUE;
        if (Toaster.selector == TIME) {
            Toaster.StartTime = (AdcRead() & AdcMask) >> Shift2;
            Toaster.StartTime += Min_Time;
        } else {
            Toaster.temperature = (AdcRead() & AdcMask) >> Shift2;
        }
    }

    if (btn == BUTTON_EVENT_3DOWN) {
        btn3 = BUTTON_EVENT_3DOWN;
        Toaster.Buttonevent = TRUE;
    } else if (btn == BUTTON_EVENT_3UP) {
        btn3 = BUTTON_EVENT_3UP;
        Toaster.Buttonevent = TRUE;
    } else if (btn == BUTTON_EVENT_4DOWN) {
        btn4 = BUTTON_EVENT_4DOWN;
        Toaster.Buttonevent = TRUE;
    } else if (btn == BUTTON_EVENT_4UP) {
        btn4 = BUTTON_EVENT_4UP;
        Toaster.Buttonevent = TRUE;
    }
}