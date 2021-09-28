# -----------------------------------------------
# This script is desinged to run a simultaneous transmit and recieve using a
# USRP X310 Radio.
#
#
# This script should initialize the radio, wait for a user trigger from the
# keyboard and the send the start commands a 1 second later.
#
# Author: Rylee Mattingly
# -----------------------------------------------
import sys
import argparse
import uhd
import math
import matplotlib.pyplot as plt
import numpy as np
from uhd import rfnoc
import threading

from uhd.usrp.libtypes import RXStreamer

def parse_args():
    description = """Simultaneous TX and RX
        Utility to Transmit from a file and store resul in a file,
        
        Use --help for a full list of arguments.
        """
    # Parse possible arguments from the command line.
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter,
                                     description=description)
    parser.add_argument("--master_clock_rate", default=184.32e6, type=float,
                        help="""Specify FPGA master clock rate: 184.32e6 or 200e6.
                        This will set the output and input sample rate at the frontend""")
    parser.add_argument("--tx_rate", default=30.72e6, type=float, help="Specify the sample rate of the file.")
    parser.add_argument("--rx_rate", default=30.72e6, type=float, help="Specify the sample rate to capture to file.")
    parser.add_argument("--tx_file", default="TestPulses.tx", type=str, help="Name of the file to read samples from.")
    parser.add_argument("--rx_file", default="Test.rx", type=str, help="Name of file to write samples to.")
    parser.add_argument("--cent_freq", default=5e9, type=float, help="Center Frequency")
    parser.add_argument("--tx_gain", default=0, type=int, help="Transmit Gain in dB. 0 to 30")
    parser.add_argument("--rx_gain", default=0, type=int, help="Recieve Gain in dB. 0 to 30")
    parser.add_argument("-p", "--plot", default=False, type=bool, help="Plot recieved data?")
    return parser.parse_args()


def main():
    # Parse the arguments
    args = parse_args()

    # Check gain is in bounds
    if args.tx_gain < 0 or args.tx_gain > 30:
        print("TX Gain out of bounds. 0 to 30 dB is valid.")
        sys.exit(1)
    if args.rx_gain < 0 or args.rx_gain > 30:
        print("RX Gain out of bounds. 0 to 30 dB is valid.")
        sys.exit(1)

    #Check the clock rate
    if not(args.master_clock_rate == 200e6 or args.master_clock_rate == 184.32e6):
        print("Invalid clock rate: Only accepts 200e6 or 184.32e6")
        exit

    # Create the graph to build on top of.

    graph = uhd.rfnoc.RfnocGraph("addr=192.168.30.2,master_clock_rate=" + str(args.master_clock_rate))

    # Let's initialize the DUC hardware block.
    ducBlock = uhd.rfnoc.DucBlockControl(graph.get_block('0/DUC#0'))
    ducBlock.set_input_rate(args.tx_rate, 0) # Set the input rate of the block.
    ducBlock.set_output_rate(args.master_clock_rate, 0) # Set the output rate of the block.
    # Print the upconversion rate.
    print('Upconversion Rate:', args.master_clock_rate/args.tx_rate)

    # Setup the radio block for transmission.
    txBlock = uhd.rfnoc.RadioControl(graph.get_block("0/Radio#0"))
    txBlock.set_tx_antenna("TX/RX", 0)
    #print("Selected Antenna: ", txBlock.get_tx_antennas(0))
    txBlock.set_tx_gain(args.tx_gain, 0)
    #print("Gain: ", str(txBlock.get_tx_gain(0)))
    txBlock.set_tx_bandwidth(args.tx_rate, 0)
    #print("Bandwidth: ", str(txBlock.get_tx_bandwidth(0)))
    txBlock.set_tx_dc_offset(False, 0)
    txBlock.set_tx_frequency(args.cent_freq, 0)
    #print("Center Frequency: ", str(txBlock.get_tx_frequency(0)))

    # Let's initialize the DDC hardware block
    ddcBlock = uhd.rfnoc.DdcBlockControl(graph.get_block('0/DDC#0'))
    ddcBlock.set_input_rate(args.master_clock_rate, 0) # Set the input rate of the block.
    ddcBlock.set_output_rate(args.rx_rate, 0) # Set the output rate of the block.
    print('Downconversion Rate:', args.master_clock_rate/args.rx_rate)

    # Let's initialize the RX streamer blocks
    rxBlock = uhd.rfnoc.RadioControl(graph.get_block("0/Radio#0"))
    rxBlock.set_rx_antenna("RX2", 0)
    rxBlock.set_rx_gain(args.rx_gain, 0)
    rxBlock.set_rx_bandwidth(args.rx_rate, 0)
    rxBlock.set_rx_dc_offset(False, 0)
    rxBlock.set_rx_frequency(args.cent_freq, 0)

    # Print details about the RX radio.
    print("TX: -----------------------")
    print("   Antenna: ", txBlock.get_tx_antenna(0))
    print("   Gain: ", str(txBlock.get_tx_gain(0)))
    print("   Bandwidth: ", str(txBlock.get_tx_bandwidth(0)))
    print("   Frequency: ", str(txBlock.get_rx_frequency(0)))

    # Print details about the RX radio.
    print("RX: -----------------------")
    print("   Antenna: ", txBlock.get_rx_antenna(0))
    print("   Gain: ", str(txBlock.get_rx_gain(0)))
    print("   Bandwidth: ", str(txBlock.get_rx_bandwidth(0)))
    print("   Frequency: ", str(txBlock.get_rx_frequency(0)))


    # Define the streaming args.
    streamArgs = uhd.usrp.StreamArgs("fc32", "sc16")
    streamArgs.args = "spp=1024"
    # Let's Create the TX Streamer using the args.
    txStreamer = graph.create_tx_streamer(1, streamArgs)
    # Let's Create the RX Streamer using the args.
    rxStreamer = graph.create_rx_streamer(1, streamArgs)

    # Connect the Rx chain.
    graph.connect(rxBlock.get_unique_id(), 0, ddcBlock.get_unique_id(), 0, False)
    graph.connect(ddcBlock.get_unique_id(), 0, rxStreamer, 0, False)
    # Connect the Tx Chain.
    graph.connect(txStreamer, 0, ducBlock.get_unique_id(), 0, False)
    graph.connect(ducBlock.get_unique_id(), 0, txBlock.get_unique_id(), 0, False)

    # Number of preload samples.
    leadSize = 1024*15
    preTxData = np.zeros([1, leadSize], dtype=np.complex64)

    # Read in the data to be sent and initialize the output to sufficient size.
    txData = np.fromfile(args.tx_file, dtype=np.complex64)
    txSize = np.size(txData, 0)
    print("Tx Sample Count: ", txSize)
    txTime = txSize/args.tx_rate
    rxData = np.zeros([1, math.ceil(args.rx_rate * txTime) + leadSize + 42], dtype=np.complex64)
    print("Rx Sample Count: ", np.size(rxData, 1))

    # Commit the graph.
    graph.commit()

    #####################################
    # Do as much as possible before here.
    #####################################
    input("Press Enter to continue...")

    # Create the metadata structures for the commands
    rxMeta = uhd.types.RXMetadata()
    txMeta1 = uhd.types.TXMetadata()
    txMeta2 = uhd.types.TXMetadata()

    # Create a stream command.
    streamCMD = uhd.types.StreamCMD(uhd.types.StreamMode.start_cont)
    streamCMD.stream_now = False
    # Create the threads for send and recieve.
    sendProcess = threading.Thread(target=doRecv, args=(rxStreamer, rxData, rxMeta))
    recvProcess = threading.Thread(target=doSend, args=(txStreamer, preTxData, txData, txMeta1, txMeta2))

    # Get the current time.
    motherboard = graph.get_mb_controller()
    timekeeper = motherboard.get_timekeeper(0)
    # Add timespec to the metadata
    goTime = timekeeper.get_time_now()

    txMeta1.has_time_spec = True
    txMeta1.time_spec = goTime + 0.1
    streamCMD.time_spec = goTime + 0.1

    # Issue the stream cmd and start the recv and send threads.
    rxBlock.issue_stream_cmd(streamCMD, 0)
    recvProcess.start()
    sendProcess.start()
    # Wait for the threads to return.
    recvProcess.join()
    sendProcess.join()
    # Issue the stop command and release the graph.
    stopCMD = uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont)
    stopCMD.stream_now = True
    rxBlock.issue_stream_cmd(stopCMD, 0)
    graph.release()

    #Write the numpy variable to file
    outputFile = open(args.rx_file, "wb")
    rxData[0, leadSize + 42:].tofile(outputFile)

# Call the recieve command.
def doRecv(rxStreamer, rxData, rxMeta):
    recieved = rxStreamer.recv(rxData, rxMeta)
    print("\n", recieved)
    print("\n", rxMeta)


def doSend(txStreamer, txData1, txData2, txMeta1, txMeta2):
    txStreamer.send(txData1, txMeta1)
    txStreamer.send(txData2, txMeta2)
    

def plot():
    rxData = np.fromfile('Test.rx', dtype=np.complex64)
    plt.plot(np.abs(rxData))
    plt.show()

if __name__ == "__main__":
    main()
    args = parse_args()
    if args.plot:
        plot()
