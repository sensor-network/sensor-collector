#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <HttpClient.h>
#define SensorPin A0          // the pH meter Analog output is connected with the Arduino’s Analog

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

void setup(void)
{
  sensors.begin();    // Start up the library
  Serial.begin(9600);
  pinMode(13,OUTPUT);  
  Serial.begin(9600);  
  //Serial.println("Ph");    //Test the serial monitor
}


float b;
int buf[10],temp;
int increment = 0;

float gather()
{
    unsigned long int avgValue;  //Store the average value of the sensor feedback
  for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  { 
    buf[i]=analogRead(SensorPin);
    delay(100);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgValue+=buf[i];
  return avgValue; 
}



float get_ph(){
    float val = gather();
    float phValue=(float)val*5.0/1024/6; //convert the analog into millivolt
    phValue=3.5*phValue;  //convert the millivolt into pH value
    return phValue;
}

void loop(void)
{ 
 sensors.requestTemperatures();

 if (millis() >= 100*increment){
      float phValue = get_ph();
      DynamicJsonDocument doc(1024);
      doc["data"]["Timestamp"]= ""; 
      doc["data"]["UTC_offset"]="";
      doc["data"]["longitude"]="";
      doc["data"]["latitude"]="";
      doc["sensors"]["Temperature"] = sensors.getTempCByIndex(0);
      doc["sensors"]["Temperature_unit"]="C";
      doc["sensors"]["ph"] = round(phValue*100.0)/100.0; 
      doc["sensors"]["water_conductivity"]="";
      doc["sensors"]["water_conductivity_unit"]="ppm";
      serializeJson(doc, Serial);
      Serial.println(" ");
      digitalWrite(13, HIGH);       
      //delay(800);            //output interval with 60 seconds
      digitalWrite(13, LOW);
  }  
 if (Serial.read()== 'r')
  {
    
    float phValue = get_ph();
    Serial.print(sensors.getTempCByIndex(0));
    Serial.print(" , ");
    Serial.print(phValue,2);
    Serial.println(" ");
    digitalWrite(13, HIGH);       
    //delay(800);            //output interval with 60 seconds
    digitalWrite(13, LOW);
    increment++;
  // Send the command to get temperatures
  delay(500);
  }
}
