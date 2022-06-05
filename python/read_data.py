import numpy as np
import matplotlib.pyplot as plt

data = np.fromfile('/home/shane/gui_output.dat', dtype=np.complex64)

plt.plot(np.abs(data))
plt.show()

