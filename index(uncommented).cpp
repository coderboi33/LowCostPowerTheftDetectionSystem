// Power Theft Detection (block averaging)

const int CT_UTILITY = A1;
const int CT_LOAD = A4;

const int LED_PIN = 8;
const int BUZZER_PIN = 9;
const int RELAY_PIN = 10;

const float VREF = 5.0;
const int ADC_MAX = 1023;
const float ADC_LSB = VREF / ADC_MAX;

const float CT_RATIO = 2000.0;
const float RB_UTILITY = 100.0;
const float RB_LOAD = 100.0;

const float THRESHOLD_A = 0.2;

const unsigned int SAMPLES_PER_BLOCK = 5000;
const unsigned long SAMPLE_INTERVAL_US = 1000;
const int BLOCKS = 4;

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

float takeAverageCurrent(int pin, unsigned int numSamples, float burdenOhms)
{
    unsigned long long sumCurrentCounts = 0ull;

    for (unsigned int i = 0; i < numSamples; ++i)
    {
        int raw = analogRead(pin);
        float voltage = raw * ADC_LSB;
        float iSec = voltage / burdenOhms;
        float iPrimary = iSec * CT_RATIO;
        sumCurrentCounts += (unsigned long long)(iPrimary * 1000000.0);
        delayMicroseconds(SAMPLE_INTERVAL_US);
    }

    float avgMicroAmpScaled = (float)sumCurrentCounts / (float)numSamples;
    float avgAmp = avgMicroAmpScaled / 1000000.0;
    return avgAmp;
}

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
