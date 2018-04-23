#ifndef _RETAINED_BUFFER_SIMPLER_H_
#define _RETAINED_BUFFER_SIMPLER_H_

#include "Particle.h"
#include "CommonDataTypes.h"

// Enable system SRAM
STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

namespace Buffering
{
    struct RetainedData
    {
    	char Id;
    	char Value[6];
    	time_t Timestamp;
    };

    const int MAX_BUFFER_BYTES = 3000; // Full SRAM is 3068 bytes
    const int ENTRY_SIZE = sizeof(RetainedData); //1 byte id, 5 char string value+Null term, some size timestamp
                                                // Likely 11 bytes total for 32 bit processor
    const int MAX_ENTRIES = MAX_BUFFER_BYTES / ENTRY_SIZE;

    retained int RetainedBufferEntries = 0;
    retained RetainedData RetainedBuffer[MAX_ENTRIES];

    std::vector<String> SensorNames;

    uint8_t NameToId(String Name)
    {
        for (int i=0; i < SensorNames.size(); ++i)
        {
            if (SensorNames.at(i) == Name)
                return i;
        }
        return 0xff; // Failure case
    }

    String IdToName(uint8_t id)
    {
        return SensorNames.at(id);
    }

    RetainedData SensorToRetainedData(SensorData sensorData)
    {
        RetainedData dataEntry;

        dataEntry.Id = NameToId(sensorData.Name);
        // "%05.1f" = 0-padded, 5 chars long, 1 decimal place, float
        snprintf(dataEntry.Value,6,"%05.1f\0", sensorData.Value);
        dataEntry.Timestamp = sensorData.Timestamp;

        return dataEntry;
    }

    SensorData RetainedToSensorData(RetainedData retainedData)
    {
        SensorData sensorData;

        sensorData.Name = IdToName(retainedData.Id);
        sensorData.Value = atof(retainedData.Value);
        sensorData.Timestamp = retainedData.Timestamp;

        return sensorData;
    }

    int GetAbsoluteIndex(int index)
    {
        if (abs(index) < RetainedBufferEntries)
        {
            // Get the actual index in the array.  If it is negative, go backwards into the list
            return (index >= 0) ? index : RetainedBufferEntries + index;
        }
        return -1; // Absolute index should never be negative, so this indicates an error
    }


//////////// PUBLIC FUNCTIONS ///////////////////

    bool AddEntry(SensorData sensorData)
    {
        bool success = false;
        if (RetainedBufferEntries < MAX_ENTRIES)
        {
            RetainedBuffer[RetainedBufferEntries] = SensorToRetainedData(sensorData);

            RetainedBufferEntries++;
            success = true;
        }
        return success;
    }

    bool GetEntry(int index, SensorData &sensorData)
    {
        bool success = false;
        int absoluteIndex = GetAbsoluteIndex(index);
        if (absoluteIndex >= 0) // GetAbsoluteIndex returns -1 for invalid index
        {
            sensorData = RetainedToSensorData(RetainedBuffer[absoluteIndex]);
            success = true;
        }
        return success;
    }

    // Warning!! This operation will take longer the closer to the beginning
    // of the buffer you perform this because it shifts all remaining entries left
    bool RemoveEntry(int index)
    {
        bool success = false;

        int absoluteIndex = GetAbsoluteIndex(index);
        if (absoluteIndex >= 0)
        {
            // Shift the remaining entries left
            int bytesToCopy = sizeof(RetainedData)*(RetainedBufferEntries-absoluteIndex-1);
            memmove(RetainedBuffer+absoluteIndex, RetainedBuffer+(absoluteIndex+1), bytesToCopy);
            RetainedBufferEntries--;
            success = true;
        }
        return success;
    }

    // Sensor names are encoded as a 1 byte Id to save space. Sensor names must be registered
    // so it can have an Id assigned to it.
    void RegisterSensorName(String name)
    {
        SensorNames.push_back(name);
    }
};

#endif // _RETAINED_BUFFER_SIMPLER_H_
