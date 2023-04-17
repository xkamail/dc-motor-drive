#include <LCD_I2C.h>
#include <EasyScheduler.h>
LCD_I2C lcd(0x3F, 16, 2);
#define H 1
#define L 0
#define AIN1 0
#define AIN2 0
#define ASTBY 0
#define BIN1 0
#define BIN2 0
#define BSTBY 0

Schedular LCDTask;
Schedular PWMTask;
Schedular CycleTask;

enum Motor {
  A,
  B,
};

enum Direction {
  CW,
  CCW,
};

// global state of motor
volatile Direction _dirA = CW;
volatile Direction _dirB = CW;

void setup() {
  lcd.begin();
  lcd.backlight();
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  LCDTask.start();
  PWMTask.start();

  // Fast PWM Mode Top=0xFF
  TCCR0A = _BV(WGM01) | _BV(WGM00);
  // non-inverting mode compare output
  TCCR0A |= _BV(COM0A1) | _BV(COM0B1);
  // clk
  TCCR0B |= _BV(CS01);
  Serial.begin(9600);
  Serial.println("Starting....");
  delay(2000);
  CycleTask.start();
}

void loop() {
  LCDTask.check(updateLCD, 100);
  PWMTask.check(handleReadValue, 1000);
}

void writeSpeed(Motor m, long duty) {
  long val = map(duty, 0, 100, 0, 255);
  if (m == A) {
    OCR0A = val;  // pin=6
    return;
  }
  OCR0B = val;  // pin=5
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

void handleReadValue() {
  long x = analogRead(A0);
}

void updateLCD() {
  char buf[16];  // lcd 16 characters
  lcd.setCursor(0, 0);
  int dutyA = map(analogRead(A0), 0, 1023, 0, 100);
  snprintf(buf, 16, "A: %3d%% %s", dutyA, _dirA == CW ? "CW" : "CCW");
  lcd.print(buf);
  lcd.setCursor(0, 1);
  int dutyB = map(analogRead(A1), 0, 1023, 0, 100);
  snprintf(buf, 16, "B: %3d%% %s", dutyB, _dirA == CW ? "CW" : "CCW");
  lcd.print(buf);
}

// cmd represents control signal to IC
void cmd(Motor m, unsigned int in1, unsigned in2, unsigned stby) {
  digitalWrite(m == A ? AIN1 : BIN1, in1);
  digitalWrite(m == A ? AIN2 : BIN2, in2);
  digitalWrite(m == A ? ASTBY : BSTBY, stby);
}