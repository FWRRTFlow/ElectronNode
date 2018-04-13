
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

FuelGauge fuelGauge;

std::vector<SensorConfig> sensors;

Adafruit_BMP280 envSensorInternal;

LiquidCrystal_I2C lcd(0x27, 20, 4);


// Configuration
const int PERIPHERAL_ENABLE_PIN = B0;
const int MAXBOTIX_INPUT_PIN = A0;

const float RUN_INTERVAL_min = 15.0;

const OutputLevel NODE_OUTPUT_LEVEL = OutputLevel::NoLCD;


void ProcessAllSensors(OutputLevel outputLevel);
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
	envSensorInternal.begin(0x77);

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
	EnableSensors();
	delay(1000);

	ProcessAllSensors(NODE_OUTPUT_LEVEL);

	DisableSensors();
	delay(1000);

	//    The program restarts after each sleep, so subtract the time it takes
    // to run from the next sleep cycle.  Note that is cannot be used without sleep.
    System.sleep(SLEEP_MODE_DEEP, RUN_INTERVAL_min*60-(millis()/1000));
}

void ProcessAllSensors(OutputLevel outputLevel)
{
	for (int i = 0; i < sensors.size(); ++i)
	{
		SensorConfig sc = sensors.at(i);
		Sensor* s = sc.SensorPtr;
		float sensorOutput = s->GetTrialAverage(sc.NumSamples, sc.SamplePeriod_ms);

		if (SerialOnly == outputLevel || LocalOnly == outputLevel || AllOutputs == outputLevel || NoLCD == outputLevel)
		{
			Serial.print(s->GetName() + ": ");
			Serial.print(sensorOutput);
			Serial.println(" " + s->GetUnits());
		}

		if (LCD_Only == outputLevel || LocalOnly == outputLevel || AllOutputs == outputLevel)
		{
			lcd.clear();
			lcd.setCursor(0,0);
			lcd.print(s->GetName() + ": ");
			lcd.print(sensorOutput);
			lcd.print(" "+s->GetUnits());
		}

		if (Publish == outputLevel || AllOutputs == outputLevel || NoLCD == outputLevel)
		{
			PackageAndPublish(s->GetName(), sensorOutput, 0);
		}
	}
}

bool PackageAndPublish(String topic, float value, unsigned long timestamp)
{
	String message = String(value);
	Particle.publish(topic, message, PRIVATE);
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