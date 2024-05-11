// Tyler Kwist
// CPE 301 Spring 2024

#include <Stepper.h>
#include <LiquidCrystal.h>
#include <DHT.h>

const int tempThreshold = 80;
const int waterThreshold = 80;
#define RESETBUTTON 18
#define POWERBUTTON 19
volatile bool reset = false;
volatile bool power = false;

// Step Motor
#define CWBUTTON 2
#define CCWBUTTON 3
const int stepsPerRev = 1000;
const int rolePerMin = 17;
const int steps = 128;
volatile bool cw = false;
volatile bool ccw = false;
Stepper myStepper = Stepper(stepsPerRev, 39, 41, 43, 45);

// LCD Display
const int RS = 33, EN = 31, D4 = 29, D5 = 27, D6 = 25, D7 = 23;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Water and DHT Sensors
#define WATER_PIN A0
#define DHTPIN A1
#define DHTTYPE DHT11
DHT tempHum(DHTPIN, DHTTYPE);
volatile int waterLevel;
volatile int temp;
volatile int hum;

void setup() {
  // Initialize LCD Screen
  lcd.begin(16, 2);

  // Start DHT
  tempHum.begin();

  // Fan Motor
  DDRE |= (1 << DDE3); // ENABLE Port
  DDRH |= (1 << DDH3); // DIRA Port
  PORTH |= (1 << PORTH3); // Set DIRA high
  
  // Step Motor
  myStepper.setSpeed(rolePerMin);
  attachInterrupt(digitalPinToInterrupt(CWBUTTON), cwISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(CCWBUTTON), ccwISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(POWERBUTTON), powerISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(RESETBUTTON), resetISR, CHANGE);

  // LEDs
  DDRH |= (1 << DDH5); // Blue
  DDRH |= (1 << DDH6); // Green
  DDRB |= (1 << DDB4); // Yellow
  DDRB |= (1 << DDB5); // Red
}

void loop() {
  // DISABLED STATE
  if(!power){
    disabledState();
  }

  if(power){
    idleState();
  }
}

void disabledState(){
  lcd.clear();
  yellowON();
}

void idleState(){
  printToLCD();
  greenON();
  fanAdjust();
  waterLevel = readWaterLevel(WATER_PIN);
  if(waterLevel < waterThreshold){
    errorState();
    waterLevel = readWaterLevel(WATER_PIN);
  }
  while(temp > tempThreshold){
    runningState();
    temp = tempHum.readTemperature(true);
  }
  PORTE &= ~(1 << PORTE3);
}

void errorState(){
  PORTE &= ~(1 << PORTE3); // Fan off
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water Too Low!");
  redON();
  while(reset == false){}
}

void runningState(){
  printToLCD();
  PORTE |= (1 << PORTE3); // Fan on
  blueON();
  if(waterLevel < waterThreshold){
    errorState();
    waterLevel = readWaterLevel(WATER_PIN);
  }
}

void cwISR(){
  if(power == true){
    static unsigned long lastDebounceTime = 0;
    unsigned long debounceDelay = 10; //
    if ((millis() - lastDebounceTime) > debounceDelay) {
      cw = true;
      lastDebounceTime = millis();
    }
  }
}

void ccwISR(){
  if(power == true){
    static unsigned long lastDebounceTime = 0;
    unsigned long debounceDelay = 10;
    if ((millis() - lastDebounceTime) > debounceDelay) {
      ccw = true;
      lastDebounceTime = millis();
    }
  }
}

void powerISR(){
  static unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 10;
  if ((millis() - lastDebounceTime) > debounceDelay) {
    switch(power){
      case true:
        power = false;
        lastDebounceTime = millis();
        break;
      case false:
        power = true;
        lastDebounceTime = millis();
        break;
    }
  }
}

void resetISR(){
  static unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 10;
  if ((millis() - lastDebounceTime) > debounceDelay) {
    switch(reset){
      case true:
        reset = false;
        lastDebounceTime = millis();
        break;
      case false:
        reset = true;
        lastDebounceTime = millis();
        break;
    }
  }
}

int readWaterLevel(int pin) {
  // Configure ADC
  ADCSRA |= (1 << ADEN); // Enable ADC
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Set ADC prescaler to 128
  ADMUX = (1 << REFS0); // Set reference voltage to AVCC
  ADMUX |= (pin & 0x07); // Select analog pin
  // Start the conversion
  ADCSRA |= (1 << ADSC);
  // Wait for the conversion to complete
  while (ADCSRA & (1 << ADSC));
  // Map the sensor's output range to desired range
  int sensorValue = ADC;
  // Map the range to 0-256
  int mappedValue = map(sensorValue, 43, 260, 0, 256);
  // Return the result
  return mappedValue;
}

void fanAdjust(){
  if(cw){
    myStepper.step(steps);
    cw = false;
  }
  if(ccw){
    myStepper.step(-steps);
    ccw = false;
  }
}

void printToLCD(){
  lcd.clear();  
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  temp = tempHum.readTemperature(true);
  lcd.print(temp);
  lcd.print(" F");
  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  hum = tempHum.readHumidity();
  lcd.print(hum);
  lcd.print(" %");
}

void blueON(){
  PORTH |= (1 << PORTH5); // Blue LED OFF
  PORTH &= ~(1 << PORTH6); // Green LED OFF
  PORTB &= ~(1 << PORTB4); // Yellow LED ON
  PORTB &= ~(1 << PORTB5); // Red LED OFF
}

void greenON(){
  PORTH &= ~(1 << PORTH5); // Blue LED OFF
  PORTH |= (1 << PORTH6); // Green LED OFF
  PORTB &= ~(1 << PORTB4); // Yellow LED ON
  PORTB &= ~(1 << PORTB5); // Red LED OFF
}

void yellowON(){
  PORTH &= ~(1 << PORTH5); // Blue LED OFF
  PORTH &= ~(1 << PORTH6); // Green LED OFF
  PORTB |= (1 << PORTB4); // Yellow LED ON
  PORTB &= ~(1 << PORTB5); // Red LED OFF
}

void redON(){
  PORTH &= ~(1 << PORTH5); // Blue LED OFF
  PORTH &= ~(1 << PORTH6); // Green LED OFF
  PORTB &= ~(1 << PORTB4); // Yellow LED ON
  PORTB |= (1 << PORTB5); // Red LED OFF
}
