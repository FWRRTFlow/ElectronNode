
#include <vector>

#include <Adafruit_BMP280.h>
#include <LiquidCrystal_I2C_Spark.h>

#include "Sensor.h"
#include "AnalogUltrasonicSensor.h"
#include "PulseWidthUltrasonicSensor.h"
#include "BMP280TempSensor.h"
#include "BMP280PressSensor.h"
#include "BatterySensor.h"

struct SensorConfig
{
	Sensor* SensorPtr;
	int NumSamples;
	int SamplePeriod_ms;
};

struct SensorData
{
	String Name;
	float Value;
	time_t Timestamp;
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

SYSTEM_MODE(AUTOMATIC);
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
const float MAX_ON_TIME_min = 2.0; // unused currently


const OutputLevel NODE_OUTPUT_LEVEL = OutputLevel::NoLCD;
//const OutputLevel NODE_OUTPUT_LEVEL = OutputLevel::SerialOnly;

void ProcessAllSensors();
time_t GetMidTimeStamp(time_t startTime, time_t stopTime);
bool PublishData(const SensorData sensorData, OutputLevel outputLevel);
bool PublishBuffer(OutputLevel outputLevel);
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
	// after storing it into a SensorConfig.  If you can still access the underlying object, like without
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

	// Treat the battery level as a sensor so it will be automatically published like the rest
	BatterySensor *battery = new BatterySensor("battery", &fuelGauge);
	workingConfig.SensorPtr = battery;
	workingConfig.NumSamples = 1;
	workingConfig.SamplePeriod_ms = 0;
	sensors.push_back(workingConfig);
}

// Main program execution.  Note that, if this completes, it will shut down and restart at setup(). It will not loop.
void loop()
{
	unsigned long startTime = millis();
	EnableSensors();
	delay(1000);

	ProcessAllSensors();
	PublishBuffer(NODE_OUTPUT_LEVEL);

	Serial.println(fuelGauge.getSoC());

	DisableSensors();
	delay(1000);

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

		SensorData newData = {s->GetName(),sensorOutput,midSampleTime};
		sensorBuffer.push_back(newData);
	}
}

time_t GetMidTimeStamp(time_t startTime, time_t stopTime)
{
	return startTime + (stopTime-startTime)/2;
}

// Publish all data in the buffer if possible
bool PublishBuffer(OutputLevel outputLevel)
{
	bool success = true;
	// Prioritize newer data in the buffer by sending it in reverse order
	for (int i = sensorBuffer.size() - 1; i >= 0 ; --i)
	{
		success &= PublishData(sensorBuffer.at(i), outputLevel);
		if (success) // If this element sent successfully, remove it from the buffer
			sensorBuffer.erase(sensorBuffer.begin()+i);
	}
	return success;
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
	String stamp = Time.format(timestamp);//,TIME_FORMAT_ISO8601_FULL);
	String message = String::format("{\"val\":%f, \"time:\""+stamp+"}", value);
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
