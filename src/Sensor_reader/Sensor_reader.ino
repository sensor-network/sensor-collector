#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoHttpClient.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <SD.h>
#define SensorPin A0          // the pH meter Analog output is connected with the Arduino’s Analog
#define TdsSensorPin A1 //från A1 till A3
#define VREF 5.0 // analog reference voltage(Volt) of the ADC
#define SCOUNT 30 // sum of sample point
// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 2



const byte rs = 12, en = 11, d4 = 9, d5 = 7, d6 = 5, d7 = 4;   //changed by karim d7 from 7 to 4, d5 from 8 to 7 and d4 from 6 to 9
  LiquidCrystal lcd(rs, en, d4, d5, d6, d7);



TinyGPSPlus gps;

static const int RXPin = 0, TXPin = 1;                  //edited by karim RX from 4 to 0 and TX from 3 to 1


const char ssid[] = "AndroidAP";  //wifi ssid 
const char pass[] = "kasra123";  //wifi password 

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

WiFiClient wifi;

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature temp_sensor(&oneWire);


void SendRequest();
float get_ph(float val);
int getMedianNum(int bArray[], int iFilterLen);
void TDSSensor();
void PHSensor();
static void smartDelay(unsigned long ms);
void SensorBootup();

void setup(void)
{
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.print("Setting up...");
  ss.begin(9600);
  temp_sensor.begin();    // Start up the library
  pinMode(TdsSensorPin, INPUT);
  WiFi.begin(ssid, pass);
  Serial.println("Setup");
  SensorBootup();
}

struct TDS {
  String unit;
  float value;
};

struct Temperature {
  char unit;
  float value;
};
struct GPS {
  float latitude;
  float longitude;
  String date_time;
};
struct Measurement {
  Measurement(Temperature TEMPERATURE, GPS GPS, TDS TDS, float PH) {
    this->pH = PH;
    this->temperature = TEMPERATURE;
    this->gps = GPS;
    this->tds = TDS;
  }
  float pH;
  Temperature temperature;
  GPS gps;
  TDS tds;
};

int getMedianNum(float bArray[], int iFilterLen)
{
  float bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j;
  float bTemp;
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


String get_date_and_time() {
  TinyGPSDate d = gps.date;
  TinyGPSTime t = gps.time;

  // Date format
  char date[32];
  sprintf(date, "%02d-%02d-%02d", d.year() , d.month(), d.day());

  // Time format
  char time[32];
  sprintf(time, "%02d:%02d:%02d", t.hour() + 2, t.minute(), t.second());

  String date_time = String(date) + "T" + String(time) + "+02";
  return date_time;
}



void SendRequest(String postData){
  const char serverName[] = "sensornetwork.diptsrv003.bth.se";  // server name
/*
  String contentType = "accept: application/json";
  String postData = "name=Alice&age=12";

  client.post("/", contentType, postData);

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  */
  HttpClient client = HttpClient(wifi, serverName, 80);
  
  client.beginRequest();
  client.post("/api/v3/measurements");
  client.sendHeader("Content-Length", postData.length());
  client.sendHeader("Content-Type: application/json");
  client.sendHeader("Authorization: Bearer default");
  client.beginBody();
  client.print(postData);
  client.endRequest();
        
  Serial.println(postData);
  Serial.println("Response: " + client.responseBody());
  }
  String station_id = '1'
void DataHandler(struct Measurement local) {
    File myFile;
    static unsigned long posting = millis();
    String httpRequestData = "{\"station\":\""+station_id+"\",\"time\":\""+ local.gps.date_time +"\",\"position\":{\"long\":" + String(local.gps.longitude, 6) + ",\"lat\":" + String(local.gps.latitude, 6) + "},\"sensors\":[{\"id\":" + String(1) + ",\"value\":" + String(local.temperature.value, 1) + ",\"unit\":\"C\"},{\"id\":" + String(2) + ",\"value\":" + local.tds.value + ",\"unit\":\"ppm\"},{\"id\":" + String(3) + ",\"value\":" + String(local.pH, 1) + ",\"unit\":\"\"}]}";
    Serial.println(local.pH);
    if (WiFi.status() == WL_CONNECTED && local.pH !=0)
    {
        myFile = SD.open("buffer.txt");
        if (myFile)
        {
        while (myFile.available()) {
            String postData = myFile.readStringUntil('\n');
            SendRequest(postData);
        }
        myFile.close();
        SD.remove("buffer.txt");
        }
        SendRequest(httpRequestData);
    }

    if (WiFi.status() != WL_CONNECTED && local.pH !=0)
    {
        Serial.println(F("No wifi, storing data"));

        if (myFile)
        { myFile.println(httpRequestData);
        Serial.println(F("Open and writing"));
        }
        else
        { myFile = SD.open("buffer.txt", FILE_WRITE);
        myFile.println(httpRequestData);
        }
        myFile.close();
        WiFi.begin(ssid, pass);
   }
    delay(3000);
    smartDelay(0);
}

void SensorBootup() {
  static unsigned long sensorResponsetimepoint = millis();
  if ( millis() - sensorResponsetimepoint < 15000U) {
    //Serial.println("Starting up takes 15 seconds");
    delay(15000U);
  }
}

float getTDS(float temperatureC) {
    Serial.println("TDS");
  unsigned short int analogBufferIndex = 0;
  int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
  float analogBufferTemp[SCOUNT];
  
  while (analogBufferIndex < 20) {
    analogBuffer[analogBufferIndex++] = analogRead(TdsSensorPin);
    smartDelay(40);
  }
  for (int copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
    analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
  }
  float compensationVoltage =  getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0 / (1.0 + 0.02 * (temperatureC - 25.0)); //temperature compensation
  float tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5; //convert voltage value to tds value
  return tdsValue;
}

float get_ph() {
  float buf[10];
  byte phcounter = 0;

  while (phcounter < 10) {
    buf[phcounter++] = analogRead(SensorPin);
    delay(100);
  }
  float ph_value;
  ph_value = 3.5*getMedianNum(buf, 10)* 5.0 / 1024;
  return ph_value;
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

float getTempinC() {
  temp_sensor.requestTemperatures();
  return temp_sensor.getTempCByIndex(0);
}

bool CheckStatus(Measurement values) {
    Serial.println("Check Status");
  DallasTemperature sensors(&oneWire);
  lcd.clear();
  String wifistat = "W[]";
  String tdsstat = "C[]";
  String phstat = "P[]";
  String tempstat  = "T[]";
  String gpsstat = "G[]";
  String bufferstat = "B[]";
  if (WiFi.status() != WL_CONNECTED){ 
     wifistat = "W[x]";
     WiFi.begin(ssid, pass);
  }
  if (values.tds.value > 1300){
      tdsstat = "C[x]";
    }
  if (values.pH > 10|| values.pH<3){
      phstat = "P[x]";
    }
  if (values.gps.latitude == 0.000000 || values.gps.latitude == 0.000000 || values.gps.date_time ==  "2000-00-00T02:00:00+02"){
    gpsstat = "G[x]";
    }
  //Serial.println(sensors.getDeviceCount());
  if (getTempinC() < -10 || getTempinC() > 30)
  { 
   tempstat = "T[x]";
  }
  if (SD.begin(A5) == 0) {
    bufferstat = "B[x]";
  }
  lcd.setCursor(0,0);
  lcd.print(wifistat +' '+ gpsstat+ ' '+ bufferstat );
  lcd.setCursor(0,1);
  lcd.print(phstat+' '+tempstat+ ' ' +tdsstat);
} 



void loop(void)
{  Serial.println("Loop");

  struct Temperature temperature;
  temperature.value = getTempinC();
  temperature.unit = 'C';

  float ph_struct = get_ph();

  struct TDS tds;
  tds.value = getTDS(temperature.value);
  tds.unit = "ppm";

  struct GPS gps_struct;
  gps_struct.latitude = gps.location.lat();
  gps_struct.longitude = gps.location.lng();
  gps_struct.date_time = get_date_and_time();
  Measurement measurement(temperature, gps_struct, tds, ph_struct);
  CheckStatus(measurement);
  DataHandler(measurement);
  
  smartDelay(1000);
}
