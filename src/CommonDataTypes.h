#ifndef _COMMON_DATA_TYPES_H_
#define _COMMON_DATA_TYPES_H_

#include "Sensor.h"

struct SensorData
{
	String Name;
	float Value;
	time_t Timestamp;
};

// Hold both a sensor reference and information required for sampling
struct SensorConfig
{
	Sensor* SensorPtr; // Can be a subclass as well
	int NumSamples; // How many samples to take for this sensor
	int SamplePeriod_ms; // How long between each sample to wait before taking another.
						 // This approximates a sampling interval if this value is long
						 // compared to the sensor actuation time.
};

// Determine in what ways to send the data from the node
enum OutputLevel
{
	NoOutput,
	LCD_Only,
	SerialOnly,
	Publish,
	LocalOnly, // Serial and LCD only
	NoLCD, // Serial and Publish
	AllOutputs
};

#endif // _COMMON_DATA_TYPES_H_
