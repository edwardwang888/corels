import sys
sys.path[:0] = ["/Library/Python/2.7/site-packages/"]
import numpy as np
import matplotlib.pyplot as plt
import os
import fnmatch

all_files = ["wpa.csv", "falling.csv", "default.csv", "default.csv"]
files = []
for i in all_files:
    if os.access(i, os.F_OK):
        files.append(i)

print(files)

if len(files) > 2:
    print("Error: More than two CSV files in directory")
    sys.exit()

dt = np.dtype([('Regularization',np.float32),('Length',np.int32),('Accuracy',np.float32),('Objective',np.float32)])
data1 = np.genfromtxt(files[0], delimiter=',', skip_header=1, dtype=dt)
data2 = np.genfromtxt(files[1], delimiter=',', skip_header=1, dtype=dt)

#Remove csv extension
name1 = files[0][:-4].capitalize()
name2 = files[1][:-4].capitalize()

#Plot regularization vs. objective value
plt.figure(1)
plt.plot(data1[:]['Regularization'], data1[:]['Objective'], 'x-', label=name1)
plt.plot(data2[:]['Regularization'], data2[:]['Objective'], '+-', label=name2)
plt.xlabel('Regularization')
plt.ylabel('Min Objective')
plt.title("Regularization vs. Min Objective")
plt.legend()

data1.sort(order=['Length','Accuracy'])
data2.sort(order=['Length','Accuracy'])

#Plot list length vs. accuracy
plt.figure(2)
plt.plot(data1[:]['Length'], data1[:]['Accuracy'], 'x--', label=name1)
plt.plot(data2[:]['Length'], data2[:]['Accuracy'], '+--', label=name2)
plt.title("Accuracy vs. Length of Rule List")
plt.xlabel('Length of Rule List')
plt.ylabel('Accuracy')
plt.legend()

#Plot list length vs. objective
plt.figure(3)
plt.plot(data1[:]['Length'], data1[:]['Objective'], 'x', label=name1)
plt.plot(data2[:]['Length'], data2[:]['Objective'], '+', label=name2)
plt.title("Min Objective vs. Length of Rule List")
plt.xlabel('Length of Rule List')
plt.ylabel('Min Objective')
plt.legend()
plt.show()