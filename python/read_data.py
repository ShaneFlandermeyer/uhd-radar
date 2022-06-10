import numpy as np
import matplotlib.pyplot as plt
import scipy.signal

rates = [12, 25, 50, 100, 200]
starts = [50, 57, 63, 82, 120]

rates = [200]
starts = [120]

for i in range(len(rates)):
  x = np.fromfile('/home/shane/debug.dat',dtype=np.complex64)
  # plt.figure()
  # plt.title(str(rates[i]) + ' MHz')
  # plt.plot(np.real(x[starts[i]:]))
  # plt.figure()
  plt.plot(np.abs(x))
plt.show()

