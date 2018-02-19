/****************************************************************************

File Name:      photodetector_cube_demo.ino
Author:         Curtis Lee
Description:    Testing a single photodetector cube for Chessbox.
                Designed with visible light LED and photodetector, but should
                work for infrared LEDs too.
                Manually calibrated for now.
****************************************************************************/
/* testing with red LEDs? */
#define COLOR_RED true
/* print everything, for debugging */
#define VERBOSE false

/* number of buttons and calibration values */
#define NUM 2

/* pins */
#define APIN A7
#define BUTTON_BASELINE 5
#define BUTTON_CALIBRATE 6
#define LED_UNSURE 3
#define LED_CLEAR 4
#define LED_BLOCKED 2
#define LED_SQUARE 2

/* calibration and read values */
#define SAMPLE_TIMES 10
#define SAMPLE_RATE 50
int calibration[NUM];
int cutoff;
long lastReadTime = 0;
/* red bleeds IR, need to offset */
int redOffset;

/* handle buttons (probably overkill method btw) */
#define DEBOUNCE_TIME 150
#define FUNC_BASELINE 0
#define FUNC_CALIBRATE 1
byte button[NUM] = {BUTTON_BASELINE, BUTTON_CALIBRATE};
long currentTime[NUM] = {0};
long lastTime[NUM] = {0};
bool currentState[NUM] = {LOW};
bool prevState[NUM] = {LOW};
bool blocked = false;
bool light = LOW;
long lastLightTime = 0;

/* function declares */
int readAvgValue();
int readAvgValue(int * value);

/* you know what this is */
void setup() {
  Serial.begin(115200);
  pinMode(LED_UNSURE, OUTPUT);
  pinMode(LED_CLEAR, OUTPUT);
  pinMode(LED_BLOCKED, OUTPUT);
  pinMode(LED_SQUARE, OUTPUT);
  for(int i = 0; i < NUM; i++){
    pinMode(button[i], INPUT_PULLUP);
  }

  if(COLOR_RED) redOffset = getRedOffset();
  /* calibrate baseline at startup, assuming it starts clear */
  calibration[FUNC_BASELINE] = readAvgValue();
  printVariables();
}

/* you know what this is */
void loop() {
  /* get the current time each loop */
  long now = millis();

  /* debounce and handle every button, again probably overkill */
  for(int i = 0; i < NUM; i++){
      currentState[i] = digitalRead(button[i]);
      if(now - lastTime[i] > DEBOUNCE_TIME)
      {
        if(currentState[i] == LOW && prevState[i] == HIGH)
        {
          /* calibrate specified value */
          if(COLOR_RED && light){
            calibration[i] = readAvgValue() - redOffset;
          }
          else{
            readAvgValue(&calibration[i]);
          } 
          readAvgValue(&calibration[i]);
          /*  and recalculate cutoff point */
          cutoff = (calibration[FUNC_BASELINE]+calibration[FUNC_CALIBRATE]) / 2;
          /*  and might as well print those values */
          printVariables();
          lastTime[i] = now;
        }
        prevState[i] = currentState[i];
      }
  }

  /* only sample at the defined sample rate */
  if(now - lastReadTime > SAMPLE_RATE){
    /* read photodiode value */
    int sensorValue = analogRead(APIN);
    if(VERBOSE) Serial.println(sensorValue);
    
    /* print LED accordingly */
    if(COLOR_RED && light){
      blocked = (sensorValue - redOffset) < cutoff;
    }
    else{
      blocked = sensorValue < cutoff;
    }
    //digitalWrite(LED_BLOCKED, blocked);
    if(!blocked) light = LOW;
    digitalWrite(LED_CLEAR, !blocked);
    
    lastReadTime = now;
  }
  
  if(blocked && (now - lastLightTime > 500)){
    light = !light; 
    lastLightTime = now;
  }
  digitalWrite(LED_SQUARE, light);
}

/* prints out calibration info */
void printVariables(){
  Serial.print(F("BASELINE: "));
  Serial.println(calibration[FUNC_BASELINE]);
  Serial.print(F("PLACED  : "));
  Serial.println(calibration[FUNC_CALIBRATE]);
  Serial.print(F("CUTOFF  : "));
  Serial.println(cutoff);
  Serial.print(F("OFFSET  : "));
  Serial.println(redOffset);
}
/* calculates the offset to compensate if using red LED that emits IR */
int getRedOffset(){
  digitalWrite(LED_SQUARE, HIGH);
  delay(100);
  int red = readAvgValue();
  digitalWrite(LED_SQUARE, LOW);
  delay(100);
  int black = readAvgValue();
  int offset =  red - black;
  if(VERBOSE){
    Serial.print(F("Red Offset: "));
    Serial.print(red);
    Serial.print(F(" + "));
    Serial.print(black);
    Serial.print(F(" = "));
    Serial.println(offset);
  }
  return red - black;
}
/* readAvgValue but stores value at passed pointer */
int readAvgValue(int * value){
  *value = readAvgValue();
}
/* returns an average of the input when called */
int readAvgValue(){
  int value = 0;
  Serial.println(F("Calibrating..."));

  /* sample the amount of times specified */
  for(int i = 1; i <= SAMPLE_TIMES; i++){
    /* this algorithm constantly basically does a continous average */
    value = value + (analogRead(APIN) - value)/i;
    if(VERBOSE){
      Serial.print(F("Sample #"));
      Serial.print(i);
      Serial.print(": ");
      Serial.println(value);
    }
    /* read slowly, for no reason */
    delay(10);
  }
  if(VERBOSE){
    Serial.print(F("Average Sensor Value: "));
    Serial.println(value);
  }
  return value;
}

