/*========================================*/
/*             ARDUINO PINOUT             */
/*========================================*/
/* - A0 --> Battery level.                */
/* - D0 -->                               */
/* - D1 -->                               */
/* - D2 --> Encoder CLOCK.                */
/* - D3 --> Encoder DATA.                 */
/* - D4 --> Encoder PUSHBUTTON.           */
/* - D5 --> Buzzer.                       */
/* - D6 -->                               */
/* - D7 --> LCD E.                        */
/* - D8 --> LCD RS.                       */
/* - D9 --> LCD backlight ON/OFF.         */
/* - D10--> LCD DB7.                      */
/* - D11--> LCD DB6.                      */
/* - D12--> LCD DB5.                      */
/* - D13--> LCD DB4.                      */
/*========================================*/

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

/*========================================*/
/*             DATA MANAGEMENT            */
/*========================================*/

#define ADDR_SPD    0    // Speed 1 byte (0 - 255).
#define ADDR_TMR    1    // Delay time 2 byte (0 - 900).

struct settings
{
  int     motorSpeed;
  int     delayTime;
};

settings userSettings;


struct Camera {
    int timer = 10;
};

Camera camera;

int cameraTimer = 10;
int cameraShots = 1;
bool cameraRun = false;
String cameraRunString = "False";


void loadEEPROM() {
  // Only speed, delay time and heater mode are stored in EEPROM.
  userSettings.motorSpeed = EEPROM.read(ADDR_SPD);
  userSettings.delayTime = (EEPROM.read(ADDR_TMR) << 8) | EEPROM.read(ADDR_TMR + 1);
}

void writeEEPROM(int address, int value) {
  // EEPROM takes 3.3ms to write a cell.
  // Timer data requires 2 byte (storing mode: little endian).
  if(address == ADDR_TMR) {
    EEPROM.write(address, highByte(value));
    delay(5);
    EEPROM.write(address + 1, lowByte(value));
    delay(5);
  }else{
    // Other data requires only 1 byte.
    EEPROM.write(address, byte(value));
    delay(5);
  }
}

/*========================================*/
/*               SLEEP MODE               */
/*========================================*/

ISR(WDT_vect) {
  // Dummy function to avoid microcontroller reset.
}

void enterSleep(int sleepTime) {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  wdt_enable(sleepTime);
  // Now enter sleep mode.
  sleep_mode();
  // The program will continue from here after the WDT timeout.
  wdt_disable();
  // First thing to do is disable sleep.
  sleep_disable();
}
/*
void setupWDT() {
  //Clear the reset flag.
  MCUSR &= ~ (1 << WDRF);
  //In order to change WDE or the prescaler, we need to
  //set WDCE (This will allow updates for 4 clock cycles).
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  //set new watchdog timeout prescaler value (1.0 seconds)
  WDTCSR = 1 << WDP1 | 1 << WDP2;
  //Enable the WD interrupt (note no reset).
  WDTCSR |= _BV(WDIE);
}

/*========================================*/
/*             LCD MANAGEMENT             */
/*========================================*/

#define PIN_BACKLIGHT   9

#define M_MAIN          00
#define M_SETTINGS      90
#define M_CAMERA        91
#define M_TELESCOPE     92

#define S_HEAT          0
#define S_ARROW         1
#define S_CHECKSUM      2
#define S_TIMEOUT       3
#define S_ERROR         4

uint8_t symbol_heat[8]      = {0x1F,0x11,0x1F,0x11,0x1F,0x11,0x1F,0x00};
uint8_t symbol_arrow[8]     = {0x00,0x04,0x1E,0x1F,0x1E,0x04,0x00,0x00};
uint8_t symbol_checksum[8]  = {0x1F,0x19,0x17,0x17,0x17,0x19,0x1F,0x1F};
uint8_t symbol_timeout[8]   = {0x1F,0x11,0x1B,0x1B,0x1B,0x1B,0x1F,0x1F};
uint8_t symbol_error[8]     = {0x1F,0x11,0x17,0x13,0x17,0x11,0x1F,0x1F};

LiquidCrystal lcd(8, 7, 13, 12, 11, 10);

int pointingRow = 0;
int menu = M_SETTINGS;
bool actionRequest = false;
bool editingParameter = false;
int modifier = 0;

void loadCustomSymbols() {
  lcd.createChar(S_HEAT, symbol_heat);
  lcd.createChar(S_ARROW, symbol_arrow);
  lcd.createChar(S_CHECKSUM, symbol_checksum);
  lcd.createChar(S_TIMEOUT, symbol_timeout);
  lcd.createChar(S_ERROR, symbol_error);
}

void scrollItems(String items[], int numOfItems) {

  lcd.clear();
  // Limit row index.
  if (pointingRow < 0) {
    pointingRow = 0;
  } else if (pointingRow > numOfItems - 1) {
    pointingRow = numOfItems - 1;
  }
  // Update pointer position.
  int column = 0;
  if (editingParameter) {
    column = 7;
  } else {
    column = 0;
  }
  // Scroll through items.
  if (pointingRow < 2) {
    lcd.setCursor(1, 0);
    lcd.print(items[0]);
    lcd.setCursor(1, 1);
    lcd.print(items[1]);
  } else if ((pointingRow < numOfItems) && (pointingRow % 2 == 0)) {
    lcd.setCursor(1, 0);
    lcd.print(items[pointingRow]);
    lcd.setCursor(1, 1);
    lcd.print(items[pointingRow + 1]);
  } else if ((pointingRow < numOfItems) && (pointingRow % 2 != 0)) {
    lcd.setCursor(1, 0);
    lcd.print(items[pointingRow - 1]);
    lcd.setCursor(1, 1);
    lcd.print(items[pointingRow]);
  }
  // Arrow animation.
  if (pointingRow % 2 == 0) {
    lcd.setCursor(column, 0);
    lcd.write(byte(S_ARROW));
  } else {
    lcd.setCursor(column, 1);
    lcd.write(byte(S_ARROW));
  }
  
  // DEBUG.
  //lcd.setCursor(15, 0);
  //lcd.print(pointingRow);
  //lcd.setCursor(14, 1);
  //lcd.print(menu);
}

void navigateMenu(int menuID) {

  switch (menuID) {
	  
    case M_SETTINGS: {
      String settings[2] = {"CAMERA", "TELESCOPE"};
	  int numOfItems = sizeof(settings) / sizeof(String);
	  if (actionRequest) {
		// Handle button pressure.
		actionRequest = false;
	    switch (pointingRow) {
	      case 0: {
			menu = M_CAMERA;
			pointingRow = 0;
		    break;
		  }
		  case 1: {
			menu = M_TELESCOPE;
			pointingRow = 0;
		    break;
		  }
		}
	    navigateMenu(menu);
	  } else {
		// Scroll item list.  
        scrollItems(settings, numOfItems);
      }
	  break;
	}
	
    case M_CAMERA: {
        // Handle parameter edit.
        if (editingParameter) {
            switch (pointingRow) {
                case 0: {
                    if (cameraTimer + modifier > 0) {
                        cameraTimer += modifier;
                    }
                    break;
                }
                case 1: {
                    if (cameraShots + modifier > 0) {
                        cameraShots += modifier;
                    }
                    break;
                }
                case 2: {
                    if( modifier != 0) {
                        if (cameraRun) {
                            cameraRun = false;
                            cameraRunString = "False";
                        } else {
                            cameraRun = true;
                            cameraRunString = "True";
                        }
                    }
                    break;
                }
            }
            
        }
        String camera[4] = {"TIMER: " + String(cameraTimer) + "s", "SHOTS: " + String(cameraShots) + "", "RUN:   " + cameraRunString, "BACK"};
        int numOfItems = sizeof(camera) / sizeof(String);

        // Handle button pressure.
        if (actionRequest) {
            actionRequest = false;
            modifier = 0;
            switch (pointingRow) {
                case 0: {
                    editingParameter = true;
                    break;
                }
                case 1: {
                    editingParameter = true;
                    break;
                }
                case 2: {
                    editingParameter = true;
                    break;
                }
                case 3: {
                    menu = M_SETTINGS;
                    pointingRow = 0;
                    break;
                }
            }
		    navigateMenu(menu);
	    } else {
            scrollItems(camera, numOfItems);
	    }
	    break;
	}
	
	case M_TELESCOPE: {

        break;
	}
	
    default: {
      break;
    }
  }
}

void pointerMoveDown() {
  if (editingParameter) {
    modifier = 1;
  } else {
    pointingRow += 1;
  }
  navigateMenu(menu);
}

void pointerMoveUp() {
  if (editingParameter) {
    modifier = -1;
  } else {
    pointingRow -= 1;
  }
  navigateMenu(menu);
}

// TODO: Optimize...
void runTimer() {
  while(1) {
    lcd.setCursor(13,1);
    lcd.print(F(">  "));
    delay(200);
    lcd.setCursor(13,1);
    lcd.print(F(">> "));
    delay(200);
    lcd.setCursor(13,1);
    lcd.print(F(">>>"));
    delay(200);
    lcd.setCursor(13,1);
    lcd.print(F(" >>"));
    delay(200);
    lcd.setCursor(13,1);
    lcd.print(F("  >"));
    delay(200);
  }
}

void backlightON() {
    digitalWrite(PIN_BACKLIGHT, HIGH);
}

void backlightOFF() {
    digitalWrite(PIN_BACKLIGHT, LOW);
}

/*========================================*/
/*                 BUZZER                 */
/*========================================*/

void beep() {
    tone(5, 1500, 50);
}

/*========================================*/
/*         ENCODER FUNCTIONALITIES        */
/*========================================*/

#define PIN_ENCODER_CLOCK 2
#define PIN_ENCODER_DATA  3
#define PIN_ENCODER_BTN   4

int actualState;
int previousState;
bool buttonPressed = false;
int idleTime_ms = 0;

void encoderRead() {
    // Check if button is pressed.
    if (digitalRead(PIN_ENCODER_BTN) == LOW) {
        beep();
        idleTime_ms = 0;
        buttonPressed = true;
        if (editingParameter) {
            // Exit editing.
            editingParameter = false;
            } else {
            // Action requested.
            actionRequest = true;
        }
	    navigateMenu(menu);
        // Software debouncing.
	    delay(250);
	    buttonPressed = false;
    } else {
        // Reads the "actual" state of the clock pin.
        actualState = digitalRead(PIN_ENCODER_CLOCK);
        // If the previous and the actual state of the clock are different, that means a step has occured.
        if (actualState != previousState) {
            idleTime_ms = 0;
            // If the data state is different to the clock state, that means the encoder is rotating clockwise.
            if (digitalRead(PIN_ENCODER_DATA) != actualState) {
                pointerMoveDown();
            } else {
                pointerMoveUp();
            }
        }
    }
    // Updates the previous state of the clock with the current state.
    previousState = actualState;
    // Increase idle time.
    idleTime_ms += 1;
    delay(1);
}


/*========================================*/
/*            BATERY MANAGEMENT           */
/*========================================*/



/*========================================*/
/*            CAMERA TRIGGERING           */
/*========================================*/

#define PIN_CAMERA  4
#define PIN_LED     6

void takePhoto(int numPhotos) {
  for(int j = 0; j < numPhotos; ++j) {
    digitalWrite(PIN_CAMERA, HIGH);
    delay(500);
    digitalWrite(PIN_CAMERA, LOW);
    for(int i = 0; i < userSettings.delayTime; ++i) {
      enterSleep(WDTO_1S);
    }
  }
}

/*========================================*/
/*              MAIN ROUTINES             */
/*========================================*/

#define PIN_BTN   5
#define PIN_BCKL  9

void setup() {
  wdt_disable();
  //pinMode(BZZ, OUTPUT);
  //pinMode(PIN_MOTOR, OUTPUT);
  //pinMode(PIN_CAMERA, OUTPUT);
  //pinMode(PIN_BCKL, OUTPUT);
  
  // Encoder initialization.
  pinMode(PIN_ENCODER_CLOCK, INPUT);
  pinMode(PIN_ENCODER_DATA, INPUT);
  pinMode(PIN_ENCODER_BTN, INPUT);
  previousState = digitalRead(PIN_ENCODER_CLOCK);
  // LCD initialization.
  loadCustomSymbols();
  lcd.begin(16, 2);
  navigateMenu(M_SETTINGS);
  // Load saved settings.
  loadEEPROM();
  //setupWDT();
  //http://www.leonardomiliani.com/2013/impariamo-ad-usare-il-watchdog-1/
}

void loop() {
  encoderRead();
  if (idleTime_ms > 5000) {
      backlightOFF();
      idleTime_ms = 5000;
  } else {
      backlightON();
  }
  // pointerSelection();
  // if(digitalRead(PIN_BTN) == HIGH) {
    // delay(250);
    // while(digitalRead(PIN_BTN) == LOW) {
      // delay(250);
      // pointerMoveDown();
      // lcd.setCursor(pointer + 1, 1);
      // getPOT(menu + 91);
      // lcd.print(F(" "));
      // mainMenu();
    // }
    // lcd.setCursor(pointer + 1, 1);
    // //Non-optimized EEPROM data saving.
    // writeEEPROM(ADDR_SPD, userSettings.motorSpeed);
    // writeEEPROM(ADDR_TMR, userSettings.delayTime);
    // writeEEPROM(ADDR_HTR, userSettings.dewMode);
    // pointerMoveUp();
  // }
  // delay(250);
}

// Remeber AREF!!!