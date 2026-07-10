#include <HX711.h>
#include <Wire.h>
#include <I2C_RTC.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include <SparkFun_FS3000_Arduino_Library.h> 

// Target ESP32 I2C Address and Multiplexer Configuration
#define SLAVE_ADDR 0x55
#define MUX_ADDR 0x70
#define ESP32_MUX_CH 5 // <-- Set this to the MUX channel your ESP32 is wired to

#define LedPinRed 2
#define LedPinGreen 3

// -------------------------------------------------------------------------
// MEMORY SAVED: Custom stream class to slice data on-the-fly without String allocation
// -------------------------------------------------------------------------


void pca9548a_select(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(MUX_ADDR);
  Wire.write(1 << channel); 
  Wire.endTransmission();

  
}

class I2CChunkStream : public Print {
  private:
    uint8_t _address;
    uint8_t _buffer[30]; // 30-byte safe buffer chunk limit
    uint8_t _bufCount;

    void flushChunk() {
      if (_bufCount == 0) return;
      Wire.beginTransmission(_address);
      Wire.write(_buffer, _bufCount);
      Wire.endTransmission();
      _bufCount = 0;
      delay(10); // Increased to 10ms to give the ESP32 Slave adequate processing headroom
    }

  public:
    I2CChunkStream(uint8_t address) : _address(address), _bufCount(0) {}

    size_t write(uint8_t character) override {
      // Overwrite raw newlines into an explicit message cycle block termination sequence
      if (character == '\n') {
        flushChunk();
        Wire.beginTransmission(_address);
        Wire.write('\n'); // Send direct end signal byte
        Wire.endTransmission();
        return 1;
      }
      if (character == '\r') return 1; // Drop carriage returns

      _buffer[_bufCount++] = character;
      if (_bufCount >= 30) {
        flushChunk();
      }
      return 1;
    }
};

// Instantiate the custom I2C streaming object


I2CChunkStream esp32I2C(SLAVE_ADDR);

HX711 scale;
static DS3231 RTC; 
FS3000 fs;

const uint8_t dataPinHx711 = 7;
const uint8_t clockPinHx711 = 6;
const uint8_t chipSelect = 10;
const uint8_t ACPin = A0;      

float Amp = 1.0;
float ARalpha = 0.995;

long previousACMeasurementTime = 0;
const long ACMeasurementTime = 10;

long previousGetMinuteseasurementTime = 0;
const long getMinuteseasurementTime = 10000;
 

#define ACTectionRange 20    
#define VREF 5.0
#define sensor_1_address 0x76
#define sensor_2_address 0x77

Adafruit_BME280 bme; 

const uint8_t measure_interval = 1; 
uint8_t T_measure = 5; 

void setup() {
  Serial.begin(19200);
  delay(500);

  Serial.println("hello");
  Wire.begin();
  scale.begin(dataPinHx711, clockPinHx711);
  pca9548a_select(0);   bme.begin(0x76);
  Serial.println("hello");

  pca9548a_select(1);   bme.begin(0x76);
  pca9548a_select(2);   bme.begin(0x76);
  
  SD.begin(chipSelect);

  pca9548a_select(7); RTC.begin();

  pca9548a_select(3);   fs.begin(); fs.setRange(AIRFLOW_RANGE_15_MPS); 
  pca9548a_select(4);   fs.begin(); fs.setRange(AIRFLOW_RANGE_15_MPS);

  pinMode(LedPinRed, OUTPUT);
  pinMode(LedPinGreen, OUTPUT);

}

// Fixed framework: Temporarily grabs RTC data, then forces MUX routing back to the targeted stream receiver
void printDataStream(Print &output, uint8_t targetMuxChannel, float p1, float t1, float h1, float p2, float t2, float h2, float p3, float t3, float h3, float v1, float v2, float m, float amp) {
  // Grab timestamp from channel 7
  pca9548a_select(7);
  String date = RTC.getDateString();
  String time = RTC.getTimeString();

  // Instantly restore MUX path back to the intended device destination before printing
  pca9548a_select(targetMuxChannel);

  output.print(date);
  output.print(F(" "));
  output.print(time);
  output.print(F(","));
  output.print(p1);  output.print(F(","));
  output.print(t1);  output.print(F(","));
  output.print(h1);  output.print(F(","));
  output.print(p2);  output.print(F(","));
  output.print(t2);  output.print(F(","));
  output.print(h2);  output.print(F(","));
  output.print(p3);  output.print(F(","));
  output.print(t3);  output.print(F(","));
  output.print(h3);  output.print(F(","));
  output.print(v1);  output.print(F(","));
  output.print(v2);  output.print(F(","));
  output.print(m);   output.print(F(","));
  output.println(amp); // Triggers \n ending signal sequence natively inside I2CChunkStream
}

void loop() {

  if ((millis() - previousACMeasurementTime) > ACMeasurementTime){
    previousACMeasurementTime = millis();
    Amp = (ARalpha * Amp) + (1.0 - ARalpha) * readACCurrentValue();
  }

  if ((millis() - previousGetMinuteseasurementTime) > getMinuteseasurementTime){
    previousGetMinuteseasurementTime = millis();
    pca9548a_select(7);
    uint8_t minutes = RTC.getMinutes(); 
    Serial.println(T_measure);
  
    if (minutes >= T_measure) { 
      //float Amp = readACCurrentValue();
      // float Amp = 1.45;
      
      // Read from BME channel 0
      pca9548a_select(0);
      float Temp1 = bme.readTemperature();
      float Humi1 = bme.readHumidity();
      float Pres1 = bme.readPressure();
      
      // Read from BME channel 1
      pca9548a_select(1);
      float Temp2 = bme.readTemperature();
      float Humi2 = bme.readHumidity();
      float Pres2 = bme.readPressure();

      // Read from BME channel 2
      pca9548a_select(2);
      float Temp3 = bme.readTemperature();
      float Humi3 = bme.readHumidity();
      float Pres3 = bme.readPressure();
      
      float mass = scale.read_average(10); 
      
      // Read from FS3000 channel 3
      pca9548a_select(3);
      float v1 = fs.readMetersPerSecond();
      
      // Read from FS3000 channel 4
      pca9548a_select(4);
      float v2 = fs.readMetersPerSecond();

      // 1. Print locally to Hardware Serial Monitor (MUX destination doesn't matter for Serial)
      Serial.print(F("Sending packet: "));
      printDataStream(Serial, 7, Pres1, Temp1, Humi1, Pres2, Temp2, Humi2, Pres3, Temp3, Humi3, v1, v2, mass, Amp);
      
      // 2. Transmit to ESP32 over I2C in optimized 30-byte segments
      // Enforces strict redirection to your ESP32's actual MUX branch
      printDataStream(esp32I2C, ESP32_MUX_CH, Pres1, Temp1, Humi1, Pres2, Temp2, Humi2, Pres3, Temp3, Humi3, v1, v2, mass, Amp);
      
      // 3. Log values onto Local micro-SD Card (SPI interface, MUX channel safe)
      // switch mux to RTC, get date: for filename
      pca9548a_select(7);
      String date = RTC.getDateString();
      //fuck you with stupid string formats
      String dateFileName = date.substring(6,10) + date.substring(3,5) + date.substring(0,2) + ".txt";
      Serial.println(dateFileName);
      File Data = SD.open(dateFileName, FILE_WRITE);
      if (Data) {
        printDataStream(Data, 7, Pres1, Temp1, Humi1, Pres2, Temp2, Humi2, Pres3, Temp3, Humi3, v1, v2, mass, Amp);
        Data.close();
        digitalWrite(LedPinGreen, HIGH);
        digitalWrite(LedPinRed, LOW);
      } else {  
        Serial.println("error opening "+ dateFileName); 
        digitalWrite(LedPinRed, HIGH);
        digitalWrite(LedPinGreen, LOW);
      }
      
      T_measure += measure_interval;

      if (T_measure >= 60){
        T_measure = 0;
        while(true){
          pca9548a_select(7);
          if(RTC.getMinutes() != 59) break;
          delay(100);
        }
      }    
    }
  }
  //delay(2000);
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
