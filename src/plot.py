import sys
sys.path[:0] = ["/Library/Python/2.7/site-packages/"]
import numpy as np
import matplotlib.pyplot as plt

dt = np.dtype([('Regularization',np.float32),('Length',np.int32),('Accuracy',np.float32),('Objective',np.float32)])
data = []
names = []
for i in range(1, len(sys.argv)):
    data.append(np.genfromtxt(sys.argv[i], delimiter=',', skip_header=1, dtype=dt))
    # Remove csv extension
    names.append(sys.argv[i][:-4])

if len(data) <= 2:
    marker = ['x','+']
else:
    marker = ['1','2','3','4','+','x']

for i in data:
    i.sort(order=['Regularization'])



#Plot regularization vs. objective value
plt.figure(1)
for i in range(len(data)):
    plt.plot(data[i][:]['Regularization'], data[i][:]['Objective'], marker[i] + '-', label=names[i])
plt.xlabel('Regularization')
plt.ylabel('Min Objective')
plt.title("Regularization vs. Min Objective")
plt.legend()

for i in data:
    i.sort(order=['Length','Accuracy'])



#Plot list length vs. accuracy
plt.figure(2)
for i in range(len(data)):
    plt.plot(data[i][:]['Length'], data[i][:]['Accuracy'], marker[i] + '--', label=names[i])
plt.title("Accuracy vs. Length of Rule List")
plt.xlabel('Length of Rule List')
plt.ylabel('Accuracy')
plt.legend()



#Plot list length vs. objective
plt.figure(3)
for i in range(len(data)):
    plt.plot(data[i][:]['Length'], data[i][:]['Objective'], marker[i], label=names[i])
plt.title("Min Objective vs. Length of Rule List")
plt.xlabel('Length of Rule List')
plt.ylabel('Min Objective')
plt.legend()
plt.show()