#ifndef _PULSE_WIDTH_ULTRASONIC_SENSOR_H_
#define _PULSE_WIDTH_ULTRASONIC_SENSOR_H_

#include "Sensor.h"
#include "UltrasonicSensor.h"

class PulseWidthUltrasonicSensor
	: public Sensor
{
public:
	PulseWidthUltrasonicSensor(String name, const int echoPin, const int trigPin):
		Sensor(name, String("cm"))
	{
		if (echoPin == trigPin)
		{
			sensor_ = new UltrasonicSensor(UltrasonicSensor::ThreePin, echoPin);
		}
		else
		{
			sensor_ = new UltrasonicSensor(UltrasonicSensor::FourPin, echoPin, trigPin);
		}
	}

	~PulseWidthUltrasonicSensor()
	{
		delete sensor_;
	}

	float GetSample()
	{
		return sensor_->GetDistance_cm();
	}

private:
	UltrasonicSensor *sensor_;
};

#endif // _PULSE_WIDTH_ULTRASONIC_SENSOR_H_
