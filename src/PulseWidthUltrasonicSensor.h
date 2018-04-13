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
			sensor = new UltrasonicSensor(UltrasonicSensor::ThreePin, echoPin);
		}
		else
		{
			sensor = new UltrasonicSensor(UltrasonicSensor::FourPin, echoPin, trigPin);
		}
	}

	~PulseWidthUltrasonicSensor()
	{
		delete sensor;
	}

	float GetSample()
	{
		return sensor->GetDistance_cm();
	}

	void Enable()
	{
		sensor->Enable();
	}

	void Disable()
	{
		sensor->Disable();
	}

private:
	UltrasonicSensor *sensor;
};

#endif // _PULSE_WIDTH_ULTRASONIC_SENSOR_H_
