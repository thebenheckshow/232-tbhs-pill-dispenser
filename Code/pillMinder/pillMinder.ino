#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include "HX711.h"

LiquidCrystal   lcd(12, 11,  5,  4,  6, 10);

#define DOUT  3
#define CLK  2

HX711 scale(DOUT, CLK);

float calibration_factor = -150; 				//-7050 worked for my 440lb max scale setup
float calibration_factor_old = calibration_factor;
float scaleValue = 0.0;

int blinker = 0;								//Blinks the colon on time
int alarmBuzzer = 0;

float weight[15];								//AM and PM, 7 days a week. The 15th value is the empty (tare) weight we'll compare Saturday PM against
float currentWeight = 0;
int currentWeightSamples = 0;
float currentAverage = 0;
float difference = 0;
unsigned char flag[15];							//The flag for each event. Once pills taken, clear flag

int amAlarm = 6;
int pmAlarm = 19;

int amPMflag = 0;
int dayArray = 0;

int latchPin = A3;
int clockPin = A1;
int dataPin = A2;
 
#define buzzer		9
#define menuButton	8
#define loadButton	7

unsigned char dayLED[] = {0, 15, 1, 14, 2, 13, 3, 12, 4, 11, 5, 10, 8, 9};

void setup() {
	
	Serial.begin(9600);
	while (!Serial) ; // wait for serial
	
	Serial.println("HX711 calibration sketch");
	Serial.println("Remove all weight from scale");
	Serial.println("After readings begin, place known weight on scale");
	Serial.println("Press + or a to increase calibration factor");
	Serial.println("Press - or z to decrease calibration factor");

	scale.set_scale();
	//scale.tare();  //Reset the scale to 0

	long zero_factor = scale.read_average(); //Get a baseline reading

	Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.

	Serial.println(zero_factor);
	scale.set_scale(calibration_factor);
  
	lcd.begin(8, 2);
	lcd.clear();  

	pinMode(buzzer, OUTPUT);
	
	pinMode(menuButton, INPUT);
	pinMode(loadButton, INPUT);
	digitalWrite(menuButton, 1);
	digitalWrite(loadButton, 1);
 
	pinMode(latchPin, OUTPUT);
	pinMode(clockPin, OUTPUT);
	pinMode(dataPin, OUTPUT);

	delay(500);
	
}

void loop() {

	tmElements_t tm;
	blinker += 1;
		
	if (blinker < 80) {
	
		if (RTC.read(tm)) {
					
			lcd.setCursor(0,0);			
			
			printHour(tm.Hour);
			
			if (tm.Second & 1) {
				lcd.write(':');				
			}
			else {
				lcd.write(' ');						
			}

			print2digitsLCD(tm.Minute);
			
			lcd.setCursor(5, 0);
			lcd.print(" ");
			printAMPM(0);		
			
			lcd.setCursor(0, 1);
			lcd.print(" ");
			printDay(tm.Wday);
			//lcd.print(tm.Wday);
			//printDay(tm.Wday);
			
			lcd.print("  ");
			
			
		}
				
	}
	else {

		lcd.setCursor(0, 0);
		lcd.print("NEXT:   ");		
		lcd.setCursor(6, 0);
		RTC.read(tm);

		int showWhich = 0;
		
		if (tm.Hour < pmAlarm) {
			showWhich = 1;
		}
		
		if (tm.Hour < amAlarm) {
			showWhich = 0;			
		}
		
		if (showWhich == 0) {
			if (amAlarm == 0) {
				print2digitsLCD(amAlarm + 12);					
			}
			else {
				print2digitsLCD(amAlarm);					
			}			
			lcd.setCursor(0, 1);		
			lcd.print(":00 ");						
			lcd.print("AM");							
		}
		else {
			if (pmAlarm > 12) {					
				print2digitsLCD(pmAlarm - 12);	
			}
			else {
				print2digitsLCD(pmAlarm);						
			}			
			lcd.setCursor(0, 1);		
			lcd.print(":00 ");						
			lcd.print("PM");							
		}
		
	}

	if (blinker > 160) {
		
		blinker = 0;
	}
	
	delay(50);

	int whichArray = (dayArray * 2) + amPMflag;	//Checks which value we should be looking at depending on day of week and AM/PM 
	
	if (tm.Hour == amAlarm or tm.Hour == pmAlarm) {
		
		if (flag[whichArray] == 0) {						//No flag set? First time we've hit this event
		
			if (checkWeight(whichArray) == 255) {			//Does weight match the NEXT pill?
				flag[whichArray] = 1;						//Pills taken, set flag not to do this again
				noTone(buzzer);
				Serial.println("PILLS TAKEN!");				
			}
			else {
				alarm();				
			}
		}
				
	}
	else {
		loadLED(dayLED[whichArray]);		
	}
	
	if (digitalRead(menuButton) == 0) {
		waitMenu();
		menu();									//Jump to menu
		blinker = 0;							//Reset this for clean transistion
	}
	
}

int checkWeight(unsigned char whichOne) {

	currentWeight = scale.get_units(50);
	
	Serial.print(whichOne);
	Serial.print(" ");
	Serial.println(currentWeight);
	
	Serial.print(weight[whichOne] - difference);
	Serial.print(" ");
	Serial.print(weight[whichOne] + difference);
	
	if ((currentWeight < (weight[whichOne] - difference)) and (currentWeight > (weight[whichOne] + difference))) {
		Serial.println(" MATCH CURRENT");
		return 0;		
	}
	
	if ((currentWeight < (weight[whichOne + 1] - difference)) and (currentWeight > (weight[whichOne + 1] + difference))) {
		Serial.println(" MATCH NEXT");
		return 255;		
	}
	
	/*
	//If it's still near its weight, then pills haven't been taken yet	
	if (currentWeight > (weight[whichOne] - difference) and currentWeight < (weight[whichOne] + difference)) {
		Serial.println("MATCH CURRENT");
		return 1;		
	}
	
	//Check if it's close to the weight of the NEXT reading
	if (currentWeight > (weight[whichOne + 1] - difference) and currentWeight < (weight[whichOne + 1] + difference)) {
		Serial.println("MATCH NEXT");
		return 255;		
	}

	*/
	
}

void print2digitsLCD(int number) {
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void printHour(int whichHour) {

	amPMflag = 0;				//Default is AM

	if (whichHour > 11) {
		amPMflag = 1;		
	}
	if (whichHour > 12) {
		whichHour -= 12;
	}
	
	print2digitsLCD(whichHour);	
	
}

void printAMPM(unsigned char op) {
	
	int printWhich = 0;
	
	if (op) {		
		printWhich = op;		
	}
	else {	
		printWhich = amPMflag + 1;
	}

	if (printWhich == 1) {
		lcd.print("AM");		
	}
	if (printWhich == 2) {
		lcd.print("PM");		
	}
		
}

void printDay(unsigned char whichDay) {
	
	switch(whichDay) {
		
		case 1:
			lcd.print("SUN");
		break;
		case 2:
			lcd.print("MON");
		break;			
		case 3:
			lcd.print("TUE");
		break;			
		case 4:
			lcd.print("WED");
		break;
		case 5:
			lcd.print("THU");
		break;			
		case 6:
			lcd.print("FRI");
		break;
		case 7:
			lcd.print("SAT");
		break;							
	}		
	
	dayArray = whichDay - 1;
	
}

void menu() {

	lcd.clear();
	
	int inMenu = 1;
	tone(buzzer, 1000, 50);
	
	while(inMenu) {

		delay(20);

		if (digitalRead(menuButton) == 0) {
			tone(buzzer, 1000, 50);
			waitMenu();
			inMenu += 1;
			lcd.clear();
			
			if (inMenu > 4) {
				inMenu = 1;				
			}
			
		}
		
		if (digitalRead(loadButton) == 0) {
			tone(buzzer, 2000, 50);
			waitLoad();
			switch(inMenu) {
				
				case 1:
					amAlarm += 1;
					if (amAlarm > 11) {
						amAlarm = 0;						
					}
				break;
				case 2:
					pmAlarm += 1;
					if (pmAlarm > 23) {
						pmAlarm = 12;						
					}		
				break;
				case 3:
					loadPills();
					inMenu = 0;	
				break;
				case 4:
					inMenu = 0;		
				break;			
				
			}
			
		}	
		
		switch(inMenu) {
			
			case 1:
				lcd.setCursor(0, 0);
				lcd.print("AM ALARM");
				lcd.setCursor(1, 1);					
				if (amAlarm == 0) {
					print2digitsLCD(amAlarm + 12);					
				}
				else {
					print2digitsLCD(amAlarm);					
				}		
				lcd.print(":00 ");	
			break;
			case 2:
				lcd.setCursor(0, 0);
				lcd.print("PM ALARM");
				lcd.setCursor(1, 1);					
				if (pmAlarm > 12) {					
					print2digitsLCD(pmAlarm - 12);	
				}
				else {
					print2digitsLCD(pmAlarm);						
				}	
				lcd.print(":00 ");			
			break;
			case 3:
				lcd.setCursor(0, 0);
				lcd.print("LOAD PIL");
				lcd.setCursor(0, 1);					
				lcd.print("LS?");				
			break;
			case 4:
				lcd.setCursor(0, 0);
				lcd.print("EXIT?");			
			break;			
			
		}
						
	}

	lcd.clear();
	
}

void alarm() {
	
	if (alarmBuzzer < 2) {
		tone(buzzer, 1500);				
	}
	else {
		tone(buzzer, 2500);				
	}	
	
	alarmBuzzer += 1;
	
	if (alarmBuzzer > 4) {
		alarmBuzzer = 0;		
	}

}

void loadPills() {
	
	loadLED(128);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("WEIGHING");
	lcd.setCursor(0, 1);
	lcd.print("....");
	
	weight[14] = scale.get_units(100);			//Get tare weight

	difference = 0;
	
	Serial.println(weight[14]);
	
	for (int day = 7 ; day > 1 ; day--) {
			
		for (int half = 1 ; half > -1 ; half--) {

			lcd.clear();
			
			lcd.setCursor(0, 0);
			lcd.print("LOAD ");
			printDay(day);
		
			//loadLED(((day - 1) * 2) + half);
			
			lcd.setCursor(1, 1);

			if (half == 0) {				
				lcd.print("AM");	
			}
			else {
				lcd.print("PM");								
			}
			
			while(digitalRead(loadButton) == 1) {		//Wait for button press
				loadLED(dayLED[((day - 1) * 2) + half]);			
				lcd.setCursor(4, 1);
				lcd.print("#");
				tone(buzzer, 2000, 5);
				delay(195);
				loadLED(128);
				lcd.setCursor(4, 1);
				lcd.print(" ");
				delay(200);				
			}
			tone(buzzer, 2000, 50);
			waitLoad();

			lcd.clear();
			
			lcd.setCursor(0, 0);
			lcd.print("WEIGHING");
			lcd.setCursor(0, 1);
			lcd.print("....");
			
			weight[((day - 1) * 2) + half] = scale.get_units(100);
			
			Serial.println(weight[((day - 1) * 2) + half]);

			difference += (weight[((day - 1) * 2) + half] - weight[((day - 1) * 2) + half + 1]);
			
			Serial.println(difference);
			
			//lcd.setCursor(0, 0);
			//lcd.print(weight[((day - 1) * 2) + half]);
			//delay(1000);
			
		}
			
	}
	
	difference /= 4; //14;	
	Serial.println(difference);	
	difference *= .4;	
	Serial.println(difference);

	for (int x = 0 ; x < 15 ; x++) {
		
		flag[x] = 0;
		Serial.println(weight[x]);
				
	}
	
}

void waitMenu() {
	
	while(digitalRead(menuButton) == 0) {
		delay(10);			
	}	
	
}

void waitLoad() {
	
	while(digitalRead(loadButton) == 0) {
		delay(10);			
	}	
	
}

void loadLED(unsigned whichBit) {

	unsigned short theWord = 0;

	if (whichBit == 255) {			//All
		theWord = 65535;
	}
	if (whichBit == 128) {			//None
		theWord = 0;		
	}	
	if (whichBit < 128) {
		theWord = (1 << whichBit);			
	}
	
    digitalWrite(latchPin, LOW); 	
	shiftOut(dataPin, clockPin, MSBFIRST, theWord & 0xFF);
	shiftOut(dataPin, clockPin, MSBFIRST, theWord >> 8);
    digitalWrite(latchPin, HIGH);

	
}


