#define VREF 5.0
#define ACTectionRange 20    


const uint8_t ACPin = A0;      
const long measurementTime = 10; //in ms

long previousTime = 0;

float amp = 1.0;
const float ARalpha = 0.995;


void setup() {
  Serial.begin(115200);

}

void loop() {
  if ((millis() - previousTime) > measurementTime){
    previousTime = millis();
    float Amp = readACCurrentValue();
    amp = (ARalpha * amp) + (1.0 - ARalpha) * Amp;
    Serial.print(amp);
    Serial.print(",");
    Serial.println(Amp);
  }

}

float readACCurrentValue() {
  float peakVoltage = 0;
  for (uint8_t i = 0; i < 5; i++) { 
    peakVoltage += analogRead(ACPin);   
    delay(1);
  }
  peakVoltage = peakVoltage / 5.0;
  float voltageVirtualValue = peakVoltage * 0.707;    
  voltageVirtualValue = (voltageVirtualValue / 1024.0 * VREF ) / 2.0;
  return voltageVirtualValue * ACTectionRange;
}