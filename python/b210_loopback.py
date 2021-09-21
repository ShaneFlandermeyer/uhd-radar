# -----------------------------------------------
# This script is designed to run a simultaneous transmit and recieve using a
# USRP B210 Radio.
#
# Usage: TODO
#
# Author: Shane Flandermeyer
# -----------------------------------------------
from python.linear_fm_waveform import LinearFMWaveform
import numpy as np
import matplotlib.pyplot as plt
import uhd
import threading
from uhd.usrp.multi_usrp import MultiUSRP
from uhd import libpyuhd as lib


def main():
    # Tx/Rx parameters
    # TODO: For now, leave these hard-coded
    txGain = 50
    rxGain = 30
    txFreq = 5e9
    rxFreq = txFreq
    channels = (0,)
    usrp = uhd.usrp.MultiUSRP()
    # Waveform parameters
    nPulses = 1000
    bandwidth = 10e6
    pulsewidth = 10e-6
    sampleRate = 10e6
    prf = 1e3
    lfm = LinearFMWaveform(bandwidth, pulsewidth, sampleRate, prf)
    txData = lfm.generateWaveform()
    # Repeat the waveform nPulses times
    txData = np.tile(txData, nPulses)

    # Set up the Tx process

    # Set up the sample rate, center frequency and gain of each Tx/Rx channel
    usrp.get_rx_gain_range()
    for chan in channels:
        super(MultiUSRP, usrp).set_tx_rate(sampleRate, chan)
        super(MultiUSRP, usrp).set_rx_rate(sampleRate, chan)
        super(MultiUSRP, usrp).set_tx_freq(
            lib.types.tune_request(txFreq), chan)
        super(MultiUSRP, usrp).set_rx_freq(
            lib.types.tune_request(rxFreq), chan)
        super(MultiUSRP, usrp).set_tx_gain(txGain, chan)
        super(MultiUSRP, usrp).set_rx_gain(rxGain, chan)
        

    streamArgs = lib.usrp.stream_args("fc32", "sc16")
    streamArgs.channels = channels
    txMeta = lib.types.tx_metadata()
    rxMeta = lib.types.rx_metadata()
    txStreamer = super(MultiUSRP, usrp).get_tx_stream(streamArgs)
    rxStreamer = super(MultiUSRP,usrp).get_rx_stream(streamArgs)
    rxData = np.zeros(np.shape(txData),dtype=np.complex64)
    
    streamCmd = lib.types.stream_cmd(lib.types.stream_mode.start_cont)
    streamCmd.stream_now = True
    rxStreamer.issue_stream_cmd(streamCmd)
    
    txProcess = threading.Thread(target=txStreamer.send,args=(txData,txMeta))
    rxProcess = threading.Thread(target=rxStreamer.recv,args=(rxData,rxMeta))
    txProcess.start()
    rxProcess.start()
    txProcess.join()
    rxProcess.join()
    
    stopCmd = uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont)
    rxStreamer.issue_stream_cmd(stopCmd)
    plt.plot(np.real(rxData))
    plt.show()

if __name__ == "__main__":
    main()
