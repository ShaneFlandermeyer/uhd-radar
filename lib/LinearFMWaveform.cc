#include "LinearFMWaveform.h"

#include <iostream>
std::vector<std::complex<float>> LinearFMWaveform::generateWaveform() {
  std::cout << "Generating waveform" << std::endl;
  return std::vector<std::complex<float>>(0);
}

LinearFMWaveform::LinearFMWaveform() {
  bandwidth = 0;
  pulsewidth = 0; 
  sampleRate = 0;
  prf = 0;
}

LinearFMWaveform::LinearFMWaveform(float bandwidth, float pulsewidth, float prf,
                                   float sampleRate) {
  this->bandwidth = bandwidth;
  this->pulsewidth = pulsewidth;
  this->prf = prf;
  this->sampleRate = sampleRate;
}

LinearFMWaveform::~LinearFMWaveform() {}