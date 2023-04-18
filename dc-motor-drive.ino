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

enum Motor {
  A,
  B,
};
enum Direction {
  CW,
  CCW,
};
enum MotorMode {
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
  DDRB = 0b00110000;   // input internal pull-up
  PORTB = 0b00001111;  // for motor B
  DDRC = 0b111100;     // PWM analog read
  btn2.setDebounceTime(50);
  btn8.setDebounceTime(50);
  lcd.begin();
  lcd.backlight();
  LCDTask.start();

  TCCR0A = _BV(WGM01) | _BV(WGM00);     // Fast PWM Mode Top=0xFF
  TCCR0A |= _BV(COM0A1) | _BV(COM0B1);  // non-inverting mode compare output
  TCCR0B |= _BV(CS01);                  // clk
  Serial.begin(9600);                   // debug serial
  Serial.println("Starting....");
  delay(2000);  // wait
  PWMTask.start();
  BtnTask.start();
}

unsigned long lastDebounceTime = 0;
int lastButtonState = HIGH;

void loop() {
  btn2.loop();
  btn8.loop();
  LCDTask.check(updateLCD, 100);
  PWMTask.check(handleReadValue, 100);
  bool A_brake = !digitalRead(BRAKEA);
  bool A_CW = !digitalRead(CW_A);
  bool A_CCW = !digitalRead(CCW_A);
  bool A_TOGGLE = btn2.isReleased();
  userControl(A, A_brake, A_CW, A_CCW, A_TOGGLE);
  bool B_brake = !digitalRead(BRAKEB);
  bool B_CW = !digitalRead(CW_B);
  bool B_CCW = !digitalRead(CCW_B);
  bool B_TOGGLE = btn8.isReleased();
  if (B_TOGGLE || B_brake || B_CW || B_CCW) {
    Serial.println("B toggle");
  }
  userControl(B, B_brake, B_CW, B_CCW, B_TOGGLE);
}

void handleButtons() {
}

void userControl(Motor m, bool isBrake, bool isCW, bool isCCW, bool pressStopStby) {
  MotorMode currentMode = m == A ? motorA : motorB;
  volatile MotorMode* t = m == A ? &motorA : &motorB;
  if ((currentMode == MotorCCW || currentMode == MotorCW) && isBrake) {
    m == A ? motorABeforeBrake = currentMode : motorBBeforeBrake = currentMode;
    *t = MotorBrake;
    return;
  }

  switch (currentMode) {
    case MotorStanby:
      if (pressStopStby) {
        *t = MotorStop;
      }
      break;
    case MotorStop:
      if (pressStopStby) {
        *t = MotorStanby;
      }
      if (isCW) {
        *t = MotorCW;
      }
      if (isCCW) {
        *t = MotorCCW;
      }
      break;
    case MotorCCW:
      if (isCW) {
        *t = MotorCW;
        return;
      }
      if (pressStopStby) {
        *t = MotorStop;
      }
      break;
    case MotorCW:
      if (isCCW) {
        *t = MotorCCW;
        return;
      }
      if (pressStopStby) {
        *t = MotorStop;
        return;
      }
      break;
    case MotorBrake:  //
      if (isBrake) return;
      m == A ? motorA = motorABeforeBrake : motorB = motorBBeforeBrake;
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
    OCR0A = map(analogRead(PWMA), 0, 1023, 0, 255);
  } else {
    OCR0A = 0;
  }
  if (motorB == MotorCCW || motorB == MotorCW) {
    OCR0B = map(analogRead(PWMB), 0, 1023, 0, 255);
  } else {
    OCR0B = 0;
  }
}


// convert motor state into human text
void getMotorText(char* __s, Motor m) {
  int dutyA = map(m == A ? OCR0A : OCR0B, 0, 255, 0, 100);
  snprintf(__s, 16, "%c: %3d%% %s",
           m == A ? 'A' : 'B',
           dutyA,
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