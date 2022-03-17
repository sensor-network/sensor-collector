#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <string>
#define SensorPin A0          // the pH meter Analog output is connected with the Arduino’s Analog
#define TdsSensorPin A1       // the tds meter Analog output is connected with the Arduino’s Analog
#define VREF 5.0 // analog reference voltage(Volt) of the ADC
#define SCOUNT 30 // sum of sample point

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;
float b;
int buf[10], temp;
int increment = 0;


void setup(void)
{
  Serial.begin(9600);
  sensors.begin();    // Start up the library
  pinMode(TdsSensorPin, INPUT);

}

// The function to get the ph value
float get_ph(float val) {
  float phValue = (float)val * 5.0 / 1024 / 6; //convert the analog into millivolt
  phValue = 3.5 * phValue; //convert the millivolt into pH value
  return phValue;
}

//The function to get median number for tds
int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}


// start collecting data
void loop(void)
{
  static unsigned long sensorResponsetimepoint = millis();
  if ( millis() - sensorResponsetimepoint < 15000U) delay(15000U);
  sensors.requestTemperatures();
  static unsigned long analogSampleTimepoint = millis();

  if (millis() - analogSampleTimepoint > 40U) //every 40 milliseconds,read the analog value from the ADC
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin); //read the analog value and store into the buffer
    analogBufferIndex++;

    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }
  static unsigned long printTimepoint = millis();

  if (millis() - printTimepoint > 800U)
  {
    temperature = sensors.getTempCByIndex(0);
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0); //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge = averageVoltage / compensationCoefficient; //temperature compensation
    tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
  }
  static unsigned long phcounter = 0;
  static unsigned long phSenReadTimepoint = millis();   

  if (millis() - phSenReadTimepoint > 100U) {
    buf[phcounter] = analogRead(SensorPin);
    phcounter ++;
    phSenReadTimepoint = millis();
  }

  if (phcounter == 10) {
    phcounter = 0;
    for (int i = 0; i < 9; i++) //sort the analog from small to large
    {
      for (int j = i + 1; j < 10; j++)
      {
        if (buf[i] > buf[j])
        {
          temp = buf[i];
          buf[i] = buf[j];
          buf[j] = temp;
        }
      }
    }

    unsigned long int avgValue = 0;
    for (int i = 2; i < 8; i++)               //take the average value of 6 center sample
      avgValue += buf[i];

    float phValue = get_ph(avgValue);       // variable contains ph value from get_ph function

    //serialize data to make it json format
    DynamicJsonDocument doc(1024);
    doc["data"]["Timestamp"] = "";
    doc["data"]["UTC_offset"] = "";
    doc["data"]["longitude"] = "";
    doc["data"]["latitude"] = "";
    doc["sensors"]["Temperature"] = round((sensors.getTempCByIndex(0)) * 100.0) / 100.0;
    doc["sensors"]["Temperature_unit"] = "C";
    doc["sensors"]["ph"] = round(phValue * 100.0) / 100.0;
    doc["sensors"]["water_conductivity"] = tdsValue;
    doc["sensors"]["water_conductivity_unit"] = "ppm";
    serializeJson(doc, Serial);
    Serial.println(" ");

  }

}
