import numpy as np
import matplotlib.pyplot as plt
4
data = np.fromfile('/home/shane/test.dat', dtype=np.complex64)

plt.plot(np.abs(data))
plt.show()

