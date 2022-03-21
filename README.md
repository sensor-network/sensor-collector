# Sensor Collector

## Introduction
The purpose of this project is to build a succesful sensor-network that will collect, transform and transmit water-related data to an API. 

## Architecture Overview
![image](https://github.com/Hemofrags/pictures/blob/main/7c7e425912a04b4514be9cc7e669b9bd.png)

## How to Use

### Build

**Prerequisites**
* [Arduino IDE 1.8.19](https://www.arduino.cc/en/software) or later
* Arduino uno wifi rev2
* MASUNN Ds18B20 temperature sensor
* Beitian BN-220 dubble GPS
* KEYESTUDIO Analog TDS sensor
* Garsent Digital PH sensor

### Test

TODO: Explain how unit- or integreation tests can be executed.

### Run

1- Connect the senors to the arduino board as in the [figure](https://github.com/sensor-network/sensor-collector/blob/main/Diagrams/sensor_circuit.png)

2- Download the [code](https://github.com/sensor-network/sensor-collector/blob/main/sensor_code/Sensor_reader.zip) and extract it.

3- Open the file using Arduino IDE.

4- Navigate to Tools -> Manage libraries.

5- Copy and paste each of the libraries bellow in the search bar to download it:
   * Onewire
   * DallasTemperature
   * ArduinoJson

6- Change the Arduino's analog pins to match the connected pins for both PH and TDS sensors.
```
#define SensorPin A0          // the pH meter Analog output is connected with the Arduino’s Analog
#define TdsSensorPin A1       // the tds meter Analog output is connected with the Arduino’s Analog
```

7- Run the code

## License
Source Code: [MIT](https://github.com/sensor-network/sensor-collector/blob/main/license.txt) License
