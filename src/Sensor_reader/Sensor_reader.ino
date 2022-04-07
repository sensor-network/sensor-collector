#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoHttpClient.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <SD.h>
#define SensorPin A0          // the pH meter Analog output is connected with the Arduino’s Analog
#define TdsSensorPin A1
#define VREF 5.0 // analog reference voltage(Volt) of the ADC
#define SCOUNT 30 // sum of sample point
// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 2

File myFile;


TinyGPSPlus gps;

char ssid[] = "Tele2_96e70a_2.4Ghz";  //wifi ssid
char pass[] = "cdmkfmy3";  //wifi password
const char serverName[] = "webhook.site";  // server name
int port = 80;

/*
   This sample code demonstrates the normal use of a TinyGPSPlus (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;


// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverName, port);

int status = WL_IDLE_STATUS;
// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25, phValue = 0.0;


void setup(void)
{
  Serial.begin(9600);
  ss.begin(GPSBaud);
  sensors.begin();    // Start up the library
  pinMode(TdsSensorPin, INPUT);
  WiFi.begin(ssid, pass);
  Serial.println("Initializing SD card...");
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    while (1);
  }
}

float b;
int buf[10], temp;
int increment = 0;


float get_ph(float val) {
  float phValue = (float)val * 5.0 / 1024 / 6; //convert the analog into millivolt
  phValue = 3.5 * phValue; //convert the millivolt into pH value
  return phValue;
}

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


void SendRequest() {

  static unsigned long posting = millis();
  if (millis() - posting > 10000U) {
    posting = millis();
    float phvalueedited = round(phValue * 100.0) / 100.0;
    float temperatureedited = round((temperature) * 100.0) / 100.0;
    String contentType = "application/x-www-form-urlencoded";

    TinyGPSTime t = gps.time;
    TinyGPSDate d = gps.date;

    // Time format
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour() + 2, t.minute(), t.second());

    // Date format
    char cz[32];
    sprintf(cz, "%02d-%02d-%02d ", d.year() , d.month(), d.day());

    float latitude = gps.location.lat();
    float longitude = gps.location.lng();

    String httpRequestData = "{\"timestamp\":\"" + String(cz) +"T"+ String(sz) +"+02"+"\",\"longitude\":" + String(longitude, 6) + ",\"latitude\":" + String(latitude, 6) + ",\"sensors\":{\"temperature\":" + String(temperatureedited, 1) + ",\"temperature_unit\":\"C\",\"conductivity\":" + tdsValue + ",\"conductivity_unit\":\"Spm\",\"ph_level\":" + phvalueedited + "}}" ;
    if (WiFi.status() == WL_CONNECTED && latitude != 0 && longitude != 0 && phvalueedited !=0)
    {
      myFile = SD.open("measurments.txt");
      if (myFile)
      {
        while (myFile.available()) {
          String postData = myFile.readStringUntil('\n');
          client.post("/bb10c047-1d67-4c98-bd00-a5c3009a3561", contentType, postData);
          int statusCode = client.responseStatusCode();
          String response = client.responseBody();
          Serial.println(httpRequestData);
          Serial.print("Status code: ");
          Serial.println(statusCode);
          Serial.print("Response: ");
          Serial.println(response);
        }
        myFile.close();
        SD.remove("measurments.txt");
      }

      String postData = httpRequestData;
      client.post("/bb10c047-1d67-4c98-bd00-a5c3009a3561", contentType, postData);
      int statusCode = client.responseStatusCode();
      String response = client.responseBody();
      Serial.println(httpRequestData);
      Serial.print("Status code: ");
      Serial.println(statusCode);
      Serial.print("Response: ");
      Serial.println(response);
    }

    else if (WiFi.status() != WL_CONNECTED && latitude != 0 && longitude != 0 && phvalueedited !=0)
    {
      Serial.println("Wireless not connected, saving data on SD-card");

      if (myFile)
      { myFile.println(httpRequestData);
        Serial.println("The file is already open and data is writing");
      }
      else
      { myFile = SD.open("test.txt", FILE_WRITE);
        myFile.println(httpRequestData);
        Serial.println("Opening file");
      }
      myFile.close();
      Serial.println("Closing file");
    }

    else
    { Serial.println("Location is not valid");
    }
    smartDelay(0);
  }
}

void SensorBootup() {
  static unsigned long sensorResponsetimepoint = millis();
  if ( millis() - sensorResponsetimepoint < 15000U) {
    Serial.println("Starting up takes 15 seconds");
    delay(15000U);
  }
}


void TDSSensor() {

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

}

void PHSensor() {
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


    phValue = get_ph(avgValue);

  }
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}


void loop(void)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    SensorBootup();
    TDSSensor();
    PHSensor();
    SendRequest();
    smartDelay(1000);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    SensorBootup();
    TDSSensor();
    PHSensor();
    SendRequest();
    smartDelay(1000);
  }
}
