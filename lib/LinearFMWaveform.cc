#include "LinearFMWaveform.h"

#include <iostream>

/*
 * Generate an LFM waveform
 */
std::vector<std::complex<float>> LinearFMWaveform::generateWaveform() {
  // TODO: Make this a constant somewhere else
  std::complex<float> j(0, 1);
  // Sample interval
  float ts = 1 / sampleRate;
  // Number of samples per pulse
  int nSamplesPulse = static_cast<int>(sampleRate * pulsewidth);
  // Number of samples per PRI
  int nSamplesPri = static_cast<int>(sampleRate / prf);
  std::vector<std::complex<float>> lfm(nSamplesPri, 0);
  float t;
  for (int n = 0; n < nSamplesPulse; n++) {
    t = n * ts;
    float phase =
        -bandwidth / 2 * t + bandwidth / (2 * pulsewidth) * std::pow(t, 2);
    lfm[n] = std::exp(j * (float)(2 * M_PI) * phase);
  }
  return lfm;
}

/*
 * Default constructor
 */
LinearFMWaveform::LinearFMWaveform() {
  bandwidth = 0;
  pulsewidth = 0;
  sampleRate = 0;
  prf = 0;
}

/*
 * Non-default constructor
 */
LinearFMWaveform::LinearFMWaveform(float bandwidth, float pulsewidth, float prf,
                                   float sampleRate) {
  this->bandwidth = bandwidth;
  this->pulsewidth = pulsewidth;
  this->prf = prf;
  this->sampleRate = sampleRate;
}

/*
 * Destructor
 */ 
LinearFMWaveform::~LinearFMWaveform() {}