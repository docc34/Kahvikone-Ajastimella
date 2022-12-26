#include "ds3231.h"
#include "Wire.h"
#include "TimerOne.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


const int output1 = 13;
const int pwrButton = A1;
const int clockVResistor = A0;
struct ts t; 
//Determines the weekday
int day = 3;
int seconds = 0;

//Determines is the alarm on or off
bool pwr = true;
//Is used to blink the alarm on section when the coffee maker is cooking
bool cooking = false;
//Current time
int displayHours = 0;
int displayMinutes = 0;

//Currently set hour for the alarm
int alarmTimeHour =0;
//Currently set minute for the alarm
int alarmTimeMinutes = 0;
//Read value from the variable resistor
int alarmTimeRead= 0;

//used for counting when a full second has passed
int secondCounter = 0;
//Leonardossa pinit 2(SCA) ja 3(SCL) ovat myös sda ja scl pinejä. Wire kirjasto määrittää nämä automaattisesti näytölle
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(9600);
  Wire.begin();
  DS3231_init(DS3231_CONTROL_INTCN);
  /*----------------------------------------------------------------------------
  In order to synchronise your clock module, insert timetable values below !
  ----------------------------------------------------------------------------*/
  t.hour=16; 
  t.min=05;
  t.sec=4;
  t.mday=26;
  t.mon=12;
  t.year=2022;
  
  pinMode(pwrButton,INPUT);
  pinMode(clockVResistor,INPUT);
  pinMode(output1,OUTPUT);
  digitalWrite(output1,HIGH);

   
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1000);

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);
  display.display();
  delay(1000);
  
  DS3231_set(t); 
}  
int alarmStageHistory = 0;
void loop() {
  DS3231_get(&t);
  Serial.print("Date : ");
  Serial.print(t.mday);
  Serial.print("/");
  Serial.print(t.mon);
  Serial.print("/");
  Serial.print(t.year);
  Serial.print("\t Hour : ");
  Serial.print(t.hour);
  Serial.print(":");
  Serial.print(t.min);
  Serial.print(".");
  Serial.println(t.sec);
  
  int button = digitalRead(pwrButton);
  
  displayHours = t.hour;
  displayMinutes = t.min;
  
  alarmTimeRead = analogRead(clockVResistor);
  
  int alarmStage = alarmTimeRead / 11.05;
  alarmTimeHour = alarmStage / 4;

  //Checks if the current input has a remainder with 4 different values. It defines the current 15 minute difference in the set alarm
  double stage1 = alarmStage * 0.25;        //0 minutes
  double stage2 = alarmStage * 0.25 + 0.25; //45 minutes
  double stage3 = alarmStage * 0.25 + 0.5;  //30 minutes
  double stage4 = alarmStage * 0.25 + 0.75; //15 minutes

  if(isInteger(stage1))
    alarmTimeMinutes = 0;
    
  if(isInteger(stage2))
    alarmTimeMinutes = 45;
    
  if(isInteger(stage3))
    alarmTimeMinutes = 30;
    
  if(isInteger(stage4))
    alarmTimeMinutes = 15;

  //Makes the button an on and off switch for turning on the alarm
  if(button == 1 & pwr == true){
    pwr = false;
  }
  else if(button == 1 & pwr == false){
    pwr = true;
  }
  

  //Settings for text
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  //Intented Oled output example below:
  //Clock:  16:17:03
  //Alarm:  15:00:00
  //Alarm on:  true
  
  //Row 1
  display.setCursor(1, 1);
    display.println("Clock:");
    
  display.setCursor(45, 1);
    display.println(displayHours);
    
  display.setCursor(55, 1);
    display.println(":");
    
  display.setCursor(60, 1);
    display.println(displayMinutes);
    
  display.setCursor(70, 1);
    display.println(":");
    
  display.setCursor(75, 1);
    display.println(t.sec);


  //Row 2
  display.setCursor(1, 10);
    display.println("Alarm:");

  display.setCursor(45, 10);
    display.println(alarmTimeHour);

  display.setCursor(55, 10);
    display.println(":");
    
  display.setCursor(60, 10);
    display.println(alarmTimeMinutes);
    
  display.setCursor(70, 10);
    display.println(":");
    
  display.setCursor(75, 10);
    display.println("00");
    
  //Row 3
  display.setCursor(1, 20);
    if(cooking == true && secondCounter == 2 || cooking == false){
      display.println("Alarm on:");
    
  display.setCursor(55, 20);
  if(pwr == false)
    display.println("False");
  else if(pwr == true)
    display.println("True");
   }
  display.display(); // Renders all the defined text on the Oled screen

  delay(250);
  secondCounter++;
  //Calls the coffee making function to update current time every second
  if(secondCounter == 4){
    makeCoffee();
    secondCounter = 0;
  }
  
}

//Check does the number have an reminder
boolean isInteger(double x)
{
    return x == (int) x;
}

void makeCoffee(){


  Serial.print("alarmTimeHour:");
  Serial.println(alarmTimeHour);
  
  if(pwr == true){

    //The device could be set to only cook on weekdays ect
    if(day <= 7){
      
      if(t.hour == alarmTimeHour & displayMinutes >= alarmTimeMinutes & alarmTimeMinutes + 10 > displayMinutes & t.sec > 0){
      //if(t.hour == 7 & t.min < 20 & t.min >= 15 & t.sec > 0){ 
      //The coffeemaker switch has to be turned on and off for it to cook.    
        seconds++;
        if(seconds == 1){
          digitalWrite(output1,LOW);
        }
        if(seconds == 6){
          digitalWrite(output1,HIGH);
        }
        if(seconds == 8){
          seconds = 0; 
        }
        Serial.println(seconds);
        cooking = true;
      }
      else{
        //Turns off the coffeemaker
        digitalWrite(output1,HIGH);
        cooking = false;
      }
    }
    
  }
  else{
    cooking = false;
    digitalWrite(output1,HIGH);
  }

  //Updates the weekday
  if(t.hour == 1 & displayMinutes == 1 & t.sec == 1){
    day++;
  }
  
  if(day >= 7){
    day = 0;
  }

}
