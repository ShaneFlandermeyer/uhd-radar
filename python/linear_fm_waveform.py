import numpy as np
import matplotlib.pyplot as plt


class LinearFMWaveform():
    """
    Basic class to generate LFM waveform samples

    TODO: Add a way to generate multiple pulses at once 
    TODO: Add mutli-prf 
    TODO: Add an option for up and down chirping
    TODO: Add symmetric and positive sweep option (see matlab phased.LinearFMSource)
    TODO: Create a waveform namespace
    """

    def __init__(self, bandwidth=1e6, pulsewidth=100e-6, sampleRate=1e6, prf=1e3):
        self.bandwidth = bandwidth
        self.pulsewidth = pulsewidth
        self.sampleRate = sampleRate
        self.prf = prf

    def generateWaveform(self):
        """
        Generate one PRI of the LFM
        """
        wav = np.zeros((round(self.sampleRate/self.prf),), dtype=np.complex64)
        Ts = 1 / self.sampleRate
        t = np.arange(0, self.pulsewidth-Ts, Ts)
        phase = -self.bandwidth/2*t + self.bandwidth / \
            (2*self.pulsewidth)*np.square(t)
        wav[0:len(phase)] = np.exp(1j*phase)
        return wav


def main():
    """
    Plot the default waveform
    """
    lfm = LinearFMWaveform()
    wav = lfm.generateWaveform()
    plt.plot(np.real(wav))
    plt.show()


if __name__ == '__main__':
    main()
