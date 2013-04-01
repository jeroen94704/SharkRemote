/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
int led1 = 6;
int led2 = 5;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  //pinMode(led1, OUTPUT);     
  //pinMode(led2, OUTPUT);     
}

// the loop routine runs over and over again forever:
void loop() {
  analogWrite(led2, 0); 
  for(int i=0; i<128; i++) {
    analogWrite(led1, 178);
  }
  delay(250);               // wait for a second
  analogWrite(led1, 0);
  analogWrite(led2, 0); 
  delay(1500);               // wait for a second
  analogWrite(led1, 0);   
  for(int i=0; i<128; i++) {
    analogWrite(led2, 178);
  }
  delay(250);               // wait for a second
  analogWrite(led1, 0);
  analogWrite(led2, 0); 
  delay(1500);               // wait for a second
}
