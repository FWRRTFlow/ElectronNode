#ifndef _BATTERY_SENSOR_H_
#define _BATTERY_SENSOR_H_

#include "Sensor.h"
#include "application.h"

class BatterySensor
	: public Sensor
{
public:
	BatterySensor(String name, FuelGauge *sensor):
		Sensor(name, String("%"))
	{
		sensor_ = sensor;
	}

	float GetSample()
	{
		return sensor_->getSoC();
	}

private:
	FuelGauge *sensor_;
};

#endif // _BATTERY_SENSOR_H_
