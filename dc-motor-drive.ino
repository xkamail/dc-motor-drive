#include <LCD_I2C.h>
#include <EasyScheduler.h>
LCD_I2C lcd(0x3F, 16, 2);
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

  Serial.begin(9600);
  Serial.println("Starting....");
  delay(2000);
  CycleTask.start();
  run();
}

void loop() {
  LCDTask.check(updateLCD, 100);
  PWMTask.check(handleReadValue, 1000);
}

// run PWM
void run() {
  // clk
  TCCR0B |= _BV(CS01);
}

void writeSpeed(Motor m, long duty) {
  long val = map(duty, 0, 100, 0, 255);
  if (m == A) {
    OCR0A = val;  // pin=6
  } else {
    OCR0B = val;  // pin=5
  }
}

void handleReadValue() {
  Serial.print("value=");
  long x = analogRead(A0);
  Serial.print(map(x, 0, 1023, 0, 100));
  Serial.println("");
}

void updateLCD() {
  char buf[16];  // lcd 16 characters
  lcd.setCursor(0, 0);
  int dutyA = map(analogRead(A0), 0, 1023, 0, 100);
  snprintf(buf, 16, "A: %3d%% CCW", dutyA);
  lcd.print(buf);
  lcd.setCursor(0, 1);
  int dutyB = map(analogRead(A1), 0, 1023, 0, 100);
  snprintf(buf, 16, "B: %3d%% CCW", dutyB);
  lcd.print(buf);
}