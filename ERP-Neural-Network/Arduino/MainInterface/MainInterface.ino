constexpr int INPUT_NEURON_COUNT = 8;
constexpr int OUTPUT_NEURON_COUNT = 2;

// 0, 1, 6, 7, 2, 3, 4, 5
const int spikeOut[INPUT_NEURON_COUNT] = {2, 3, 6, 6, 6, 6, 4, 5};
const int spikeIn[OUTPUT_NEURON_COUNT] = {A0, A1};

const int jumpSpikeThreshold = 256;

bool wasHigh[OUTPUT_NEURON_COUNT] = {false, false};

unsigned long pulseStart[INPUT_NEURON_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
bool pulseActive[INPUT_NEURON_COUNT] = {false, false, false, false, false, false, false, false};

void startPulse(int idx) {
  digitalWrite(spikeOut[idx], HIGH);
  pulseStart[idx] = micros();
  pulseActive[idx] = true;
}

void updatePulse(int idx) {
  if (pulseActive[idx] && (micros() - pulseStart[idx] >= 12000)) {
    digitalWrite(spikeOut[idx], LOW);
    pulseActive[idx] = false;
  }
}

void setup() {
  Serial.begin(115200);

  for (int idx = 0; idx < INPUT_NEURON_COUNT; idx++)
  {
    pinMode(spikeOut[idx], OUTPUT);
    digitalWrite(spikeOut[idx], LOW);
  }
  for (int idx = 0; idx < OUTPUT_NEURON_COUNT; idx++)
  {
    pinMode(spikeIn[idx], INPUT);
  }
}

void loop() {
  while (Serial.available() > 0)
  {
    byte mask = Serial.read();

    for (int idx = 0; idx < INPUT_NEURON_COUNT; idx++)
    {
      bool state = mask & (1 << idx);
      if (state)
      {
        startPulse(idx);
      }
    }
  }
  
  for (int idx = 0; idx < INPUT_NEURON_COUNT; idx++)
  {
    updatePulse(idx);
  }
  for (int idx = 0; idx < OUTPUT_NEURON_COUNT; idx++)
  {
    int v = analogRead(spikeIn[idx]);
    bool isHigh = (v > jumpSpikeThreshold);
    if (isHigh && !wasHigh[idx])
    {
      byte output = idx;
      Serial.write(output + 23);
    }
    wasHigh[idx] = isHigh;
  }
}