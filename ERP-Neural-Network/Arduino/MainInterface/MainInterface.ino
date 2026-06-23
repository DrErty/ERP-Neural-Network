// Author: DrErty
// Date: June 2026
// This is the main Arduino controller, this is used in all measurements in the article

struct PulseChannel
{
  unsigned long long nextHigh;
  unsigned long long nextLow;
  bool high;
};

struct AnalogChannel
{
  bool armed;
  bool hasSpike;
  unsigned long long lastSpike;
  float freq;
};

constexpr int HID_IN_PINS[3] = {2, 3, 4};
constexpr int OUT_IN_PINS[3] = {5, 6, 7};
constexpr int HID_OUT_PINS[3] = {A0, A1, A2};
constexpr int OUT_OUT_PIN = A3;
constexpr int BASELINE_SAMPLES = 100;

float neuronBaseline[4] = {0, 0, 0, 0};

constexpr float ADC_REFERENCE = 5.0;
constexpr float SPIKE_THRESHOLD_V = 1.0;
constexpr float SPIKE_RESET_V = 0.5;

constexpr unsigned long long PULSE_WIDTH_US = 1500;
constexpr unsigned long long FREQ_TIMEOUT_US = 200000;

constexpr unsigned long long CALIB_SETTLE_TIME_MS = 1000;
constexpr unsigned long long CALIB_PERIOD_MS = 60000;
constexpr unsigned long long CALIB_TIMEOUT_MS = 5000;

constexpr float F_BASELINE = 10.0;
constexpr float F_OFFSET = 10.0;
constexpr float FAIL_FREQUENCY = 1.0;

constexpr float DECODE_SCALE = 1.0;

constexpr float HIDDEN_BIAS[3] = {1.0, 1.0, 0.0};
constexpr float OUTPUT_BIAS = 0.0;

unsigned long long lastCalibration = 0;

float encodeToFreq(float value)
{
  if (value < -1.0) value = -1.0;
  if (value >  1.0) value =  1.0;
  return F_BASELINE + F_OFFSET * value;
}

float decodeToValue(float freq, int idx)
{
  float v = (freq - neuronBaseline[idx]) / DECODE_SCALE;
  if (v < -1.0) v = -1.0;
  if (v >  1.0) v =  1.0;
  return v;
}

float signActivate(float value)
{
  return (value >= 0.0) ? 1.0 : -1.0;
}

float readVoltage(int pin)
{
  return analogRead(pin) * (ADC_REFERENCE / 1023.0);
}

constexpr int inputChannelCount = 6;
PulseChannel inputChannels[inputChannelCount];

constexpr int outputChannelCount = 4;
AnalogChannel outputChannels[outputChannelCount];

void initPulse()
{
  unsigned long long now = micros();
  for (int i = 0; i < inputChannelCount; i++) { inputChannels[i].nextHigh = now; inputChannels[i].high = false; }
}

void initAnalog()
{
  unsigned long long now = micros();
  for (int i = 0; i < outputChannelCount; i++) {
    outputChannels[i].armed = true;
    outputChannels[i].hasSpike = false;
    outputChannels[i].lastSpike = now;
    outputChannels[i].freq = 0.0;
  }
}

void setup()
{
  Serial.begin(115200);
  for (int i = 0; i < 3; i++) { pinMode(HID_IN_PINS[i], OUTPUT); digitalWrite(HID_IN_PINS[i], LOW); }
  for (int i = 0; i < 3; i++) { pinMode(OUT_IN_PINS[i], OUTPUT); digitalWrite(OUT_IN_PINS[i], LOW); }

  lastCalibration = millis();

  initPulse();
  initAnalog();
}

bool pollPulse(float freq, int pin, int idx)
{
  unsigned long long now = micros();

  if (inputChannels[idx].high)
  {
    if ((long long)(now - inputChannels[idx].nextLow) >= 0)
    {
      digitalWrite(pin, LOW);
      inputChannels[idx].high = false;
    }
    return false;
  }

  if (freq > 0.0 && (long long)(now - inputChannels[idx].nextHigh) >= 0)
  {
    digitalWrite(pin, HIGH);
    inputChannels[idx].high = true;
    inputChannels[idx].nextLow = now + PULSE_WIDTH_US;
    inputChannels[idx].nextHigh += (unsigned long long)(1000000.0 / freq);
    return true;
  }

  return false;
}

float pollFrequency(int pin, int idx)
{
  unsigned long long now = micros();
  float v = analogRead(pin) * (ADC_REFERENCE / 1023.0);

  if (outputChannels[idx].armed && v >= SPIKE_THRESHOLD_V) {
    if (outputChannels[idx].hasSpike) {
      unsigned long long isi = now - outputChannels[idx].lastSpike;
      if (isi > 0) outputChannels[idx].freq = 1000000.0 / (float)isi;
    }
    outputChannels[idx].lastSpike = now;
    outputChannels[idx].hasSpike = true;
    outputChannels[idx].armed = false;
  } else if (!outputChannels[idx].armed && v < SPIKE_RESET_V) {
    outputChannels[idx].armed = true;
  }

  if ((now - outputChannels[idx].lastSpike) > FREQ_TIMEOUT_US) {
    outputChannels[idx].freq = 0.0;
  }

  return outputChannels[idx].freq;
}

float inputBuff[3] = {0.0, 0.0, 0.0};
float hiddenBuff[3] = {0.0, 0.0, 0.0};

bool calibrating = true;

float baselineSampleTotal[4] = {0.0, 0.0, 0.0, 0.0};
unsigned long long sampleCount[4] = {0, 0, 0, 0};

void AddSample(float freq, int neuronIdx)
{
  if ((millis() - lastCalibration > CALIB_SETTLE_TIME_MS) && calibrating)
  {
    baselineSampleTotal[neuronIdx] += freq;
    sampleCount[neuronIdx] += 1;
  }
}

void loop()
{
  if ((millis() - lastCalibration > CALIB_TIMEOUT_MS) && calibrating)
  {
    calibrating = false;
    for (int neuronIdx = 0; neuronIdx < 4; ++neuronIdx)
    {
      neuronBaseline[neuronIdx] = baselineSampleTotal[neuronIdx] / sampleCount[neuronIdx];
    }
    lastCalibration = millis();
  }

  if (millis() - lastCalibration > CALIB_PERIOD_MS)
  {
    calibrating = true;
    for (int i = 0; i < 4; i++)
    {
      baselineSampleTotal[i] = 0.0;
      sampleCount[i] = 0;
    }
    lastCalibration = millis();
  }
  
  while (Serial.available() > 0)
  {
    String line = Serial.readStringUntil('\n');
    line.trim();

    int idx = 0, start = 0;
    for (int i = 0; i <= (int)line.length() && idx < 3; i++) {
      if (i == (int)line.length() || line.charAt(i) == ',') {
        inputBuff[idx++] = line.substring(start, i).toFloat();
        start = i + 1;
      }
    }
  }

  for (int i = 0; i < 3; i++)
  {
    float freq = encodeToFreq(inputBuff[i]);
    if (calibrating)
    {
      freq = encodeToFreq(0);
    }
    pollPulse(freq, HID_IN_PINS[i], i);
  }

  float outputFrequencies[4];

  for (int neuronIndex = 0; neuronIndex < 3; neuronIndex++)
  {
    float inFreq = pollFrequency(HID_OUT_PINS[neuronIndex], neuronIndex);
    AddSample(inFreq, neuronIndex);
    outputFrequencies[neuronIndex] = inFreq;

    float decodedInValue = decodeToValue(inFreq, neuronIndex);
    decodedInValue = signActivate(decodedInValue + HIDDEN_BIAS[neuronIndex]);
    
    hiddenBuff[neuronIndex] = decodedInValue;

    float outFreq = encodeToFreq(decodedInValue);
    if (calibrating)
    {
      outFreq = encodeToFreq(0);
    }
    pollPulse(outFreq, OUT_IN_PINS[neuronIndex], neuronIndex + 3);
  }

  float finalOutput = 0.0;
  {
    int neuronIndex = 3;

    float inFreq = pollFrequency(OUT_OUT_PIN, neuronIndex);
    AddSample(inFreq, neuronIndex);
    outputFrequencies[neuronIndex] = inFreq;

    float decodedInValue = decodeToValue(inFreq, neuronIndex);
    decodedInValue = signActivate(decodedInValue + OUTPUT_BIAS);
    
    finalOutput = decodedInValue;
  }

  {
    if (calibrating)
    {
      Serial.println("Calibrating");
      return;
    }
    String outString = "Neuron,";
    for (int i{0}; i < 4; ++i)
    {
      outString += String(outputFrequencies[i]);
      outString += ",";
    }
    outString += String(finalOutput);
    outString += ",";
    outString += String(0.0);
    outString += ",";
    for (int i{0}; i < 4; ++i)
    {
      outString += String(neuronBaseline[i]);
      outString += ",";
    }
    Serial.println(outString);
  }
}