/****************************************************************************************/
/***************************  LIBRARIES             *************************************/
/****************************************************************************************/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>


/****************************************************************************************/
/***************************  GLOBAL VARIABLES      *************************************/
/****************************************************************************************/


// Digital switches and their states
int dswitches[7] =     {  9,  5,  6,  4,  8, 10,  7 };
int dswitch_state[7] = {  0,  0,  0,  0,  0,  0,  0 };

// Analog switches and their states
int aswitches[2] =     { A0, A1 };
int aswitch_state[2] = {  0,  0 };

// The timers used throughout the program
unsigned long timer_last[5] = {   0,   0,   0,   0,   0 };
unsigned long timer_diff[5] = { 200,   0,   0,   0,   0 };

// Microtimer: does the same as the main timer, but based on microseconds
unsigned long microtimer_last[5] = {   0,   0,   0,   0,   0 };
unsigned long microtimer_diff[5] = {   0,   0,   0,   0,   0 };

// State of the engine
bool turning = false;
float angle = 0.0;
float target = 0.0;

// Set default speed
byte speed = 5;

LiquidCrystal_I2C lcd(0x27, 20, 4);


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
    // Set input mode for all analog switches
    for (int i = 0; i < (sizeof(aswitches)/sizeof(int)); i++) {
        pinMode(aswitches[i], INPUT);
    }

    lcd.begin(20, 4);
    lcd.clear();
    lcd.backlight();
    lcd.print("HELLO WORLD");
}

/**
 * Main loop
 */
void loop() {
    if (checkTimer(0)) {
        resetTimer(0);
        refreshInputDigital();
        refreshInputAnalog();
    }

    //lcd.begin(20, 4);
    //lcd.clear();
    //lcd.backlight();
    //lcd.setCursor(0,0);
    //lcd.print("HELLO WORLD");
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

/**
 * Refreshes the input on the analog pins
 */
void refreshInputAnalog() {
    for (int i = 0; i < (sizeof(aswitches)/sizeof(int)); i++) {
        aswitch_state[i] = digitalRead(aswitches[i]);
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
