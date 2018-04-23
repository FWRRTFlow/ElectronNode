#ifndef _COMMON_DATA_TYPES_H_
#define _COMMON_DATA_TYPES_H_

struct SensorData
{
	String Name;
	float Value;
	time_t Timestamp;
};

struct SensorConfig
{
	Sensor* SensorPtr;
	int NumSamples;
	int SamplePeriod_ms;
};

enum OutputLevel
{
	NoOutput,
	LCD_Only,
	SerialOnly,
	Publish,
	LocalOnly,
	NoLCD,
	AllOutputs
};

#endif // _COMMON_DATA_TYPES_H_
