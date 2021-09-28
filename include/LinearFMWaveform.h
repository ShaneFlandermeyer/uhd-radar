#ifndef LINEARFMWAVEFORM_H
#define LINEARFMWAVEFORM_H

#include <complex>
#include <vector>

class LinearFMWaveform {
  /*
   * Basic class to generate samples for a linear FM waveform
   */
 public:
  // TODO: Make these private
  float bandwidth;
  float pulsewidth;
  float sampleRate;
  float prf;
  std::vector<std::complex<float>> generateWaveform();
  // Default constructor
  LinearFMWaveform();
  // Parameterized constructor
  LinearFMWaveform(float bandwidth, float pulsewidth, float prf,
                   float sampleRate);
  ~LinearFMWaveform();
};

#endif
