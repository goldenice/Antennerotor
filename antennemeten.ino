/**
 * TODOlist before this script can be used:
 * - Implement turning
 * - Implement the display
 * - Set defines to the correct pins and correct dswitch_state[]'s
 *
 */

/****************************************************************************************/
/***************************  STATIC DEFINES        *************************************/
/****************************************************************************************/

// These are the dstates, not the actual pins!
#define SW_TURN_LEFT = 1;   // turn left
#define SW_TURN_RIGHT = 2;  // turn right

#define SW_THDB_LEFT = 0;   // 3db left
#define SW_THDB_RIGHT = 0;  // 3db right

#define SW_ZERO = 0;        // reset dynamic degrees
#define SW_OET = 0;         // 180 deg turn
#define SW_KILL = 0;        // kill switch

#define PULSE_PIN = 11;
#define CW_PIN = 12;

/****************************************************************************************/
/***************************  LIBRARIES             *************************************/
/****************************************************************************************/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>


/****************************************************************************************/
/***************************  GLOBAL VARIABLES      *************************************/
/****************************************************************************************/

LiquidCrystal_I2C lcd(0x27, 20, 4);

// Digital switches and their states
int dswitches[9] =              {  9,  5,  6,  4,  8, 10,  7,  A0,  A1 };
int dswitch_state[9] =          {  0,  0,  0,  0,  0,  0,  0,   0,   0 };
int dswitch_state_old[9] =      {  0,  0,  0,  0,  0,  0,  0,   0,   0 };

// The timers used throughout the program
unsigned long timer_last[5] =   {   0,   0,   0,   0,   0 };
unsigned long timer_diff[5] =   { 200,   0,   0,   0,   0 };

// Microtimer: does the same as the main timer, but based on microseconds
unsigned long microtimer_last[5] = {   0,   0,   0,   0,   0 };
unsigned long microtimer_diff[5] = {8000,   0,   0,   0,   0 };

// State of the engine
bool longturn = false;
bool turning_left = false;
bool turning_right = false;
float offset = 0.0;
float angle = 0.0;
float target = 0.0;
bool pulseState = false;


/****************************************************************************************/
/***************************  MAIN FUNCTIONS        *************************************/
/****************************************************************************************/

/**
 * Setup function: is run once before loop is run continuously
 */
void setup() {
    // Set input mode for all digital switches
    for (int i = 0; i < (sizeof(dswitches)/sizeof(int)); i++) {
        pinMode(dswitches[i], INPUT);
    }

    pinMode(PULSE_PIN, OUTPUT);
    pinMode(CW_PIN, OUTPUT);

    lcd.init();
    lcd.backlight();
    lcd.clear();
}

/**
 * Main loop
 */
void loop() {
    if (checkMicroTimer(0)) {
        resetMicroTimer(0);
        doPulse();
    }

    if (checkTimer(0)) {
        resetTimer(0);
        refreshInputDigital();
        refreshInputAnalog();
        interpretInput();
    }
}

/****************************************************************************************/
/***************************  INPUT READING         *************************************/
/****************************************************************************************/

/**
 * Refreshes input on the digital pins
 */
void refreshInputDigital() {
    for (int i = 0; i < (sizeof(dswitches)/sizeof(int)); i++) {
        dswitch_state[i] = digitalRead(dswitches[i]);
    }
}

void interpretInput() {
    if (!arrayEquals(dswitch_state, dswitch_state_old) || !arrayEquals(aswitch_state, aswitch_state_old)) {
        if (!longturn) {
            turning_left = dswitch_state[SW_TURN_LEFT] == HIGH;
            turning_right = dswitch_state[SW_TURN_RIGHT] == HIGH;
            turning = turning_left || turning_right;
            if (dswitch_state[SW_ZERO] == HIGH) {
                offset = angle;
            }

        }
        if (dswitch_state[SW_KILL] == HIGH) {
            longturn = false;
            turning_left = false;
            turning_right = false;
            target = angle;
        }
    }
    dswitch_state_old = dswitch_state;
    aswitch_state_old = aswitch_state;
}


/****************************************************************************************/
/***************************  MOTOR FUNCTIONS       *************************************/
/****************************************************************************************/

void setDirection(bool cw) {
    if (cw) {
        digitalWrite(CW_PIN, HIGH);
    } else {
        digitalWrite(CW_PIN, LOW);
    }
}

void doPulse() {
    if (turning) {
        if (pulseState) {
            digitalWrite(PULSE_PIN, HIGH);
            pulseState = true;
        } else {
            digitalWrite(PULSE_PIN, LOW);
            pulseState = false;
        }
    }
}


/****************************************************************************************/
/***************************  HELPER FUNCTIONS      *************************************/
/****************************************************************************************/

/**
 * Checks whether a certain timer has expired already
 * @param    int     timerid    The ID of the timer to check on
 * @return   bool
 */
bool checkTimer(int timerid) {
    return ((millis() < timer_last[timerid]) || ((millis() - timer_last[timerid]) >= timer_diff[timerid]));
}

/**
 * Resets the timer by ID
 * @param    int    timerid    The ID of the timer to reset
 */
void resetTimer(int timerid) {
    timer_last[timerid] = millis();
}

/**
 * Gives the time until the timer expires
 * @param    int     timerid    ID of the timer
 * @return   int
 */
int untilTimerExpires(int timerid) {
    return (millis() - microtimer_last[timerid]);
}

/**
 * Checks whether a certain microtimer has expired already
 * @param    int     timerid    The ID of the timer to check on
 * @return   bool
 */
bool checkMicroTimer(int timerid) {
    return ((micros() < microtimer_last[timerid]) || ((micros() - microtimer_last[timerid]) >= microtimer_diff[timerid]));
}

/**
 * Resets the microtimer by ID
 * @param    int    timerid    The ID of the timer to reset
 */
void resetMicroTimer(int timerid) {
   microtimer_last[timerid] = micros();
}

/**
 * Checks if a float is in a certain range
 * @param    float  num1      The number to check on
 * @param    float  num2      The reference number
 * @param    float  delta     The maximum difference
 * @return   bool
 */
bool checkFloat(float num1, float num2, float delta) {
    return (((num1 - num2) < delta) && ((num1 - num2) > -delta));
}


bool arrayEquals(int[] arr1, int[] arr2) {
    if (sizeof(arr1) != sizeof(arr2)) return false;
    for (int i = 0; i < sizeof(arr1)/sizeof(arr1[0]); i++) {
        if (arr1[i] != arr2[i]) return false;
    }
    return true;
}
