struct PulseChannel
{
  unsigned long nextHigh;
  unsigned long nextLow;
  bool high;
};

struct AnalogChannel
{
  bool armed;
  bool hasSpike;
  unsigned long lastSpike;
  float freq;
};

constexpr int HID_IN_PINS[3] = {2, 3, 4};
constexpr int OUT_IN_PINS[3] = {5, 6, 7};
constexpr int HID_OUT_PINS[3] = {A0, A1, A2};
constexpr int OUT_OUT_PIN = A3;

constexpr float ADC_REFERENCE = 5.0;
constexpr float SPIKE_THRESHOLD_V = 1.0;
constexpr float SPIKE_RESET_V = 0.5;

constexpr unsigned long PULSE_WIDTH_US = 5000 / 3;
constexpr unsigned long FREQ_TIMEOUT_US = 20000000;

constexpr float F_BASELINE = 10.0;
constexpr float F_OFFSET = 5.0;
constexpr float FAIL_FREQUENCY = (F_BASELINE - F_OFFSET) / 2.0;

constexpr float DECODE_CENTER = 10.0;
constexpr float DECODE_SCALE = 2.0;

constexpr float HIDDEN_WEIGHTS[3][3] = {
  {-1.0, -1.0, -1.0},
  {-1.0, 1.0, 1.0},
  {-1.0, -1.0, -1.0}
};

constexpr float OUTPUT_WEIGHTS[3] = {-1.0, 0.3862, -1.0};

constexpr float HIDDEN_BIAS[3] = {1.0, 1.0, 0.0};
constexpr float OUTPUT_BIAS = 0.0;

float encodeToFreq(float value)
{
  if (value < -1.0) value = -1.0;
  if (value >  1.0) value =  1.0;
  return F_BASELINE + F_OFFSET * value;
}

float decodeToValue(float freq)
{
  float v = (freq - DECODE_CENTER) / DECODE_SCALE;
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
  unsigned long now = micros();
  for (int i = 0; i < inputChannelCount; i++) { inputChannels[i].nextHigh = now; inputChannels[i].high = false; }
}

void initAnalog()
{
  unsigned long now = micros();
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

  initPulse();
  initAnalog();
}

bool pollPulse(float freq, int pin, int idx)
{
  unsigned long now = micros();

  if (inputChannels[idx].high)
  {
    if ((long)(now - inputChannels[idx].nextLow) >= 0)
    {
      digitalWrite(pin, LOW);
      inputChannels[idx].high = false;
    }
    return false;
  }

  if (freq > 0.0 && (long)(now - inputChannels[idx].nextHigh) >= 0)
  {
    digitalWrite(pin, HIGH);
    inputChannels[idx].high = true;
    inputChannels[idx].nextLow = now + PULSE_WIDTH_US;
    inputChannels[idx].nextHigh += (unsigned long)(1000000.0 / freq);
    return true;
  }

  return false;
}

float pollFrequency(int pin, int idx)
{
  unsigned long now = micros();
  float v = analogRead(pin) * (ADC_REFERENCE / 1023.0);

  if (outputChannels[idx].armed && v >= SPIKE_THRESHOLD_V) {
    if (outputChannels[idx].hasSpike) {
      unsigned long isi = now - outputChannels[idx].lastSpike;
      if (isi > 0) outputChannels[idx].freq = 1000000.0 / (float)isi;
    }
    outputChannels[idx].lastSpike = now;
    outputChannels[idx].hasSpike = true;
    outputChannels[idx].armed = false;
  } else if (!outputChannels[idx].armed && v < SPIKE_RESET_V) {
    outputChannels[idx].armed = true;
  }

  if (outputChannels[idx].hasSpike && (now - outputChannels[idx].lastSpike) > FREQ_TIMEOUT_US) {
    outputChannels[idx].freq = 0.0;
  }

  return outputChannels[idx].freq;
}

float inputBuff[3];
float hiddenBuff[3];

void loop()
{
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
    pollPulse(freq, HID_IN_PINS[i], i);
  }

  for (int i = 0; i < 3; i++)
  {
    float inFreq = pollFrequency(HID_OUT_PINS[i], i);

    float out = HIDDEN_BIAS[i];
    for (int j = 0; j < 3; j++)
    {
      out += HIDDEN_WEIGHTS[i][j] * inputBuff[j];
    }
    out = signActivate(out);

    // TODO: FIX
    if (inFreq < FAIL_FREQUENCY)
    {
      Serial.println("Error: Hidden neuron frequency below 5 Hz. Did you disconnect the neuron? Current frequency: " + String(inFreq));
      out = 0.0;
    }

    hiddenBuff[i] = out;

    float outFreq = encodeToFreq(out);
    pollPulse(outFreq, OUT_IN_PINS[i], i + 3);
  }

  //Serial.println("Error: Info: " + String(hiddenBuff[]));

  {
    float inFreq = pollFrequency(OUT_OUT_PIN, 3);
    float decodedInValue = decodeToValue(inFreq);

    decodedInValue = signActivate(decodedInValue);

    // BEGIN
    float out = OUTPUT_BIAS;
    for (int j = 0; j < 3; j++)
    {
      out += OUTPUT_WEIGHTS[j] * hiddenBuff[j];
    }
    out = signActivate(out);
    // END

    // TODO: FIX
    if (inFreq < FAIL_FREQUENCY)
    {
      out = 0.0;
      Serial.println("Error: Output neuron frequency below 5 Hz. Did you disconnect the neuron? Current frequency: " + String(inFreq));
    }

    Serial.println(String(out));
    Serial.println("Error: " + String(decodedInValue) + ", " + String(out) + ", freq: " + inFreq);
  }

  delay(5);
}