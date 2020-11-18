//For OLED Display
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ultrasonic.h>
#include <EEPROM.h>
/*
 * Parking Assist Stoplight
 * Created by Ty Palowski
 * https://youtube.com/c/typalowski
 * 
 * 
 *  I haven't located a github repository for Ty (pjk)
 */
 /************************** Configuration ***********************************
 *
 *    Modifications made to source code by 
 *    Peter Kelly (pjk)
 *    eeguy98@gmail.com
 *    
 *    Fusion 360 models for 3D printing created by Peter Kelly.
 *  
 *  Revision History
 *  Rev       Author        Changes
 *  1.00      pjk           Initial changes from Ty's original code found at 
 *                            https://create.arduino.cc/projecthub/typalowski/parking-assist-stoplight-2794d7
 *                            View README to find major changes in my build and Ty's initial release.
 *                            
 */

//For Rotary Encoder
int inputCLK = 4;     //Encoder Pin CLK (Clock)
int inputDT = 2;      //Encoder Pin DT (Data)
int inputSW = 8;      //Encoder Pin SW (Switch)

int counter = 1;      // Offset from startup value of light1
                      //  This is incremented/decremented with every click of teh encoder
int currentStateCLK = LOW;  
int previousStateCLK = currentStateCLK; 
int switchState = HIGH;

String encdir ="";

//For Ultrasonic Sensor
#define TRIGGER_PIN 9   // US Trigger pin.  Toggle to start measurement
#define ECHO_PIN   10   // US Echo Pin.  Measure length to convert to inches

#define RED_LED_ON    12      // Current supply for RED LED 
#define YELLOW_LED_ON 11      // Current supply for YELLOW LED
#define GREEN_LED_ON  13      // Current supply for GREEN LED

/*********************************************
 * 
 * 
 * Red light span
 * |             |<- light1 (red_const)
 *                Yellow light Span
 *               |                      |<- light2 = light1 + yellow_const
 *                                       Green light span
 *                                      |                         |<- light3 = light2 + green_const
 *                                                                 No LED on
 *                                                                >|  Anything greater than the Green gap
 *                                                                
 */
                    // The following variables change as the encoder is turned
int light1 = 18;    // Distance used to make a decision on which LED to turn on
int light2 = 24;    //  These only change when the switch button is pressed.
int light3 = 36;    //  These values are stored in EEPROM
int distance1;
                                        // The constant numbers determine the gaps
unsigned char  red_const = light1;      // How close you can get when the light turns red (in inch)
unsigned char  yellow_const = light2;   // Light turns yellow
unsigned char  green_const = light3;    // Light turns green


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)

/*******************************************************
 * 
 *  Set up the EEPROM addresses
 *  ADDR      DATA
 *  ----      ----
 *  0x00      Current 'RED' distance
 *  0x01      Gap from RED to YELLOW
 *  0x02      Gap from YELLOW  to GREEN
 *
 ********************************************************/
#define RED_DIST    0
#define YELLOW_GAP  1
#define GREEN_GAP   2
#define MAX_EEPROM  256

#define DEBUG 1       //  Turn on or off the DEBUG output
#define CLEAR_EEPROM  0   // Used to clear the EEPROM if you get it too hosed.


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//#define LOGO_HEIGHT   16
//#define LOGO_WIDTH    16
const unsigned char myBitmap [] PROGMEM = 
{ // 'stoplight and jeep, 128x32px
  0x00, 0x00, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x01, 0x80, 0x06, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x01, 0x80, 0x06, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xff, 0xfc, 0xff, 0xfc, 0x00, 0x00, 0x01, 0xe0, 0x07, 0x80, 0x1c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xff, 0xf0, 0x3f, 0xfc, 0x00, 0x07, 0xc1, 0xe0, 0x07, 0x80, 0x0e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xff, 0xf0, 0x1f, 0xfc, 0x00, 0x07, 0xc1, 0xe0, 0x07, 0x80, 0x1e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x7f, 0xf0, 0x1f, 0xf8, 0x00, 0x06, 0xc1, 0xe0, 0x07, 0x80, 0x3e, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x3f, 0xf0, 0x1f, 0xf0, 0x00, 0x07, 0xc1, 0xb0, 0x06, 0xc0, 0x3f, 0xff, 0x00, 0x00, 0x00, 
  0x00, 0x0f, 0xf0, 0x3f, 0xe0, 0x00, 0x07, 0xcf, 0xf8, 0x07, 0xc0, 0x3f, 0xff, 0xff, 0x80, 0x00, 
  0x00, 0x0f, 0xf8, 0x7f, 0xc0, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 
  0x00, 0x01, 0xff, 0xfe, 0x00, 0x00, 0x07, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xa0, 0x00, 
  0x00, 0x00, 0xff, 0xfc, 0x00, 0x00, 0x07, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 
  0x00, 0xff, 0xfe, 0xff, 0xfc, 0x00, 0x07, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 
  0x00, 0xff, 0xf0, 0x3f, 0xfc, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 
  0x00, 0xff, 0xf0, 0x1f, 0xfc, 0x00, 0x07, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 
  0x00, 0x7f, 0xf0, 0x1f, 0xf8, 0x00, 0x07, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 
  0x00, 0x3f, 0xf0, 0x1f, 0xf8, 0x00, 0x07, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 
  0x00, 0x1f, 0xf0, 0x3f, 0xe0, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 
  0x00, 0x0f, 0xf8, 0x3f, 0xc0, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 
  0x00, 0x07, 0xff, 0xff, 0xc0, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 
  0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 
  0x00, 0x01, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 
  0x00, 0xff, 0xf8, 0x7f, 0xfc, 0x00, 0x00, 0x03, 0xff, 0xf6, 0x00, 0x00, 0x6f, 0xff, 0xc0, 0x00, 
  0x00, 0xff, 0xf0, 0x3f, 0xfc, 0x00, 0x00, 0x03, 0xff, 0xf0, 0x00, 0x00, 0x07, 0xff, 0xc0, 0x00, 
  0x00, 0xff, 0xf0, 0x1f, 0xfc, 0x00, 0x00, 0x03, 0xff, 0xf0, 0x00, 0x00, 0x0f, 0xff, 0xc0, 0x00, 
  0x00, 0x7f, 0xe0, 0x1f, 0xf8, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x00, 0x07, 0xff, 0x80, 0x00, 
  0x00, 0x3f, 0xf0, 0x1f, 0xf0, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x00, 0x07, 0xff, 0x80, 0x00, 
  0x00, 0x0f, 0xf0, 0x3f, 0xc0, 0x00, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x00, 0x03, 0xff, 0x00, 0x00, 
  0x00, 0x0f, 0xfc, 0x7f, 0xc0, 0x00, 0x00, 0x00, 0x7f, 0x80, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x00, 
  0x00, 0x01, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 
  0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void setup()
{ Serial.begin (9600);              // Set up serial port to 9600
  pinMode(TRIGGER_PIN, OUTPUT);     // Ultrasonic (US) Trigger is an output to start the signal
  pinMode(ECHO_PIN, INPUT);         // US Echo is an input to measure time on
  pinMode(RED_LED_ON, OUTPUT);            // Set up the LED 'turn-on' pins to output
  pinMode(YELLOW_LED_ON, OUTPUT);
  pinMode(GREEN_LED_ON, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(inputDT), update, CHANGE);

   // Set encoder pins as inputs  
   pinMode (inputCLK, INPUT_PULLUP);    // Encoder pins are all pullups, since the
   pinMode (inputDT, INPUT_PULLUP);     //  encoder I used switches to GND  (pjk)
   pinMode (inputSW, INPUT_PULLUP);

   // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();     //  Print the pretty graphcs Ty made
  display.setRotation (2);

  display.drawBitmap(0, 0, myBitmap, 128, 64, WHITE); // display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);

#if CLEAR_EEPROM
    EEPROM.write(RED_DIST, 0xff);     // Clear all your current values 
    EEPROM.write(YELLOW_GAP, 0xff);   //  so you are clean on reboot.
    EEPROM.write(GREEN_GAP, 0xff);
#endif
  
#if DEBUG
    Serial.print("EEPROM[0] = ");
    Serial.println(EEPROM.read(RED_DIST)); 
    Serial.print("EEPROM[1] = ");
    Serial.println(EEPROM.read(YELLOW_GAP)); 
    Serial.print("EEPROM[2] = ");
    Serial.println(EEPROM.read(GREEN_GAP)); 
    
    delay(10000);
#endif
  if (EEPROM.read(RED_DIST) != 0xff)    // If it's never been written...
    light1 = (EEPROM.read(RED_DIST));
 
  if (EEPROM.read(YELLOW_GAP) != 0xff)  
    light2 = (EEPROM.read(YELLOW_GAP));

  if (EEPROM.read(GREEN_GAP) != 0xff)
    light3 = (EEPROM.read(GREEN_GAP));
    
#if DEBUG
    Serial.print("red_const = ");
    Serial.println(light1);   
    Serial.print("yellow_const = ");
    Serial.println(light2);  
    Serial.print("green_const = ");
    Serial.println(light3);
#endif

red_const = light1;   // How close you can get when the light turns red (in inch)
                      // The other lights vary based on the gap set above

}

void loop(){

//  Set up local variables used in loop()
  long duration, distance; 
  unsigned char i, dist_smooth = 10;    // Get a few samples of the US to smooth out noise 
  float dist_total;
  
// BUTTON
  switchState = digitalRead(inputSW); // Read the digital value of the switch (LOW/HIGH)
  // If the switch is pressed (LOW), print message and store new values
  if (switchState == LOW) 
  {
    Serial.println("Switch pressed");
    EEPROM.write(RED_DIST, light1);                     // Store all your current values 
    EEPROM.write(YELLOW_GAP, light1 + yellow_const);    //  so they are available on reboot.
    EEPROM.write(GREEN_GAP, light2 + green_const);
  }

  dist_total = 0;   // reset this
  for (i = 0; i < dist_smooth; i++){
    Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN);
    distance = ultrasonic.distanceRead()/2.54;  /* Read the distance, in cm.  Convert to inches */
    dist_total += distance;     // keep the running total
    delay(50);
  }
  distance = dist_total / i;    // Get the average
  
  Serial.print("Distance measured:");
  Serial.println(distance);

  light1 = counter + red_const;     // light1 ..2 and ..3 set the distance to change the active LED.
  light2 = light1 + yellow_const;
  light3 = light2 + green_const;
  distance1 = distance;

#if DEBUG
    Serial.print("red_const = ");
    Serial.println(red_const);   
    Serial.print("yellow_const = ");
    Serial.println(yellow_const);  
    Serial.print("green_const = ");
    Serial.println(green_const);
    Serial.println();
    Serial.print("counter: ");
    Serial.println(counter);
    Serial.print("light1: ");
    Serial.println(light1);
    Serial.print("light2: ");
    Serial.println(light2);
    Serial.print("light3: ");
    Serial.println(light3);
    Serial.print("distance: ");
    Serial.println(distance);
#endif


if (distance < light1)
 { //RED LIGHT
  digitalWrite(RED_LED_ON,HIGH);
  digitalWrite(YELLOW_LED_ON,LOW);
  digitalWrite(GREEN_LED_ON,LOW);
 }
else if (distance >= light1 && distance < light2)
 { //YELLOW LIGHT
  digitalWrite(YELLOW_LED_ON,HIGH);
  digitalWrite(RED_LED_ON,LOW);
  digitalWrite(GREEN_LED_ON,LOW);
 }
else if (distance >= light2 && distance < light3)
 { //GREEN LIGHT
  digitalWrite(GREEN_LED_ON,HIGH);
  digitalWrite(RED_LED_ON,LOW);
  digitalWrite(YELLOW_LED_ON,LOW);
 }
else
 {
  digitalWrite(RED_LED_ON,LOW);
  digitalWrite(YELLOW_LED_ON,LOW);
  digitalWrite(GREEN_LED_ON,LOW);
 }
  displayStopDist();
  display.display();
}

void update() {
    // Read the current state of inputCLK
   currentStateCLK = digitalRead(inputCLK);
   
   Serial.println(F("Reading encoder"));
   if ((previousStateCLK == LOW) && (currentStateCLK == HIGH)) 
   {  
      if (digitalRead(inputDT) == HIGH)
        counter --;
      else 
        counter ++;       
   } 
   previousStateCLK = currentStateCLK;    // Remember where it was
}

void displayStopDist(){

  // Define d as Stop Distance
  float d = light1;
  float c = distance1;
  // Clear the display
  display.clearDisplay();
  //Set the color - always use white despite actual display color
  display.setTextColor(WHITE);
  //Set the font size
  display.setTextSize(1);
  //Set the cursor coordinates
  display.setCursor(0,0);
  display.print("Parking Stop Assist");
  display.setCursor(0,11); 
  display.print("Stop Dist: "); 
  display.setTextSize(2);
  display.print(d,0);
  display.print("in");
  display.setTextSize(1);
  display.setCursor(0,25);
  //display.print("Temperature: "); 
  display.print(c,0);
  display.print("in");
}
