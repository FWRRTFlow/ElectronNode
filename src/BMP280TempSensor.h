#ifndef _BMP_280_Temp_SENSOR_H_
#define _BMP_280_Temp_SENSOR_H_

#include "Sensor.h"
#include "Adafruit_BMP280.h"

class BMP280TempSensor
	: public Sensor
{
public:
	BMP280TempSensor(String name, Adafruit_BMP280 *sensor):
		Sensor(name, String("C"))
	{
		sensor_ = sensor;
	}

	float GetSample()
	{
		return sensor_->readTemperature();
	}

private:
	Adafruit_BMP280 *sensor_;
};

#endif // _BMP_280_Temp_SENSOR_H_
