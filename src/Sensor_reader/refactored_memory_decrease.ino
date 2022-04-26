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
#define TdsSensorPin A1
#define VREF 5.0 // analog reference voltage(Volt) of the ADC
#define SCOUNT 30 // sum of sample point
// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 2



/*
Try to remove as many prints for debugging as you can, these take quite alot memory. Also check if #include <SPI.h> is used otherwise remove it. 
I dont think we can make any more global variables local but if it is possible it helps alot with the memory. Having functions where something is repeated also reduces memory quite alot.
I suggest that we should make bubblesort work with both mediannum and the existing use of it
*/

TinyGPSPlus gps;

const char ssid[] PROGMEM = "One";  //wifi ssid 
const char pass[] PROGMEM = "12341234";  //wifi passwordcdmkfmy3


/*
   This sample code demonstrates the normal use of a TinyGPSPlus (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/
const char RXPin = 4, TXPin = 3;
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
void SensorBootup();
void TDSSensor();
void PHSensor();
static void smartDelay(unsigned long ms);

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



void setup(void)
{
  static const uint32_t GPSBaud = 9600;
  Serial.begin(9600);
  //lcd.print("Setting up...");
  ss.begin(GPSBaud);
  temp_sensor.begin();    // Start up the library
  pinMode(TdsSensorPin, INPUT);
  WiFi.begin(ssid, pass);
  //Serial.println("Initializing SD card...");
  if (!SD.begin(10)) {
    //Serial.println("initialization failed!");
    while (1);
  }
  SensorBootup();

}

/*
float convert_to_ph(float val) {
  float ph_val_converted = 3.5 * val * 5.0 / 1024; //convert the millivolt into pH value
  return ph_val_converted;
}
*/
int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
      /*
   should call to bubblesort
   */
  unsigned short int i, j, bTemp;
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
  
  String contentType1 = "application/json";
  String contentType2 = "Bearer default";
  const short int port = 80;
  const char serverName[] = "sensornetwork.diptsrv003.bth.se";  // server name
  HttpClient client = HttpClient(wifi, serverName, port);
  
  client.beginRequest();
  client.post("/api/v3/measurements");
  client.sendHeader("Content-Type",contentType1);
  client.sendHeader("Authorization",contentType2);
  client.beginBody();
  client.print(postData);
  client.endRequest();
         
  Serial.println(postData);
  Serial.println("Status code: "+client.responseStatusCode());
  Serial.println("Response: "+client.responseBody());
  }

void DataHandler(const Measurement& local) {
    File myFile;
    static unsigned long posting = millis();
    String httpRequestData = "{\"time\":\""+ local.gps.date_time +"\",\"position\":{\"long\":" + String(local.gps.longitude, 6) + ",\"lat\":" + String(local.gps.latitude, 6) + "},\"sensors\":[{\"id\":" + String(1) + ",\"value\":" + String(local.temperature.value, 1) + ",\"unit\":" + local.temperature.unit +"},{\"id\":" + String(2) + ",\"value\":" + local.tds.value + ",\"unit\":"+ local.tds.unit +"},{\"id\":" + String(3) + ",\"value\":" + String(local.pH, 1) + ",\"unit\":\"\"}]}" ;

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
        //Serial.println("Wireless not connected, saving data on SD-card");
        if (myFile)
        { myFile.println(httpRequestData);
        //Serial.println("The file is already open and data is writing");
        }
        else
        { myFile = SD.open("buffer.txt", FILE_WRITE);  
        myFile.println(httpRequestData);
        }
        myFile.close();
        //Serial.println("Closing file");
        WiFi.begin(ssid, pass);
   }
   /*
    else{  
      Serial.println("Location is not valid");
      //Serial.println(local.gps.longitude, 6);
      //Serial.println(local.gps.latitude, 6);
      //Serial.println(local.pH);
    
    }
    */
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
  int analogBufferIndex = 0;
  int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
  int analogBufferTemp[SCOUNT];
  while (analogBufferIndex < 20) {
    analogBuffer[analogBufferIndex++] = analogRead(TdsSensorPin);
    smartDelay(40);
  }
  for (int copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
    analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
  }
  // read the analog value more stable by the median filtering algorithm, and convert to voltage value
  //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
  float compensationVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0 / 1.0 + 0.02 * (temperatureC - 25.0); //temperature compensation
  float tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5; //convert voltage value to tds value
  return tdsValue;
}

float get_average(float list[]) {
  float average=0;
  for (unsigned short int i = 0; i < 6 ; i++) {
    average += list[i];
  }
  return average / 6;
}
/*
 bubble sort and shorten should problaby be two different functions
 */
float bubble_sort_and_shorten(float list[]) {
  int temp;
  for (unsigned short int i = 0; i < 9; i++) //sort the analog from small to large
  {
    for (unsigned short int j = i + 1; j < 10; j++)
    {
      if (list[i] > list[j])
      {
        temp = list[i];
        list[i] = list[j];
        list[j] = temp;
      }
    }
  }
  for (unsigned short int i = 2; i < 8; i++) {
    list[0] = list[i];
  }
  return get_average(list);
}

float get_ph() {
  float buf[10];
  unsigned short int phcounter = 0;
  float phValue = 0;
  while (phcounter < 10) {
    buf[phcounter++] = analogRead(SensorPin);
    delay(100);
  }
  phValue = bubble_sort_and_shorten(buf);
  return  3.5 * phValue * 5.0 / 1024;
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

bool CheckStatus() {
  
  const byte rs = 12, en = 11, d4 = 6, d5 = 8, d6 = 5, d7 = 7;
  LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
  
  DallasTemperature sensors(&oneWire);
  lcd.clear();
  if (WiFi.status() != WL_CONNECTED)
  { lcd.print("Wifi not connected");
    delay(800);
    lcd.setCursor(0, 1);
  }
  Serial.println(sensors.getDeviceCount());
  if (sensors.getDeviceCount() != 1)
  { lcd.print("Temperature sensor not connected");
    delay(800);
    lcd.setCursor(0, 1);
  }
  


}
void loop(void)
{
  //CheckStatus();
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
  DataHandler(measurement);
  smartDelay(1000);
}
