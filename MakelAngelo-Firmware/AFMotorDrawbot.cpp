//------------------------------------------------------------------------------
// Adafruit Motor shield library, modified for Drawbot
// copyright Dan Royer, 2012
// this code is public domain, enjoy!
//------------------------------------------------------------------------------
#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif
#include "AFMotorDrawbot.h"



//------------------------------------------------------------------------------



static uint8_t latch_state;
static AFMotorController MC;



//------------------------------------------------------------------------------



AFMotorController::AFMotorController() {}


void AFMotorController::enable() {
  // setup the latch
  /*
  LATCH_DDR |= _BV(LATCH);
  ENABLE_DDR |= _BV(ENABLE);
  CLK_DDR |= _BV(CLK);
  SER_DDR |= _BV(SER);
  */
  pinMode(MOTORLATCH, OUTPUT);
  pinMode(MOTORENABLE, OUTPUT);
  pinMode(MOTORDATA, OUTPUT);
  pinMode(MOTORCLK, OUTPUT);

  latch_state = 0;

  latch_tx();  // "reset"

  //ENABLE_PORT &= ~_BV(ENABLE); // enable the chip outputs!
  digitalWrite(MOTORENABLE, LOW);
}


void AFMotorController::latch_tx() {
  uint8_t i;

  //LATCH_PORT &= ~_BV(LATCH);
  digitalWrite(MOTORLATCH, LOW);

  //SER_PORT &= ~_BV(SER);
  digitalWrite(MOTORDATA, LOW);
/*
  for (i=0; i<8; i++) {
    //CLK_PORT &= ~_BV(CLK);
    digitalWrite(MOTORCLK, LOW);
    if (latch_state & _BV(7-i)) {
      //SER_PORT |= _BV(SER);
      digitalWrite(MOTORDATA, HIGH);
    } else {
      //SER_PORT &= ~_BV(SER);
      digitalWrite(MOTORDATA, LOW);
    }
    //CLK_PORT |= _BV(CLK);
    digitalWrite(MOTORCLK, HIGH);
  }
*/
    digitalWrite(MOTORCLK, LOW);    digitalWrite(MOTORDATA, HIGH * ( (latch_state & _BV(7)) >> (7) ) );    digitalWrite(MOTORCLK, HIGH);
    digitalWrite(MOTORCLK, LOW);    digitalWrite(MOTORDATA, HIGH * ( (latch_state & _BV(6)) >> (6) ) );    digitalWrite(MOTORCLK, HIGH);
    digitalWrite(MOTORCLK, LOW);    digitalWrite(MOTORDATA, HIGH * ( (latch_state & _BV(5)) >> (5) ) );    digitalWrite(MOTORCLK, HIGH);
    digitalWrite(MOTORCLK, LOW);    digitalWrite(MOTORDATA, HIGH * ( (latch_state & _BV(4)) >> (4) ) );    digitalWrite(MOTORCLK, HIGH);
    digitalWrite(MOTORCLK, LOW);    digitalWrite(MOTORDATA, HIGH * ( (latch_state & _BV(3)) >> (3) ) );    digitalWrite(MOTORCLK, HIGH);
    digitalWrite(MOTORCLK, LOW);    digitalWrite(MOTORDATA, HIGH * ( (latch_state & _BV(2)) >> (2) ) );    digitalWrite(MOTORCLK, HIGH);
    digitalWrite(MOTORCLK, LOW);    digitalWrite(MOTORDATA, HIGH * ( (latch_state & _BV(1)) >> (1) ) );    digitalWrite(MOTORCLK, HIGH);
    digitalWrite(MOTORCLK, LOW);    digitalWrite(MOTORDATA, HIGH * ( (latch_state & _BV(0)) >> (0) ) );    digitalWrite(MOTORCLK, HIGH);
  //LATCH_PORT |= _BV(LATCH);
  digitalWrite(MOTORLATCH, HIGH);
}



//------------------------------------------------------------------------------
// STEPPERS
//------------------------------------------------------------------------------



AF_Stepper::AF_Stepper(uint16_t steps, uint8_t num) {
  MC.enable();

  revsteps = steps;
  steppernum = num;
  currentstep = 0;

  if (steppernum == 1) {
    latch_state &= ~_BV(MOTOR1_A) & ~_BV(MOTOR1_B) &
                   ~_BV(MOTOR2_A) & ~_BV(MOTOR2_B); // all motor pins to 0
    MC.latch_tx();

    // enable both H bridges
    pinMode(11, OUTPUT);
    pinMode(3, OUTPUT);
    digitalWrite(11, HIGH);
    digitalWrite(3, HIGH);

    a = _BV(MOTOR1_B);
    b = _BV(MOTOR2_B);
    c = _BV(MOTOR1_A);
    d = _BV(MOTOR2_A);
  } else if (steppernum == 2) {
    latch_state &= ~_BV(MOTOR3_A) & ~_BV(MOTOR3_B) &
                   ~_BV(MOTOR4_A) & ~_BV(MOTOR4_B); // all motor pins to 0
    MC.latch_tx();

    // enable both H bridges
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    digitalWrite(5, HIGH);
    digitalWrite(6, HIGH);

    a = _BV(MOTOR3_B);
    b = _BV(MOTOR4_B);
    c = _BV(MOTOR3_A);
    d = _BV(MOTOR4_A);
  }
}


void AF_Stepper::setSpeed(uint16_t rpm) {
  usperstep = 60000000 / ((uint32_t)revsteps * (uint32_t)rpm);
  steppingcounter = 0;
}


void AF_Stepper::release() {
  // release all
  latch_state &= ~a & ~b & ~c & ~d; // all motor pins to 0
  MC.latch_tx();
}


void AF_Stepper::step(uint16_t steps, uint8_t dir) {
  uint32_t uspers = usperstep;

  while (steps--) {
    onestep(dir);
/*
    delay(uspers/1000);  // in ms
    steppingcounter += (uspers % 1000);
    if (steppingcounter >= 1000) {
      delay(1);
      steppingcounter -= 1000;
    }
//*/
    delayMicroseconds(uspers);
  }
}


void AF_Stepper::onestep(uint8_t dir) {
  if (dir == FORWARD) {
    currentstep++;
  } else {
    // BACKWARDS
    currentstep--;
  }

  currentstep += MICROSTEPS*4;
  currentstep %= MICROSTEPS*4;

#ifdef MOTORDEBUG
  Serial.print("current step: "); Serial.println(currentstep, DEC);
#endif

  // set all of this motor's pins to 0 (don't smash other motor)
  latch_state &= ~a & ~b & ~c & ~d;

  // No wait!  Keep some energized.
  switch (currentstep/(MICROSTEPS/2)) {
  case 0:  latch_state |= a;      break;  // energize coil 1 only
  case 1:  latch_state |= a | b;  break;  // energize coil 1+2
  case 2:  latch_state |= b;      break;  // energize coil 2 only
  case 3:  latch_state |= b | c;  break;  // energize coil 2+3
  case 4:  latch_state |= c;      break;  // energize coil 3 only
  case 5:  latch_state |= c | d;  break;  // energize coil 3+4
  case 6:  latch_state |= d;      break;  // energize coil 4 only
  case 7:  latch_state |= d | a;  break;  // energize coil 1+4
  }

  // change the energized state now
  MC.latch_tx();
}

