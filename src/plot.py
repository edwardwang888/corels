import sys
sys.path[:0] = ["/Library/Python/2.7/site-packages/"]
import numpy as np
import matplotlib.pyplot as plt

dt1 = np.dtype([('Regularization',np.float32),('Length',np.int32),('Accuracy',np.float32),('Objective',np.float32)])
dt2 = np.dtype([('Regularization',np.float32),('Objective',np.float32)])
data = []
names = []
plot_all = True
for i in range(1, len(sys.argv)):
    filename = sys.argv[i]
    if "logistic" in filename or "rforest" in filename or "frl" in filename or "cross" in filename:
        plot_all = False
        data.append(np.genfromtxt(filename, delimiter=' ', dtype=dt2))
    else:
        data.append(np.genfromtxt(filename, delimiter=',', skip_header=1, dtype=dt1))
    # Remove csv extension
    names.append(filename[:-4])

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



if plot_all:
    for i in data:
        i.sort(order=['Length','Accuracy'])

#Plot list length vs. accuracy
if plot_all:
    plt.figure(2)
    for i in range(len(data)):
        plt.plot(data[i][:]['Length'], data[i][:]['Accuracy'], marker[i] + '--', label=names[i])
    plt.title("Accuracy vs. Length of Rule List")
    plt.xlabel('Length of Rule List')
    plt.ylabel('Accuracy')
    plt.legend()



#Plot list length vs. objective
if plot_all:
    plt.figure(3)
    for i in range(len(data)):
        plt.plot(data[i][:]['Length'], data[i][:]['Objective'], marker[i], label=names[i])
    plt.title("Min Objective vs. Length of Rule List")
    plt.xlabel('Length of Rule List')
    plt.ylabel('Min Objective')
    plt.legend()
    
    
plt.show()