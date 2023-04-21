#include <LCD_I2C.h>
#include <EasyScheduler.h>
#include "defination.h"
#include <ezButton.h>

LCD_I2C lcd(0x3F, 16, 2);

Schedular LCDTask;
Schedular PWMTask;
Schedular BtnTask;

ezButton btn2(STOP_STBY_A);
ezButton btn8(STOP_STBY_B);

enum Motor : uint8_t {
  A,
  B,
};
enum Direction : uint8_t {
  CW,
  CCW,
};
enum MotorMode : uint8_t {
  MotorStanby,
  MotorStop,
  MotorCW,
  MotorCCW,
  MotorBrake,
};
char* motorModeStr[5] = {
  "STANBY",
  "STOP  ",
  "CW.   ",
  "CCW.  ",
  "BRAKE "
};

// global state of motor
volatile MotorMode motorA = MotorStanby;
volatile MotorMode motorB = MotorStanby;
volatile MotorMode motorABeforeBrake;
volatile MotorMode motorBBeforeBrake;

void setup() {
  DDRD = 0b01100011;   // input internal pull-up
  PORTD = 0b10011100;  // for motor A
  DDRB = 0b110000;     // input internal pull-up
  PORTB = 0b00001111;  // for motor B
  DDRC = 0b1100;       // PWM analog read amd A3, A2
  btn2.setDebounceTime(50);
  btn8.setDebounceTime(50);
  lcd.begin();
  lcd.backlight();
  LCDTask.start();

  TCCR0A = _BV(WGM01) | _BV(WGM00);     // Fast PWM Mode Top=0xFF
  TCCR0A |= _BV(COM0A1) | _BV(COM0B1);  // non-inverting mode compare output
  TCCR0B |= _BV(CS01);                  // clk
  delay(2000);                          // wait
  PWMTask.start();
  BtnTask.start();
}


void loop() {
  btn2.loop();
  btn8.loop();
  LCDTask.check(updateLCD, 200);  // 30Hz
  PWMTask.check(handleReadValue, 300);

  userControl(A, !digitalRead(BRAKEA), !digitalRead(CW_A), !digitalRead(CCW_A), btn2.isReleased());

  userControl(B, !digitalRead(BRAKEB), !digitalRead(CW_B), !digitalRead(CCW_B), btn8.isReleased());
}

void userControl(Motor m, bool isBrake, bool isCW, bool isCCW, bool pressStopStby) {
  MotorMode currentMode = m == A ? motorA : motorB;
  volatile MotorMode* t = m == A ? &motorA : &motorB;
  if ((currentMode == MotorCCW || currentMode == MotorCW) && isBrake) {
    m == A ? motorABeforeBrake = currentMode : motorBBeforeBrake = currentMode;
    *t = MotorBrake;
    motorShortBreak(m);
    return;
  }

  switch (currentMode) {
    case MotorStanby:
      if (pressStopStby) {
        *t = MotorStop;
        motorStop(m);
      }
      break;
    case MotorStop:
      if (pressStopStby) {
        *t = MotorStanby;
        motorStandBy(m);
      }
      if (isCW) {
        *t = MotorCW;
        motorDirection(m, CW);
      }
      if (isCCW) {
        *t = MotorCCW;
        motorDirection(m, CCW);
      }
      break;
    case MotorCCW:
      if (isCW) {
        *t = MotorCW;
        motorDirection(m, CW);
        return;
      }
      if (pressStopStby) {
        *t = MotorStop;
        motorStandBy(m);
      }
      break;
    case MotorCW:
      if (isCCW) {
        *t = MotorCCW;
        motorDirection(m, CCW);
        return;
      }
      if (pressStopStby) {
        *t = MotorStop;
        motorStandBy(m);
        return;
      }
      break;
    case MotorBrake:  //
      if (isBrake) return;
      if (m == A) {
        motorA = motorABeforeBrake;
        motorDirection(m, motorABeforeBrake == MotorCW ? CW : CCW);
      } else {
        motorB = motorBBeforeBrake;
        motorDirection(m, motorBBeforeBrake == MotorCW ? CW : CCW);
      }
      break;
  }
}

// motor control functions
void motorStandBy(Motor m) {
  cmd(m, L, L, L);
}
void motorStop(Motor m) {
  cmd(m, L, L, H);
}
void motorDirection(Motor m, Direction d) {
  if (d == CW) {  // CW + at O1
    cmd(m, H, L, H);
    return;
  }
  cmd(m, L, H, H);
}
void motorShortBreak(Motor m) {
  cmd(m, H, H, H);
}
// ----

void handleReadValue() {
  if (motorA == MotorCCW || motorA == MotorCW) {
    long x = map(analogRead(PWMA), 0, 1024, 0, 256);
    OCR0A = x > 250 ? 0xFF : x;
  } else {
    OCR0A = 0;
  }
  if (motorB == MotorCCW || motorB == MotorCW) {
    long x = map(analogRead(PWMB), 0, 1024, 0, 256);
    OCR0B = x > 250 ? 0xFF : x;
  } else {
    OCR0B = 0;
  }
}


// convert motor state into human text
void getMotorText(char* __s, Motor m) {
  int duty = map(m == A ? OCR0A : OCR0B, 0, 0xFF, 0, 100);
  snprintf(__s, 16, "%c: %3d%% %s", m == A ? 'A' : 'B', duty,
           motorModeStr[m == A ? motorA : motorB]);
}

char buf[16];  // lcd 16 characters
void updateLCD() {
  lcd.setCursor(0, 0);
  getMotorText(buf, A);
  lcd.print(buf);
  lcd.setCursor(0, 1);
  getMotorText(buf, B);
  lcd.print(buf);
}

// cmd represents control signal to IC
void cmd(Motor m, unsigned int in1, unsigned in2, unsigned stby) {
  digitalWrite(m == A ? AIN1 : BIN1, in1);
  digitalWrite(m == A ? AIN2 : BIN2, in2);
  digitalWrite(m == A ? ASTBY : BSTBY, stby);
}