/////////////////////////////////////////////////
// Tennessee Tech Autonomous Robotics Club     //
// Third Day: Boe Bot Navigation               //
//            19 September, 2017               //
// Prepared by Chris Stringer, 19 Sept, 2017   //
/////////////////////////////////////////////////

#include "Particle.h"
#include "application.h"

#ifndef _ULTRASONIC_SENSOR_H_
#define _ULTRASONIC_SENSOR_H_

    class UltrasonicSensor
    {
    public:
        static const int INVALID_PIN = -1;

        enum SensorStyle
        {
            FourPin,
            ThreePin,
            Analog
        };

        UltrasonicSensor(SensorStyle style, const int pinRx, const int pinTx = INVALID_PIN, String name = "");
        virtual ~UltrasonicSensor();

        float GetDistance_cm();

        void SetAnalogScale_cmpV(const float scale)
        {
            analogScale_cmpV_ = scale;
        }

        void SetAnalogDivs(const unsigned int divs)
        {
            analogDivs_ = divs;
        }

        void SetAnalogRef_V(const float voltage)
        {
            analogReference_V_ = voltage;
        }

        void Disable()
        {
            enabled_ = false;
            if (INVALID_PIN != triggerPin_)
            {
                digitalWrite(triggerPin_, LOW);
                pinMode(triggerPin_, INPUT);
            }
        }

        void Enable(bool startReading = false)
        {
            enabled_ = true;
            if (INVALID_PIN != triggerPin_)
            {
                pinMode(triggerPin_, OUTPUT);
                digitalWrite(triggerPin_, startReading);
            }
        }

        String Name()
        {
            return name_;
        }

    private:
        int triggerPin_;
        int receivePin_;
        bool enabled_;
        SensorStyle sensorStyle_;

        String name_;

        float analogScale_cmpV_;
        float analogReference_V_;
        unsigned int analogDivs_;

        void TriggerSensor();

        unsigned int GetReading_us();
        float GetReading_V();
    };

#endif // _ULTRASONIC_SENSOR_H_
