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

Connect the senors to the arduino board as in the [figure](https://github.com/sensor-network/sensor-collector/blob/main/Diagrams/sensor_circuit.png)

Download the [code](https://github.com/sensor-network/sensor-collector/blob/main/sensor_code/Sensor_reader.ino) and insert it to the Arduino IDE

## License
Source Code: [MIT](https://github.com/sensor-network/sensor-collector/blob/main/license.txt) License
