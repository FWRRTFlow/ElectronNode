#ifndef _ANALOG_ULTRASONIC_SENSOR_H_
#define _ANALOG_ULTRASONIC_SENSOR_H_

#include "Sensor.h"
#include "UltrasonicSensor.h"

class AnalogUltrasonicSensor
	: public Sensor
{
public:
	AnalogUltrasonicSensor(String name, const int inputPin, const int enablePin = UltrasonicSensor::INVALID_PIN):
		Sensor(name, String("cm"))
	{
		sensor_ = new UltrasonicSensor(UltrasonicSensor::Analog, inputPin, enablePin);
	}

	~AnalogUltrasonicSensor()
	{
		delete sensor_;
	}

	void Configure(const float vRef_V, const float scale_cm_p_V, const int divs)
	{
		sensor_->SetAnalogRef_V(vRef_V);
		sensor_->SetAnalogScale_cmpV(scale_cm_p_V);
		sensor_->SetAnalogDivs(4095);
	}

	float GetSample()
	{
		return sensor_->GetDistance_cm();
	}

	void Enable()
	{
		sensor->Enable(true); // For the maxbotix sensor "true" here tells it to start taking samples
	}

private:
	UltrasonicSensor *sensor_;
};

#endif // _ANALOG_ULTRASONIC_SENSOR_H_
