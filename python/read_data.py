import numpy as np
import matplotlib.pyplot as plt
import scipy.signal
x = np.fromfile('/home/shane/cal.dat', dtype=np.complex64)
# h = np.fromfile('/home/shane/h.dat', dtype=np.complex64)
# h = np.flipud(h)
# y = scipy.signal.convolve(x, h)
plt.plot(np.abs(x))
plt.show()

