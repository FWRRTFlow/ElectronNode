#ifndef _RETAINED_BUFFER_SIMPLER_H_
#define _RETAINED_BUFFER_SIMPLER_H_

#include "Particle.h"
#include "CommonDataTypes.h"

// Enable system SRAM
STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

namespace Buffering
{
    // Slightly-compressed data form for storing in the SRAM
    // MUST NOT contain initializers, or it will overwrite data instead of declaring it
    struct RetainedData
    {
    	uint8_t Id; // Each sensor name is compressed to an identifier. Must register sensor name first.
    	uint16_t Value; // Values are stored to 1 decimal place. Values must be ~-3200.0 to ~3200.0
    	time_t Timestamp; // Timestamps are not compressed
    };

    const int MAX_BUFFER_BYTES = 3000; // Full SRAM is 3068 bytes
    const int ENTRY_SIZE = sizeof(RetainedData); //1 byte id, 2 byte data, 4 byte timestamp
                                                 //8 bytes total (7+1 byte word alignment)
                                                 //Allocation must happen in groups (likely 4 bytes)
    const int MAX_ENTRIES = MAX_BUFFER_BYTES / ENTRY_SIZE;

    // SRAM declarations.  RetainedBufferEntries will be set on first power up and retained thereafter
    // RetainedBuffer must not be initialized or cleared in setup code; only manipulate it by writing
    // and reading values you expect to be there.
    retained int RetainedBufferEntries = 0;
    retained RetainedData RetainedBuffer[MAX_ENTRIES];

    // Holds registered sensor names. The index of each name is its Id
    std::vector<String> SensorNames;

    // Returns Id for the given sensor name. 0xff indicated that the name was not found
    uint8_t NameToId(String Name)
    {
        for (int i=0; i < SensorNames.size(); ++i)
        {
            if (SensorNames.at(i) == Name)
                return i;
        }
        return 0xff; // Failure case
    }

    // Returns the name associated with an Id
    String IdToName(uint8_t id)
    {
        return SensorNames.at(id);
    }

    // Converts sensor data to a more-compressed form for the retained buffer
    RetainedData SensorToRetainedData(SensorData sensorData)
    {
        RetainedData dataEntry;

        dataEntry.Id = NameToId(sensorData.Name);
        dataEntry.Value = sensorData.Value*10; // Data will be truncated to 1 decimal
        dataEntry.Timestamp = sensorData.Timestamp;

        return dataEntry;
    }

    // Converts compressed, stored data to its original format
    SensorData RetainedToSensorData(RetainedData retainedData)
    {
        SensorData sensorData;

        sensorData.Name = IdToName(retainedData.Id);
        sensorData.Value = retainedData.Value/10.0f; // Remove scaling required in the compression
        sensorData.Timestamp = retainedData.Timestamp;

        return sensorData;
    }

    // Takes a positive or negative index and returns the actual index requested.
    // Positive numbers are taken as an index from the beginning, and negative
    // index values are taken from the end of the list.  Values outside of the
    // number of entries available are returned as -1 (not a valid absolute index)
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

    // Store and element into the retained buffer.
    // Returns false for unsuccessful operation (like the buffer is full)
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

    // Request an entry from the buffer.  It is returned in sensorData by references
    // Function returns false if the operation was unsuccessful (such as invalid index)
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

    // Removes an entry from the retained buffer.
    // Returns false if unsuccessful (such as invalid index)
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
    // so it can have an Id assigned to it. Do not register the same name twice.
    // This could be called automatically if a name is unregistered in NameToId, but calling
    // it separately enforces more effectively that the ordering must be the same after a reset.
    void RegisterSensorName(String name)
    {
        SensorNames.push_back(name);
    }
};

#endif // _RETAINED_BUFFER_SIMPLER_H_
