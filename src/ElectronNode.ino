
#include <vector>

#include <Adafruit_BMP280.h>
#include <LiquidCrystal_I2C_Spark.h>

#include "Sensor.h"
#include "AnalogUltrasonicSensor.h"
#include "PulseWidthUltrasonicSensor.h"
#include "BMP280TempSensor.h"
#include "BMP280PressSensor.h"
#include "BatterySensor.h"
#include "CommonDataTypes.h"
#include "RetainedBufferSimpler.h"

//SYSTEM_MODE(AUTOMATIC);
SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);
STARTUP(cellular_credentials_set("RESELLER", "", "", NULL)); // Needed for AT&T Network

std::vector<SensorConfig> sensors;

std::vector<SensorData> sensorBuffer; // put into retained memory?

FuelGauge fuelGauge;
Adafruit_BMP280 envSensorInternal;
LiquidCrystal_I2C lcd(0x27, 20, 4);


// Configuration
const int PERIPHERAL_ENABLE_PIN = B0;
const int MAXBOTIX_INPUT_PIN = A0;

const float RUN_INTERVAL_min = 15.0;
const float MAX_ON_TIME_min = .35; // The node will attempt to limit run time to this value


//const OutputLevel NODE_OUTPUT_LEVEL = OutputLevel::NoLCD;
const OutputLevel NODE_OUTPUT_LEVEL = OutputLevel::SerialOnly;

void ProcessAllSensors();
time_t GetMidTimeStamp(time_t startTime, time_t stopTime);
bool PublishData(const SensorData sensorData, OutputLevel outputLevel);
bool PublishBuffer(OutputLevel outputLevel);
bool StoreUnsentBuffer();
bool PublishRetainedBuffer(OutputLevel outputLevel);
bool PackageAndPublish(const SensorData sensorData);
bool PackageAndPublish(String topic, float value, unsigned long timestamp);
void EnableSensors();
void DisableSensors();


void setup()
{
	Serial.begin(115200);
	Particle.keepAlive(30); // Needed for AT&T Network

	pinMode(PERIPHERAL_ENABLE_PIN, OUTPUT);

	if (LCD_Only == NODE_OUTPUT_LEVEL || LocalOnly == NODE_OUTPUT_LEVEL || AllOutputs == NODE_OUTPUT_LEVEL)
	{
		lcd.init();
		lcd.backlight();
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.print("Sensor Started");
		delay(1000);
		lcd.noBacklight();
	}

	// Single config needed to save space.  It is copied into the vector with push_back and can be rewritten safely.
	SensorConfig workingConfig;

	//   Note: Perform ALL setup for sensors here.  If the variable type is lost, you CANNOT recover it.
	// This means that, after setup and storing the value in "sensors," you can only call functions
	// available in the Sensors.h file.  For example, you cannot call Configure() in the maxbotix sensor
	// after storing it into a SensorConfig.  If you can still access the underlying object, like with
	// the BMP280 sensors and FuelGauge, this does not matter so much.

	// Maxbotix Sensor
	AnalogUltrasonicSensor *maxbotixSensor = new AnalogUltrasonicSensor(String("maxbotix"), MAXBOTIX_INPUT_PIN);
	maxbotixSensor->Configure(3.3, 120*2.54, 4095); // 12 bit, 3.3V ADC.  Sensor is ~0.1V per foot.
	workingConfig.SensorPtr = maxbotixSensor;
	workingConfig.NumSamples = 10;
	workingConfig.SamplePeriod_ms = 100;
	sensors.push_back(workingConfig);

	// General BMP280 Setup
	envSensorInternal.begin(0x76);

	// Internal Temperature
	BMP280TempSensor *internalTemp = new BMP280TempSensor("tempInternal", &envSensorInternal);
	workingConfig.SensorPtr = internalTemp;
	workingConfig.NumSamples = 10;
	workingConfig.SamplePeriod_ms = 100;
	sensors.push_back(workingConfig);

	// Internal Temperature
	BMP280PressSensor *internalPress = new BMP280PressSensor("pressureInternal", &envSensorInternal);
	workingConfig.SensorPtr = internalPress;
	workingConfig.NumSamples = 10;
	workingConfig.SamplePeriod_ms = 100;
	sensors.push_back(workingConfig);

	// FuelGauge appears to not work with pointer references.  Battery will need to be done manually
	// Treat the battery level as a sensor so it will be automatically published like the rest
	BatterySensor *battery = new BatterySensor("battery", &fuelGauge);
	workingConfig.SensorPtr = battery;
	workingConfig.NumSamples = 1;
	workingConfig.SamplePeriod_ms = 0;
	sensors.push_back(workingConfig);

	// Register sensors names with the buffer
	for (int i=0; i < sensors.size(); ++i)
	{
		Buffering::RegisterSensorName(sensors.at(i).SensorPtr->GetName());
	}
}

// Main program execution.  Note that, if this completes, it will shut down and restart at setup(). It will not loop.
void loop()
{
	unsigned long startTime = millis();
	EnableSensors();
	delay(1000);

	ProcessAllSensors();

	DisableSensors();
	delay(1000);

	// Wait for the device to connect or to time out
	while(((millis() - startTime)/60000.0 < MAX_ON_TIME_min) && !Particle.connected());

	// Attempt to publish data until the node times out
	bool allSent = false;
	while (((millis() - startTime)/60000.0 < MAX_ON_TIME_min) && Particle.connected() && false == allSent)
	//if (((millis() - startTime)/60000.0 < MAX_ON_TIME_min) && Particle.connected() && false == allSent) // Modified to try only once
	{
		// First see if the running buffer is empty, then check that the retained buffer is empty too
		allSent |= PublishBuffer(NODE_OUTPUT_LEVEL);
		allSent &= PublishRetainedBuffer(NODE_OUTPUT_LEVEL);
	}
	// Node timed out or all sent

	if (false == allSent)
		if(StoreUnsentBuffer())
			sensorBuffer.clear(); // If all data is successfully backed up, clear the RAM buffer

	Serial.print("RetainedBufferEntries:");Serial.println(Buffering::RetainedBufferEntries);
	Serial.print("Size of Each Entry");Serial.println(Buffering::ENTRY_SIZE);
	Serial.println();

	//    The program restarts after each sleep, so subtract the time it takes
    // to run from the next sleep cycle.  Note that this cannot be used without sleep.
    //System.sleep(SLEEP_MODE_DEEP, RUN_INTERVAL_min*60-(millis()/1000));
	delay(30000-(millis() - startTime));
}

void ProcessAllSensors()
{
	for (int i = 0; i < sensors.size(); ++i)
	{
		SensorConfig sc = sensors.at(i);
		Sensor* s = sc.SensorPtr;
		time_t startTime = Time.now();
		float sensorOutput = s->GetTrialAverage(sc.NumSamples, sc.SamplePeriod_ms);
		time_t midSampleTime = GetMidTimeStamp(startTime, Time.now());

		// FIX: Figure out why it won't work normally. The value from BatterySensor
		// is always zero for pointer refs, pass by ref, and by just declaring
		// FuelGauge in the BatterySensor file
		if (s->GetName() == "battery")
			sensorOutput = fuelGauge.getSoC();

		SensorData newData = {s->GetName(),sensorOutput,midSampleTime};
		sensorBuffer.push_back(newData);
	}
}

time_t GetMidTimeStamp(time_t startTime, time_t stopTime)
{
	return startTime + (stopTime-startTime)/2;
}

// Publish all data in the buffer if possible
// Returns false if some data failed to send
bool PublishBuffer(OutputLevel outputLevel)
{
	bool allSent = true;
	// Prioritize newer data in the buffer by sending it in reverse order
	for (int i = sensorBuffer.size() - 1; i >= 0 ; --i)
	{
		bool thisSuccess = PublishData(sensorBuffer.at(i), outputLevel);
		// If this element sent successfully, remove it from the buffer.  This would normally
		// invalidate i, but because it is iterating backwards, it will make no difference
		if (thisSuccess)
			sensorBuffer.erase(sensorBuffer.begin()+i);
		allSent &= thisSuccess;
	}
	return allSent;
}

// Store any unsuccessfully sent data into retained memory
// Returns false if some entries were not stored (like out of room in buffer)
bool StoreUnsentBuffer()
{
	bool allStored = true;
	for (int i=0; i < sensorBuffer.size() ; ++i)
	{
		allStored &= Buffering::AddEntry(sensorBuffer.at(i));
	}
	return allStored;
}

// Attempt to publish all entries from the retained buffer. Any sent entries are
// removed from the buffer.
// Returns true if all entries in the buffer are sent
bool PublishRetainedBuffer(OutputLevel outputLevel)
{
	bool allSent = true;
	// Eventually reverse this?  For now, it is just easier to do it this way
	// As of now, it sends the oldest data first, which is probably good
	for (int i=0; i < Buffering::RetainedBufferEntries; ++i)
	{
		if (0 == Buffering::RetainedBufferEntries)
			break; // Exit early if the buffer is empty

		SensorData sensorData;
		if(Buffering::GetEntry(i, sensorData))
		{
			bool thisSuccess = PublishData(sensorData, outputLevel);
			// If it is sent, remove it.  However, the next iteration needs to start at the same index
			// since all remaining entries are shifted left. (i--)++ turns into the same i
			if (thisSuccess)
				Buffering::RemoveEntry(i--);
			allSent &= thisSuccess;
		}
	}
	return allSent;
}

bool PublishData(const SensorData sensorData, OutputLevel outputLevel)
{
	bool success = true;
	if (SerialOnly == outputLevel || LocalOnly == outputLevel || AllOutputs == outputLevel || NoLCD == outputLevel)
	{
		Serial.print(sensorData.Name + ": ");
		Serial.print(sensorData.Value);
		Serial.println("  "+Time.format(sensorData.Timestamp,TIME_FORMAT_ISO8601_FULL));
	}

	if (LCD_Only == outputLevel || LocalOnly == outputLevel || AllOutputs == outputLevel)
	{
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.print(sensorData.Name + ": ");
		lcd.print(sensorData.Value);
	}

	if (Publish == outputLevel || AllOutputs == outputLevel || NoLCD == outputLevel)
	{
		success &= PackageAndPublish(sensorData);
	}

	return success;
}

bool PackageAndPublish(const SensorData sensorData)
{
	return PackageAndPublish(sensorData.Name, sensorData.Value, sensorData.Timestamp);
}

bool PackageAndPublish(String topic, float value, time_t timestamp)
{
	String stamp = Time.format(timestamp,TIME_FORMAT_ISO8601_FULL);
	String message = String::format("{\"val\":%.2f, \"time:\""+stamp+"}", value);
	return Particle.publish(topic, message, PRIVATE);
}

void EnableSensors()
{
	digitalWrite(PERIPHERAL_ENABLE_PIN, HIGH);
	delay(1000);
	for (int i = 0; i < sensors.size(); ++i)
	{
		sensors.at(i).SensorPtr->Enable();
	}
}

void DisableSensors()
{
	for (int i = 0; i < sensors.size(); ++i)
	{
		sensors.at(i).SensorPtr->Disable();
	}
	digitalWrite(PERIPHERAL_ENABLE_PIN, LOW);
}
