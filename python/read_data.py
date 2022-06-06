import numpy as np
import matplotlib.pyplot as plt
import scipy.signal
x = np.fromfile('/home/shane/x.dat', dtype=np.complex64)
h = np.fromfile('/home/shane/h.dat', dtype=np.complex64)
# h = np.flipud(h)
y = scipy.signal.convolve(x, h)
plt.plot(20*np.log10(np.abs(y)))
plt.show()

