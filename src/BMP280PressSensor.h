#ifndef _BMP_280_PRESS_SENSOR_H_
#define _BMP_280_PRESS_SENSOR_H_

#include "Sensor.h"
#include "Adafruit_BMP280.h"

class BMP280PressSensor
	: public Sensor
{
public:
	BMP280PressSensor(String name, Adafruit_BMP280 *sensor):
		Sensor(name, String("hPa"))
	{
		sensor_ = sensor;
	}

	float GetSample()
	{
		return sensor_->readPressure()/100.00F;
	}

private:
	Adafruit_BMP280 *sensor_;
};

#endif // _BMP_280_PRESS_SENSOR_H_
