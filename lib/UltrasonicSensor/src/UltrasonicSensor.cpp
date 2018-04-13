/////////////////////////////////////////////////
// Tennessee Tech Autonomous Robotics Club     //
// Third Day: Boe Bot Navigation               //
//            19 September, 2017               //
// Prepared by Chris Stringer, 19 Sept, 2017   //
/////////////////////////////////////////////////

#include "UltrasonicSensor.h"


// CONSTANTS
const float SPEED_OF_SOUND_cm_per_us = 0.0343;

//! Configure a 3-pin ultrasonic distance sensor
// in: pin = Arduino pin to trigger the sensor and to read the distance from the sensor
UltrasonicSensor::UltrasonicSensor(SensorStyle style, const int pinRx, const int pinTx, String name) :
    triggerPin_(pinTx),
    receivePin_(pinRx),
    sensorStyle_(style),
    enabled_(true),
    analogScale_cmpV_(500.0),
    analogDivs_(1024),
    analogReference_V_(3.3),
    name_(name)
{
    switch (sensorStyle_)
    {
    case SensorStyle::FourPin:
        pinMode(triggerPin_, OUTPUT);
        pinMode(receivePin_, INPUT);
        break;

    case SensorStyle::ThreePin:
        pinMode(triggerPin_, OUTPUT);
        break;

    case SensorStyle::Analog:
        Enable(true); // Enable sensor and start continuous readings
        break;
    }
}

//! Set the only possible output to high impedance mode for safety
UltrasonicSensor::~UltrasonicSensor()
{
    pinMode(triggerPin_, INPUT);
}

//! Get the raw time value from the distance sensor. This will be double the time
//     representing to the distance to the object.
// out: Duration of pulse from sensor in Microseconds.
unsigned int UltrasonicSensor::GetReading_us()
{
    TriggerSensor();
    return pulseIn(receivePin_, HIGH);
}

float UltrasonicSensor::GetReading_V()
{
    return (analogRead(receivePin_) * analogReference_V_) / analogDivs_;
}

//! Get the distance reading from the sensor
// out: Distance reported by sensor in Centimeters.
float UltrasonicSensor::GetDistance_cm()
{
    float dist_cm = 0.0;

    switch(sensorStyle_)
    {
    case SensorStyle::ThreePin:
    case SensorStyle::FourPin:
        //   The time read from the sensor is a round-trip time;
        // it needs to be cut in half to get the actual distance.
        dist_cm = (GetReading_us() * SPEED_OF_SOUND_cm_per_us) / 2.0;
        break;

    case SensorStyle::Analog:
        dist_cm = GetReading_V() * analogScale_cmpV_;
        break;
    }
    return dist_cm;
}

//! Trigger the distance reading from the sensor according to its pin layout
void UltrasonicSensor::TriggerSensor()
{
    switch(sensorStyle_)
    {
    case ThreePin:
        pinMode(triggerPin_, OUTPUT); // Prepare the pin to send the pulse
        digitalWrite(triggerPin_, HIGH); // Send the pulse
        delayMicroseconds(10);
        digitalWrite(triggerPin_, LOW);
        pinMode(triggerPin_, INPUT); // Prepare the pin to read data
        break;

    case FourPin:
        digitalWrite(triggerPin_, HIGH); // Just send the pulse
        delayMicroseconds(10);
        digitalWrite(triggerPin_, LOW);
        break;

    case Analog:
        if (INVALID_PIN != triggerPin_)
        {
            digitalWrite(triggerPin_, HIGH);
        }
        break;
    }
}
