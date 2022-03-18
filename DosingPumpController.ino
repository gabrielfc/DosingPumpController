/*
||
|| @file DoserPumpController
|| @version 3.1
|| @author Gabriel F. Campolina
|| @contact gabrielfc@gmail.com
||
|| @description
|| | This library provides a simple doser pump controller. 
|| | To use all the implementation, I recommend use the following hardware:
|| | - Arduino UNO R3
|| | - Keypad 4x3
|| | - IC2 RTC 3231
|| | - IC2 LCD 16x2
|| | - Relay module with 4 outputs
|| #
||
|| @license
|| | This library is free software; you can redistribute it and/or
|| | modify it under the terms of the GNU Lesser General Public
|| | License as published by the Free Software Foundation; version
|| | 2.1 of the License.
|| |
|| | This library is distributed in the hope that it will be useful,
|| | but WITHOUT ANY WARRANTY; without even the implied warranty of
|| | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|| | Lesser General Public License for more details.
|| |
|| | You should have received a copy of the GNU Lesser General Public
|| | License along with this library; if not, write to the Free Software
|| | Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
|| #
*/
#include <Wire.h>
#include <EEPROM.h>
#include <Keypad.h> //https://github.com/Chris--A/Keypad
#include <LiquidCrystal_I2C.h> //https://github.com/marcoschwartz/LiquidCrystal_I2C

//Constants
#define DIGITAL_PIN_RELAY_1 3
#define DIGITAL_PIN_RELAY_2 4
#define DIGITAL_PIN_RELAY_3 5
#define DIGITAL_PIN_RELAY_4 6

#define DS3231_I2C_ADDRESS 104

#define  MENU_FUNCTION  6
#define  MENU_ITEM_MAIN_MENU -1
#define  MENU_ITEM_PUMP_1  0
#define  MENU_ITEM_PUMP_2  1
#define  MENU_ITEM_PUMP_3  2
#define  MENU_ITEM_PUMP_4  3
#define  MENU_ITEM_TIME  4
#define  MENU_ITEM_CALIBRATION  5
#define  MENU_VALUE  4

//LCD Setup
LiquidCrystal_I2C lcd(0x27,16,2);

byte iconTerm[8] =
{
    B00100,
    B01010,
    B01010,
    B01110,
    B01110,
    B11111,
    B11111,
    B01110
};

byte iconDrop[8] =
{
    B00100,
    B00100,
    B01010,
    B01010,
    B10001,
    B10001,
    B10001,
    B01110,
};

//Keypad setup
const byte column = 3;
const byte line = 4;
char Teclas[line][column] =
    {{'1','2','3'},
     {'4','5','6'},
     {'7','8','9'},
     {'*','0','#'}};
byte line_numbers[line] = {13,12,11,10};
byte column_numbers[column] = {9,8,7};
Keypad keypad = Keypad(makeKeymap(Teclas), line_numbers, column_numbers, line, column );

//RTC setup
byte seconds, minutes, hours, day, date, month, year;
char weekDay[4];
byte tMSB, tLSB;
float my_temp;
char my_array[100];
boolean turnOnPump1 = false;
boolean turnOnPump2 = false;
boolean turnOnPump3 = false;
boolean turnOnPump4 = false;

//Menu setup
int countMenu = MENU_ITEM_MAIN_MENU;
int countKey = 0;

String menuItem[] = 
{
    "8888888888888888"
    "P1     # -> Save",
    "P2     # -> Save",
    "P3     # -> Save",
    "P4     # -> Save",
    "Time   # -> Save",
    "Calib. * -> Exit"
};

char valueMatrix[MENU_FUNCTION][MENU_VALUE] = 
{
  {'_','_','_','_'},
  {'_','_','_','_'},
  {'_','_','_','_'},
  {'_','_','_','_'},
  {'_','_','_','_'},
  {'_','_','_','_'}
};

//EEPROM setup
int startingAddress = 30;

void setup()
{
  lcd.init();
  lcd.createChar(1,iconTerm);
  lcd.createChar(2,iconDrop);
  Wire.begin();
  startRelayModule();
  readMatrixInEEPROM();
}

void loop()
{
  char keyPress = keypad.getKey();

  if(keyPress != NO_KEY)
  {
    showMenu(keyPress);
  }

  

  if(countMenu == MENU_ITEM_MAIN_MENU)
  {
    showMainMenu();
    checkRelayStatus();
  }
}

void verifyRelay(int calibrationValue, int pumpMatrixIndex, int relayIndex)
{
      int pumpValue = getSetupValue(pumpMatrixIndex);
      if(pumpValue > 0)
      {
        float timeInSeconds = ((float)pumpValue*60)/(float)calibrationValue;
        if(timeInSeconds > 0)
        {
          turnOnRelay(relayIndex);
          delay(timeInSeconds * 1000);
          turnOffRelay(relayIndex);
        }
      }
}

void doseNow(int pumpMatrixIndex )
{
  int calibrationValue = getSetupValue(MENU_ITEM_CALIBRATION);
  if(calibrationValue <= 0){
    calibrationValue = 1;
  }
  
  int relayIndex = DIGITAL_PIN_RELAY_1;
  if(pumpMatrixIndex == MENU_ITEM_PUMP_2){
    relayIndex = DIGITAL_PIN_RELAY_2;
  }
  if(pumpMatrixIndex == MENU_ITEM_PUMP_3){
    relayIndex = DIGITAL_PIN_RELAY_3;
  }
   if(pumpMatrixIndex == MENU_ITEM_PUMP_4){
    relayIndex = DIGITAL_PIN_RELAY_4;
  }

  
      int pumpValue = getSetupValue(pumpMatrixIndex);
      if(pumpValue > 0)
      {
        float timeInSeconds = ((float)pumpValue*60)/(float)calibrationValue;
        if(timeInSeconds > 0)
        {
          turnOnRelay(relayIndex);
          delay(timeInSeconds * 1000);
          turnOffRelay(relayIndex);
        }
      }
}


void checkRelayStatus()
{
  int calibrationValue = getSetupValue(MENU_ITEM_CALIBRATION);
  if(calibrationValue > 0)
  {
    //Pump 1
    if(hours == 6 && minutes == 0 && seconds == 0 )
    {
      verifyRelay(calibrationValue, MENU_ITEM_PUMP_1, DIGITAL_PIN_RELAY_1);    
    }
    //Pump 2
    if(hours == 12 && minutes == 0 && seconds == 0 )
    {
      verifyRelay(calibrationValue, MENU_ITEM_PUMP_2, DIGITAL_PIN_RELAY_2);    
    }
    //Pump 3
    if(hours == 18 && minutes == 0 && seconds == 0 )
    {
      verifyRelay(calibrationValue, MENU_ITEM_PUMP_3, DIGITAL_PIN_RELAY_3);    
    }
    //Pump 4
    if(hours == 23 && minutes == 0 && seconds == 0 )
    {
      verifyRelay(calibrationValue, MENU_ITEM_PUMP_4, DIGITAL_PIN_RELAY_4);    
    }
  }
}

void showMainMenu()
{
  updateRTCVariables();   
  
  lcd.backlight();
  
  lcd.setCursor(0,0);
  sprintf(my_array, " %d:%d:%d", hours, minutes, seconds);
  lcd.print(my_array);
  lcd.print(' ');
  drawTerm();
  lcd.print(get3231Temp());
  
  lcd.setCursor(0,1);  
  lcd.print("Menu -> Type #");
}


int getSetupValue(int indexValue)
{
  String setupValue = String(valueMatrix[indexValue][0]) 
    + String(valueMatrix[indexValue][1]) 
    + String(valueMatrix[indexValue][2]) 
    + String(valueMatrix[indexValue][3]);        
  setupValue.replace('_', '0');
  return setupValue.toInt();
}

void showMenu(char keyPress)
{
   handlerKeyPress(keyPress);
   printMenuItem(countMenu);
}

void handlerKeyPress(int keyPress)
{
  if(countMenu == MENU_ITEM_MAIN_MENU)
  {
      readMatrixInEEPROM();
  }
  if(keyPress == '#')
  {
      writeMatrixInEEPROM();

       if(turnOnPump1){
         turnOnPump1 = false;
      }
      if(turnOnPump2){
         turnOnPump2 = false;
      }
      if(turnOnPump3){
         turnOnPump3 = false;
      }
      if(turnOnPump4){
         turnOnPump4 = false;
      }
      
      
      if(countMenu == (MENU_FUNCTION-1))
      {
        countMenu = 0;
      }else
      {
        countMenu++;
      }
      countKey = 0;
    }else if(keyPress == '*')
    {
      //When time change, set this value in RTC
      if(countMenu == MENU_ITEM_TIME)
      {        
          updateTimeInRTC();
      }      
      else if(countMenu == MENU_ITEM_PUMP_1)
      {
         if(!turnOnPump1){
            turnOnPump1 = true;   
            doseNow(MENU_ITEM_PUMP_1);   
         }
      }
      else if(countMenu == MENU_ITEM_PUMP_2)
      {
         if(!turnOnPump2){
            turnOnPump2 = true;        
           doseNow(MENU_ITEM_PUMP_2);
         }
      }
      else if(countMenu == MENU_ITEM_PUMP_3)
      {
         if(!turnOnPump3){
            turnOnPump3 = true;        
            doseNow(MENU_ITEM_PUMP_3);
         }
      }
      else if(countMenu == MENU_ITEM_PUMP_4)
      {
         if(!turnOnPump4){
            turnOnPump4 = true;        
            doseNow(MENU_ITEM_PUMP_4);
         }
      }else{
        //Save and Come back to The main menu
         writeMatrixInEEPROM();
        countMenu = MENU_ITEM_MAIN_MENU;
        countKey = 0;
      }
    }else
    {
      valueMatrix[countMenu][countKey]  = keyPress;

      if(countKey == 3)
      {
        countKey = 0;
      }else
      {
        countKey++;
      }
    }
}

void printMenuItem(int countMenu)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(menuItem[countMenu]);
  
  lcd.setCursor(0,1);
  if(countMenu == MENU_ITEM_TIME)
  {
    lcd.print(String(valueMatrix[countMenu][0]) + String(valueMatrix[countMenu][1]) + String(":") + String(valueMatrix[countMenu][2])+String(valueMatrix[countMenu][3]));  
  }else if(countMenu == MENU_ITEM_CALIBRATION)
  {
    lcd.print(String(valueMatrix[countMenu][0]) + String(valueMatrix[countMenu][1]) + String(valueMatrix[countMenu][2])+String(valueMatrix[countMenu][3]) + String(" ml/minute"));
  }else
  {
    lcd.print(String(valueMatrix[countMenu][0]) + String(valueMatrix[countMenu][1]) + String(valueMatrix[countMenu][2])+String(valueMatrix[countMenu][3]) + String("ml * -> Dose"));
  }
}

void updateTimeInRTC()
{
  String hourTemp = String(valueMatrix[MENU_ITEM_TIME][0]) + String(valueMatrix[MENU_ITEM_TIME][1]);
  String minuteTemp = String(valueMatrix[MENU_ITEM_TIME][2]) + String(valueMatrix[MENU_ITEM_TIME][3]);
  
  hourTemp.replace('_', '0');
  minuteTemp.replace('_', '0');
   
  set3231Date((byte)hourTemp.toInt(), (byte)minuteTemp.toInt());
  
  //Clear time
  valueMatrix[MENU_ITEM_TIME][0] = '_';
  valueMatrix[MENU_ITEM_TIME][1] = '_';
  valueMatrix[MENU_ITEM_TIME][2] = '_';
  valueMatrix[MENU_ITEM_TIME][3] = '_';        
}

int convertStringToInt(String value)
{
  return value.toInt();
}

void drawTerm()
{
  lcd.write(1);
}
void drawDrop()
{
  lcd.write(1);
}

void drawDegree()
{
 lcd.print((char)223);
}

void startRelayModule()
{
   pinMode(DIGITAL_PIN_RELAY_1, OUTPUT);
   pinMode(DIGITAL_PIN_RELAY_2, OUTPUT);
   pinMode(DIGITAL_PIN_RELAY_3, OUTPUT);
   pinMode(DIGITAL_PIN_RELAY_4, OUTPUT);

   turnOffRelay(DIGITAL_PIN_RELAY_1);
   turnOffRelay(DIGITAL_PIN_RELAY_2);
   turnOffRelay(DIGITAL_PIN_RELAY_3);
   turnOffRelay(DIGITAL_PIN_RELAY_4);
}

void turnOnRelay(int relayNu)
{
  digitalWrite(relayNu, LOW);
}

void turnOffRelay(int relayNumber)
{
  digitalWrite(relayNumber, HIGH);
}

//EEPROM functions
void  readMatrixInEEPROM()
{
  int count = startingAddress;
  for(int i = 0; i < MENU_FUNCTION; i++)
  {
    for(int j = 0; j < MENU_VALUE; j++)
    {
      valueMatrix[i][j] = EEPROM.read(count++);
    }
  }
}

void  writeMatrixInEEPROM(){
  int count = startingAddress;
  for(int i = 0; i < MENU_FUNCTION; i++)
  {
    for(int j = 0; j < MENU_VALUE; j++)
    {
      EEPROM.write(count++, valueMatrix[i][j]);
    }
  }
}
//RTC functions
void updateRTCVariables()
{
  // send request to receive data starting at register 0
  Wire.beginTransmission(DS3231_I2C_ADDRESS); // 104 is DS3231 device address
  Wire.write(0x00); // start at register 0
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7); // request seven bytes

  if(Wire.available()) 
  {
    seconds = Wire.read(); // get seconds
    minutes = Wire.read(); // get minutes
    hours   = Wire.read();   // get hours
    day     = Wire.read();
    date    = Wire.read();
    month   = Wire.read(); //temp month
    year    = Wire.read();
       
    seconds = (((seconds & B11110000)>>4)*10 + (seconds & B00001111)); // convert BCD to decimal
    minutes = (((minutes & B11110000)>>4)*10 + (minutes & B00001111)); // convert BCD to decimal
    hours   = (((hours & B00110000)>>4)*10 + (hours & B00001111)); // convert BCD to decimal (assume 24 hour mode)
    day     = (day & B00000111); // 1-7
    date    = (((date & B00110000)>>4)*10 + (date & B00001111)); // 1-31
    month   = (((month & B00010000)>>4)*10 + (month & B00001111)); //msb7 is century overflow
    year    = (((year & B11110000)>>4)*10 + (year & B00001111));
  }
 
  switch (day) {
    case 1:
      strcpy(weekDay, "Sun");
      break;
    case 2:
      strcpy(weekDay, "Mon");
      break;
    case 3:
      strcpy(weekDay, "Tue");
      break;
    case 4:
      strcpy(weekDay, "Wed");
      break;
    case 5:
      strcpy(weekDay, "Thu");
      break;
    case 6:
      strcpy(weekDay, "Fri");
      break;
    case 7:
      strcpy(weekDay, "Sat");
      break;
  }
}

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

void set3231Date(byte _hours, byte _minutes)
{
//T(sec)(min)(hour)(dayOfWeek)(dayOfMonth)(month)(year)
//T(00-59)(00-59)(00-23)(1-7)(01-31)(01-12)(00-99)
//Example: 02-Feb-09 @ 19:57:11 for the 3rd day of the week -> T1157193020209
// T1124154091014
  seconds = 0; // Use of (byte) type casting and ascii math to achieve result.  
  minutes = _minutes ;
  hours   = _hours;
  day     = 1;
  date    = 1;
  month   = 1;
  year    = 00;
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.write(decToBcd(seconds));
  Wire.write(decToBcd(minutes));
  Wire.write(decToBcd(hours));
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(date));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

float get3231Temp()
{
  float temp3231;
  
  //temp registers (11h-12h) get updated automatically every 64s
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x11);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);
 
  if(Wire.available()) 
  {
    tMSB = Wire.read(); //2's complement int portion
    tLSB = Wire.read(); //fraction portion   
    temp3231 = (tMSB & B01111111); //do 2's math on Tmsb
    temp3231 += ( (tLSB >> 6) * 0.25 ); //only care about bits 7 & 8
  }  
   
  return temp3231;
}
