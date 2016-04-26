/*
  LiquidCrystal Library - Hello World

 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch prints "Hello World!" to the LCD
 and shows the time.

  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe

 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystal
 */

// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
//LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
LiquidCrystal   lcd(12, 11,  5,  4,  6, 10);

void setup() {

  lcd.begin(8, 2);
  lcd.clear();

}

void loop() {

  delay(1000);

  int value = B10000001;

  for (int x = 4 ; x < 8 ; x++) {

    value = 1 << x;

    binary(value);
    lcd.setCursor(7, 1);
    lcd.write(value);
    delay(200);
    
  }

}

void binary(unsigned char theValue) {

  lcd.setCursor(0, 0);

  for (int xx = 0 ; xx < 8 ; xx++) {

    if (theValue & B10000000) {
      lcd.write(49);      
    }
    else {
      lcd.write(48);     
    }

    theValue <<= 1;
    
  }

}




