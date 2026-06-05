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
constexpr int BASELINE_SAMPLES = 11;
constexpr unsigned long CALIB_TIMEOUT_MS = 8000;

float neuronBaseline[4];

constexpr float ADC_REFERENCE = 5.0;
constexpr float SPIKE_THRESHOLD_V = 1.0;
constexpr float SPIKE_RESET_V = 0.5;

constexpr unsigned long PULSE_WIDTH_US = 5000 / 3;
constexpr unsigned long FREQ_TIMEOUT_US = 500 * 1000;

constexpr float F_BASELINE = 10.0;
constexpr float F_OFFSET = 5.0;
constexpr float FAIL_FREQUENCY = (F_BASELINE - F_OFFSET) / 2.0;

constexpr float DECODE_SCALE = 1.0;

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

float medianOf(float* a, int n)
{
  if (n <= 0) return 0.0;
  for (int i = 1; i < n; i++)
  {
    float key = a[i];
    int j = i - 1;
    while (j >= 0 && a[j] > key) { a[j + 1] = a[j]; j--; }
    a[j + 1] = key;
  }
  return a[n / 2];
}

void measureBaseline()
{
  const int inPins[6]  = {HID_IN_PINS[0], HID_IN_PINS[1], HID_IN_PINS[2], OUT_IN_PINS[0], OUT_IN_PINS[1], OUT_IN_PINS[2]};
  const int outPins[4] = {HID_OUT_PINS[0], HID_OUT_PINS[1], HID_OUT_PINS[2], OUT_OUT_PIN};

  float samples[4][BASELINE_SAMPLES];
  int   count[4]   = {0, 0, 0, 0};
  bool  armed[4]   = {true, true, true, true};
  bool  hasLast[4] = {false, false, false, false};
  unsigned long lastSpike[4];

  initPulse();
  unsigned long now0 = micros();
  for (int i = 0; i < 4; i++) lastSpike[i] = now0;

  unsigned long startMs = millis();
  bool done = false;
  while (!done && (millis() - startMs < CALIB_TIMEOUT_MS))
  {
    for (int i = 0; i < 6; i++) pollPulse(F_BASELINE, inPins[i], i);

    unsigned long now = micros();
    for (int i = 0; i < 4; i++)
    {
      if (count[i] >= BASELINE_SAMPLES) continue;
      float v = readVoltage(outPins[i]);
      if (armed[i] && v >= SPIKE_THRESHOLD_V)
      {
        if (hasLast[i])
        {
          unsigned long isi = now - lastSpike[i];
          if (isi > 0) samples[i][count[i]++] = 1000000.0 / (float)isi;
        }
        lastSpike[i] = now;
        hasLast[i] = true;
        armed[i] = false;
      }
      else if (!armed[i] && v < SPIKE_RESET_V)
      {
        armed[i] = true;
      }
    }

    done = true;
    for (int i = 0; i < 4; i++) if (count[i] < BASELINE_SAMPLES) done = false;
  }

  for (int i = 0; i < 6; i++) digitalWrite(inPins[i], LOW);

  for (int i = 0; i < 4; i++)
  {
    neuronBaseline[i] = medianOf(samples[i], count[i]);
    Serial.println("Error: Baseline neuron " + String(i) + ": " + String(neuronBaseline[i]) + " Hz (" + String(count[i]) + " samples)");
  }
}

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
  delay(2000);

  Serial.begin(115200);
  for (int i = 0; i < 3; i++) { pinMode(HID_IN_PINS[i], OUTPUT); digitalWrite(HID_IN_PINS[i], LOW); }
  for (int i = 0; i < 3; i++) { pinMode(OUT_IN_PINS[i], OUTPUT); digitalWrite(OUT_IN_PINS[i], LOW); }

  measureBaseline();

  Serial.println(String(neuronBaseline[0]));
  Serial.println(String(neuronBaseline[1]));
  Serial.println(String(neuronBaseline[2]));
  Serial.println(String(neuronBaseline[3]));

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

    float outFreq = encodeToFreq(out * OUTPUT_WEIGHTS[i]);
    pollPulse(outFreq, OUT_IN_PINS[i], i + 3);
  }

  //Serial.println("Error: Info: " + String(hiddenBuff[]));

  {
    float inFreq = pollFrequency(OUT_OUT_PIN, 3);
    float decodedInValue = decodeToValue(inFreq, 3);

    decodedInValue = signActivate(decodedInValue + OUTPUT_BIAS);

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

    //if (random(0, 11) == 10)
    //{
      //Serial.println(String(decodedInValue));
    //}
    //else
    {
      Serial.println(String(out));
    }
    Serial.println("Error: " + String(decodedInValue) + ", " + String(out) + ", freq: " + inFreq);
  }

  delay(5);
}