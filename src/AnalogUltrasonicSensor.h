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
		sensor = new UltrasonicSensor(UltrasonicSensor::Analog, inputPin, enablePin);
	}

	~AnalogUltrasonicSensor()
	{
		delete sensor;
	}

	void Configure(const float vRef_V, const float scale_cm_p_V, const int divs)
	{
		sensor->SetAnalogRef_V(vRef_V);
		sensor->SetAnalogScale_cmpV(scale_cm_p_V);
		sensor->SetAnalogDivs(4095);
	}

	float GetSample()
	{
		return sensor->GetDistance_cm();
	}

	void Enable()
	{
		sensor->Enable(true); // For the maxbotix sensor "true" here tells it to start taking samples
	}

	void Disable()
	{
		sensor->Disable();
	}

private:
	UltrasonicSensor *sensor;
};

#endif // _ANALOG_ULTRASONIC_SENSOR_H_
