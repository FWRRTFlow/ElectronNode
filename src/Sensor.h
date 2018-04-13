#ifndef _SENSOR_H_
#define _SENSOR_H_

#include "StatsArray.h"

class Sensor
{
public:
	Sensor(String name, String units)
	{
		name_ = new char [name.length()+1]; // Make space for the name
		name.toCharArray(name_, name.length()); // Copy name to memory
		name_[name.length()] = 0; // Add in Null Terminator

		units_ = new char [units.length()+1]; // Make space for the name
		units.toCharArray(units_, units.length()); // Copy name to memory
		units_[units.length()] = 0; // Add in Null Terminator
	};

	~Sensor()
	{
		delete[] name_; // Free up name memory
	}

	// Overload with custom sensor sampling code
	virtual float GetSample()
	{
		return (enabled_) ? 0 : -1;
	}

	virtual float GetTrialAverage(const int numSamples, const int samplePeriod_ms, const bool trimOutliers = true)
	{
		if (enabled_)
		{
			StatsTools::StatsArray<float> samples(numSamples);

			for (int i = 0; i < numSamples; ++i)
			{
				samples.Append(GetSample());
				delay(samplePeriod_ms);
			}
			if (trimOutliers)
				samples.TrimOutliers();
			return samples.Average();
		}
		else
		{
			return -1;
		}
	}

	virtual void Enable()
	{
		enabled_ = true;
	}

	virtual void Disable()
	{
		enabled_ = false;
	}

	String GetName()
	{
		return String(name_);
	}

	String GetUnits()
	{
		return String(units_);
	}

private:
	bool enabled_;
	char* name_;
	char* units_;
};

#endif // _SENSOR_H_
