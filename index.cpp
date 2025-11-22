// Power Theft Detection with Diode-Rectified CTs
// Using two CTs connected to A0 and A1
// Averaging over 1 minute, then comparing

// Pins
const int CT1_PIN = A0; // Utility side CT (rectified)
const int CT2_PIN = A1; // Load side CT (rectified)
const int LED_PIN = 8;
const int BUZZER_PIN = 6;
const int RELAY_PIN = 7;

// Time window for averaging
const unsigned long AVERAGE_INTERVAL = 60000UL; // 1 minute

// Threshold (in ADC counts difference ~0-1023)
// Tune this based on calibration
const int DIFF_THRESHOLD = 50;

// Variables
unsigned long lastCalcTime = 0;
unsigned long sampleCount = 0;
unsigned long sumUtility = 0;
unsigned long sumLoad = 0;

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);

    Serial.begin(9600);
    Serial.println(F("Power Theft Detection Started (Diode Mode)"));
}

void loop()
{
    // Take one measurement from each CT
    int valUtility = analogRead(CT1_PIN);
    int valLoad = analogRead(CT2_PIN);

    // Accumulate sums
    sumUtility += valUtility;
    sumLoad += valLoad;
    sampleCount++;

    // Check if 1 minute passed
    if (millis() - lastCalcTime >= AVERAGE_INTERVAL)
    {
        if (sampleCount > 0)
        {
            // Calculate averages
            unsigned int avgUtility = sumUtility / sampleCount;
            unsigned int avgLoad = sumLoad / sampleCount;
            int diff = abs((int)avgUtility - (int)avgLoad);

            Serial.print(F("Avg Utility: "));
            Serial.print(avgUtility);
            Serial.print(F("  Avg Load: "));
            Serial.print(avgLoad);
            Serial.print(F("  Diff: "));
            Serial.println(diff);

            // Reset accumulators
            sumUtility = 0;
            sumLoad = 0;
            sampleCount = 0;
            lastCalcTime = millis();

            // Theft detection
            if (diff > DIFF_THRESHOLD)
            {
                digitalWrite(LED_PIN, HIGH);
                digitalWrite(BUZZER_PIN, HIGH);
                digitalWrite(RELAY_PIN, HIGH);
                Serial.println(F("⚠ Theft Detected!"));
            }
            else
            {
                digitalWrite(LED_PIN, LOW);
                digitalWrite(BUZZER_PIN, LOW);
                digitalWrite(RELAY_PIN, LOW);
                Serial.println(F("✅ Normal Operation"));
            }
        }
    }
}