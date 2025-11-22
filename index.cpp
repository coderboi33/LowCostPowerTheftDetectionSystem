// Power Theft Detection (RMS-like, block averaging)
// Utility CT -> A1, Load CT -> A4 (adjust pins if needed)
// Measures 4 blocks of 5 seconds (5000 samples / block @ ~1 kHz sampling),
// averages the 4 block results and compares utility vs load.
// If difference > THRESHOLD_A (amps) then alarm.

const int CT_UTILITY = A1; // Utility-side CT analog pin
const int CT_LOAD = A4;    // Load-side CT analog pin

const int LED_PIN = 8;
const int BUZZER_PIN = 9;
const int RELAY_PIN = 10;

// ADC & CT parameters
const float VREF = 5.0; // Arduino reference voltage (V)
const int ADC_MAX = 1023;
const float ADC_LSB = VREF / ADC_MAX; // volts per ADC step

// Example CT and burden scaling (calibrate for your CT)
// CT_RATIO = primaryCurrent : secondaryCurrent ratio scaling factor
// e.g., for a CT with ratio 100A:50mA => factor to convert secondary-amps to primary-amps
const float CT_RATIO = 2000.0;  // example scaling factor (adjust to your CT)
const float RB_UTILITY = 100.0; // burden resistor ohms for utility CT (A1)
const float RB_LOAD = 100.0;    // burden resistor ohms for load CT (A4)

// Threshold in amps to trigger theft alarm (tune experimentally)
const float THRESHOLD_A = 0.2; // 0.2 A difference

// Sampling parameters
const unsigned int SAMPLES_PER_BLOCK = 5000;   // samples per 5-sec block (approx)
const unsigned long SAMPLE_INTERVAL_US = 1000; // ~1 kHz => 1000 us between samples
const int BLOCKS = 4;                          // number of 5-sec blocks to average (total 20 sec)

void setup()
{
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);

    Serial.println(F("=== Power Theft Detection (Block Averaging) ==="));
    Serial.print(F("Samples per block: "));
    Serial.println(SAMPLES_PER_BLOCK);
    Serial.print(F("Blocks averaged: "));
    Serial.println(BLOCKS);
    Serial.println();
}

// Take 'numSamples' analog readings from 'pin' and return average primary current (amps)
float takeAverageCurrent(int pin, unsigned int numSamples, float burdenOhms)
{
    // Sum as double/long to avoid overflow
    unsigned long long sumCurrentCounts = 0ull;

    for (unsigned int i = 0; i < numSamples; ++i)
    {
        int raw = analogRead(pin); // 0..1023
        // Convert ADC reading to voltage across burden: V = raw * ADC_LSB
        float voltage = raw * ADC_LSB; // volts across burden resistor (assuming input is centered/bias handled)
        // secondary current (A) = voltage / burdenOhms
        float iSec = voltage / burdenOhms;
        // primary current = secondary_current * CT_RATIO
        float iPrimary = iSec * CT_RATIO;
        // Convert to scaled integer-like value for sum (we'll sum floats via multiplication)
        // To avoid accumulation of floats, accumulate scaled integer representation:
        // but simplest: accumulate as float using a separate double variable
        sumCurrentCounts += (unsigned long long)(iPrimary * 1000000.0); // micro-amps scaled
        delayMicroseconds(SAMPLE_INTERVAL_US);
    }

    // convert back to float amps
    float avgMicroAmpScaled = (float)sumCurrentCounts / (float)numSamples;
    float avgAmp = avgMicroAmpScaled / 1000000.0; // back to amps
    return avgAmp;
}

// Take N blocks of 'samplesPerBlock' and return the average of block averages (amps)
float takeAverageOfBlocks(int pin, int blocks, unsigned int samplesPerBlock, float burdenOhms)
{
    float total = 0.0;
    for (int i = 0; i < blocks; ++i)
    {
        Serial.print(F("Block "));
        Serial.print(i + 1);
        Serial.print(F(" - sampling... "));
        float blockAvg = takeAverageCurrent(pin, samplesPerBlock, burdenOhms);
        Serial.print(F("Avg [A]: "));
        Serial.println(blockAvg, 6);
        total += blockAvg;
    }
    return total / (float)blocks;
}

void loop()
{
    Serial.println(F("\n=== Measuring 4 x 5-sec blocks (5000 samples each) ==="));

    // Average of 4 blocks (20 sec total) for utility and load CTs
    float avgUtility = takeAverageOfBlocks(CT_UTILITY, BLOCKS, SAMPLES_PER_BLOCK, RB_UTILITY);
    float avgLoad = takeAverageOfBlocks(CT_LOAD, BLOCKS, SAMPLES_PER_BLOCK, RB_LOAD);

    Serial.print(F("Final Utility Average [A]: "));
    Serial.println(avgUtility, 6);
    Serial.print(F("Final Load Average    [A]: "));
    Serial.println(avgLoad, 6);

    float diff = fabs(avgUtility - avgLoad);
    Serial.print(F("Difference [A]        : "));
    Serial.println(diff, 6);

    if (diff > THRESHOLD_A)
    {
        Serial.println(F(">>> POWER THEFT SUSPECTED <<<"));
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(RELAY_PIN, HIGH);
    }
    else
    {
        Serial.println(F("No Theft Detected"));
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(RELAY_PIN, LOW);
    }

    Serial.println(F("Measurement cycle complete. Waiting 5 sec before next cycle..."));
    delay(5000);
}
