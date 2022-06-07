import numpy as np
import matplotlib.pyplot as plt
import scipy.signal
x = np.fromfile('/home/shane/cal_30.dat', dtype=np.complex64)
# h = np.fromfile('/home/shane/h.dat', dtype=np.complex64)
# h = np.flipud(h)
# y = scipy.signal.convolve(x, h)
for i in range(5):
  x = np.fromfile('/home/shane/cal_' + str((i+1)*10) + '.dat', dtype=np.complex64)
  plt.figure()
  plt.title(str((i+1)*10) + ' MHz')
  plt.plot(np.abs(x))
plt.show()

