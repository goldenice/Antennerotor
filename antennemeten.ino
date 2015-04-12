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

// Defining the timers used in the program
struct Timer {
  unsigned long last;
  unsigned long interval;
  boolean micro;
};
Timer timer[10];

int pulsePin = LOW;

struct TurnState {
  boolean turnlong;
  boolean left;
  boolean right;
  float angle;
  float offset;
  float target;
  float thdbleft;
  float thdbright;
};
TurnState turnstate = (TurnState) { false, false, false, 0.0, 0.0, 0.0, 0.0, 0.0 };

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
  
  timer[0] = { micros(), 8000, true };
  timer[1] = { millis(), 100, false };
  timer[2] = { millis(), 200, false };
  
  Serial.begin(38400);
}

void loop() {
  if (checkTimer(0)) {
    if (turnstate.left || turnstate.right || pulsePin == HIGH){
      pulse();
    }
  }
  if (checkTimer(1)) {
    refreshInput();
  }
  if (checkTimer(2)) {
    output();
  }
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
      turnstate.turnlong = false;
      turnstate.left = false;
      turnstate.right = false;
    }
  } else {
    turnstate.left = switches.turnleft;
    turnstate.right = switches.turnright;
    if (switches.zero) {
      turnstate.offset = turnstate.angle;
    }
    if (switches.oet) {
      turnstate.turnlong = true;
      if (turnstate.angle > 0) {
        turnstate.turnlong = true;
        turnstate.left = true;
        turnstate.target = turnstate.angle - 180;
      } else {
        turnstate.turnlong = true;
        turnstate.right = true;
        turnstate.target = turnstate.angle + 180;
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
  if (switches.speed_low) {
    timer[0].interval = 25000;
  } else if (switches.speed_high) {
    timer[0].interval = 3000;
  } else {
    timer[0].interval = 8000;
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
      if (turnstate.left && turnstate.angle < turnstate.target) {
        turnstate.turnlong = false;
        turnstate.left = false;
      } else if (turnstate.right && turnstate.angle > turnstate.target) {
        turnstate.turnlong = false;
        turnstate.right = false;
      }
    }
  }
}

void setDirection(bool right) {
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
  Serial.print("A: ");
  Serial.print( turnstate.angle - turnstate.offset );
  Serial.print(" T: ");
  Serial.print( turnstate.target - turnstate.offset );
  Serial.print(" -3dB: ");
  Serial.print( turnstate.thdbleft );
  Serial.print(" 3dB: ");
  Serial.println( turnstate.thdbright );
}
