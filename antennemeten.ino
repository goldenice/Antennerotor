#define SW_TURN_LEFT 5 
#define SW_TURN_RIGHT 6

#define SW_THDB_LEFT 10
#define SW_THDB_RIGHT 7

#define SW_ZERO 8
#define SW_OET 4
#define SW_KILL 9

#define SW_SPD_LOW A1
#define SW_SPD_HIGH A0

#define OUT_CW 12
#define OUT_PULSE 11

#define LONG_TURN_LED 13

// Defining the struct to read the current state of the switches in
struct SwitchState {
  boolean turnleft;
  boolean turnright;
  boolean thdbleft;
  boolean thdbright;
  boolean zero;
  boolean oet;
  boolean kill;
  boolean speed_low;
  boolean speed_high;
};
SwitchState switches = (SwitchState) { false, false, false, false, false, false, false };

struct SerialCommand {
  boolean listening;
  int atStep;         // 0 = dir, 1 = angle, 2 = time, 3 = invalid, waiting for /r and/or /n to reset
  boolean right;      // CW = true, CCW = false
  int angle;
  int seconds;  
};
SerialCommand serialCommand = (SerialCommand) { true, 0, false, 0, 0 };

// Defining the timers used in the program
struct Timer {
  unsigned long last;
  unsigned long interval;
  boolean micro;
};
Timer timer[10];

int pulsePin = LOW;
int outputCtr = 0;

struct TurnState {
  boolean turnlongbyserial;
  boolean turnlong;
  boolean left;
  boolean right;
  float angle;
  float offset;
  float target;
  float thdbleft;
  float thdbright;
};
TurnState turnstate = (TurnState) { false, false, false, false, 0.0, 0.0, 0.0, 0.0, 0.0 };

#include <Wire.h>
#include <FastIO.h>
#include <I2CIO.h>
#include <LCD.h>
#include <LiquidCrystal.h>
#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal_SR.h>
#include <LiquidCrystal_SR2W.h>
#include <LiquidCrystal_SR3W.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

void setup() {
  pinMode(SW_TURN_LEFT, INPUT);
  pinMode(SW_TURN_RIGHT, INPUT);
  pinMode(SW_THDB_LEFT, INPUT);
  pinMode(SW_THDB_RIGHT, INPUT);
  pinMode(SW_ZERO, INPUT);
  pinMode(SW_OET, INPUT);
  pinMode(SW_KILL, INPUT);
  
  pinMode(OUT_CW, OUTPUT);
  pinMode(OUT_PULSE, OUTPUT);
  pinMode(LONG_TURN_LED, OUTPUT);
  
  timer[0] = { micros(), 8000, true };
  timer[1] = { millis(), 103, false };
  timer[2] = { millis(), 163, false };
  timer[3] = { millis(), 80, false };
  timer[4] = { millis(), 80, false };
  
  Serial.begin(38400);
  
  lcd.begin(20, 4);
  lcd.setCursor(0, 0);
  lcd.print(" DEG: "); 
  lcd.setCursor(0, 1);  
  lcd.print(" OFF: ");
  lcd.setCursor(0, 2);
  lcd.print("-3dB: ");
  lcd.setCursor(0, 3);
  lcd.print(" 3dB: ");
}

void loop() {
  if (checkTimer(0) && (turnstate.left || turnstate.right || pulsePin == HIGH)) pulse();
  if (checkTimer(1)) refreshInput();
  if (checkTimer(2)) output();
  if (checkTimer(3)) checkSerialInput();
  if (checkTimer(4)) writeLongTurnLed();
}

void checkSerialInput() {
  while (Serial.available() > 0 && serialCommand.listening) {
    const char c = Serial.read();

    // Process potential end of command
    if ((c == '\r' || c == '\n')) {
      if (serialCommand.atStep == 2 && serialCommand.seconds > 0) executeSerialCommand();
      serialCommand.atStep = 0;
      serialCommand.right = false;
      serialCommand.angle = 0;
      serialCommand.seconds = 0;
      continue;
    }

    if (serialCommand.atStep == 0) {
      if (c == 'L' || c == 'R') {
        serialCommand.right = c == 'R';
        continue;
      } else if (c == ' ') {
        serialCommand.atStep++;
        continue;
      }
    }

    if (serialCommand.atStep == 1) {
      if (atoi(&c) != 0 || c == '0') {
        serialCommand.angle *= 10;
        serialCommand.angle += atoi(&c);
        continue;
      } else if (c == ' ') {
        serialCommand.atStep++;
        continue;
      }
    }

    if (serialCommand.atStep == 2) {
      if (atoi(&c) != 0 || c == '0') {
        serialCommand.seconds *= 10;
        serialCommand.seconds += atoi(&c);
        continue;
      }
    }

    serialCommand.atStep = 3;
  }
}

void executeSerialCommand() {
  Serial.println("ACK");
  turnstate.left = !serialCommand.right;
  turnstate.right = serialCommand.right;
  setDirection(turnstate.right);
  turnstate.turnlong = true;
  turnstate.target = turnstate.angle + (serialCommand.right ? serialCommand.angle : -1 * serialCommand.angle);
  timer[0].interval = (serialCommand.seconds / (abs(turnstate.target - turnstate.angle) * 10)) * 100000 * 5;
  turnstate.turnlongbyserial = true;
}

void writeLongTurnLed() {
  digitalWrite(LONG_TURN_LED, turnstate.turnlong ? HIGH : LOW);
}

void refreshInput() {
  switches.turnleft     = digitalRead(SW_TURN_LEFT) == HIGH;
  switches.turnright    = digitalRead(SW_TURN_RIGHT) == HIGH;
  switches.thdbleft     = digitalRead(SW_THDB_LEFT) == HIGH;
  switches.thdbright    = digitalRead(SW_THDB_RIGHT) == HIGH;
  switches.zero         = digitalRead(SW_ZERO) == HIGH;
  switches.oet          = digitalRead(SW_OET) == HIGH;
  switches.kill         = digitalRead(SW_KILL) == HIGH;
  switches.speed_low    = digitalRead(SW_SPD_LOW) == HIGH;
  switches.speed_high   = digitalRead(SW_SPD_HIGH) == HIGH;
  if (turnstate.turnlong) {
    if (switches.kill) {
      turnstate.turnlongbyserial = false;
      turnstate.turnlong = false;
      turnstate.left = false;
      turnstate.right = false;
    }
  } else {
    turnstate.left = switches.turnleft;
    turnstate.right = switches.turnright;
    if (switches.zero) {
      turnstate.offset = turnstate.angle;
      turnstate.thdbleft = turnstate.offset;
      turnstate.thdbright = turnstate.offset;
    }
    if (switches.oet) {
      turnstate.turnlong = true;
      if (aboutEquals((turnstate.angle - turnstate.offset), -180, 10) || aboutEquals((turnstate.angle - turnstate.offset), 180, 10)) {
        if (turnstate.angle > 0) {
          turnstate.left = true;
          turnstate.target = turnstate.offset;
        } else {
          turnstate.right = true;
          turnstate.target = turnstate.offset;
        }
      } else {
        if (turnstate.angle > 0) {
          turnstate.left = true;
          turnstate.target = turnstate.offset - 180;
        } else {
          turnstate.right = true;
          turnstate.target = turnstate.offset + 180;
        }
      }
    }
    if (switches.thdbleft) {
      turnstate.thdbleft = turnstate.angle;
    }
    if (switches.thdbright) {
      turnstate.thdbright = turnstate.angle;
    }
    setDirection(turnstate.right);
  }
  if (!turnstate.turnlongbyserial) {
    if (switches.speed_low) {
      timer[0].interval = 25000;
    } else if (switches.speed_high) {
      timer[0].interval = 3000;
    } else {
      timer[0].interval = 8000;
    }
  }
}

void pulse() {
  pulsePin = flipHighLow(pulsePin);
  digitalWrite(OUT_PULSE, pulsePin);
  if (pulsePin == LOW) {
    if (turnstate.right) {
      turnstate.angle += 0.1;
    } else {
      turnstate.angle -= 0.1;
    }
    if (turnstate.turnlong) {
      if (aboutEquals(turnstate.angle, turnstate.target, 0.1)) {
        turnstate.turnlong = false;
        turnstate.turnlongbyserial = false;
        turnstate.left = false;
        turnstate.right = false;
      }
    }
  }
}

void setDirection(bool right) {
  right = !right;
  if (right) {
    digitalWrite(OUT_CW, HIGH);
  } else {
    digitalWrite(OUT_CW, LOW);
  }
}

int flipHighLow(int input) {
  if (input == HIGH) {
    return LOW;
  } else if (input == LOW) {
    return HIGH;
  } else {
    return input;
  }
}

boolean checkTimer(int index) {
  if (timer[index].micro) {
    if ((timer[index].last + timer[index].interval) <= micros()) {
      timer[index].last = micros();
      return true;
    }
  } else {
    if ((timer[index].last + timer[index].interval) <= millis()) {
      timer[index].last = millis();
      return true;
    }
  }
  return false;
}

void output() {
  lcd.setCursor(6, outputCtr);
  if (outputCtr == 0) {
    lcd.print(round(turnstate.angle - turnstate.offset));
    lcd.print("    ");
  } else if (outputCtr == 1) {
    lcd.print(round(turnstate.offset));
    lcd.print("    ");
  } else if (outputCtr == 2) {
    lcd.print(round(turnstate.thdbleft - turnstate.offset));
    lcd.print("    ");
  } else if (outputCtr == 3) {
    lcd.print(round(turnstate.thdbright - turnstate.offset));
    lcd.print("    ");
  }
  outputCtr++;
  if (outputCtr >= 4) outputCtr = 0;
}

boolean aboutEquals(double num1, double num2, double delta) {
  return (num1 < (num2 + delta) && num1 > (num2 - delta));
}
