#include <LCD_I2C.h>

LCD_I2C lcd(0x3F, 16, 2);

void setup() {
  lcd.begin();
  lcd.backlight();
}

void loop() {
  lcd.print("Hello");
  lcd.print("World!");
}