import sys
sys.path[:0] = ["/Library/Python/2.7/site-packages/"]

import numpy as np
import matplotlib.pyplot as plt

dt = np.dtype([('Regularization',np.float32),('Length',np.int32),('Accuracy',np.float32),('Objective',np.float32)])
default = np.genfromtxt('default.csv', delimiter=',', skip_header=1, dtype=dt)
falling = np.genfromtxt('falling.csv', delimiter=',', skip_header=1, dtype=dt)

#Plot regularization vs. objective value
plt.figure(1)
plt.plot(falling[:]['Regularization'], falling[:]['Objective'], 'x-', label='Falling')
plt.plot(default[:]['Regularization'], default[:]['Objective'], '+-', label='Default')
plt.xlabel('Regularization')
plt.ylabel('Min Objective')
plt.xlim(0, 0.01)
plt.ylim(0.325, 0.365)
plt.title("Regularization vs. Min Objective")
plt.legend()

falling.sort(order='Length')
default.sort(order='Length')

#Plot list length vs. accuracy
plt.figure(2)
plt.plot(falling[:]['Length'], falling[:]['Accuracy'], 'x--', label='Falling')
plt.plot(default[:]['Length'], default[:]['Accuracy'], '+--', label='Default')
plt.title("Accuracy vs. Length of Rule List")
plt.xlabel('Length of Rule List')
plt.ylabel('Accuracy')
plt.legend()

#Plot list length vs. objective
plt.figure(3)
plt.plot(falling[:]['Length'], falling[:]['Objective'], 'x', label='Falling')
plt.plot(default[:]['Length'], default[:]['Objective'], '+', label='Default')
plt.title("Min Objective vs. Length of Rule List")
plt.xlabel('Length of Rule List')
plt.ylabel('Min Objective')
plt.legend()
plt.show()