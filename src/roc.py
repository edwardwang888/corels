import numpy as np
import matplotlib.pyplot as plt

i = 1

def write_to_file(fpr, tpr, outfile_roc):
    with open(outfile_roc, 'ab') as f:
        for i in range(fpr.shape[0]):
            f.write("{} ".format(fpr[i]))
        f.write("\n")
        for i in range(tpr.shape[0]):
            f.write("{} ".format(tpr[i]))
        f.write("\n")
        f.close()

def plot(outfile_roc):
    global i
    plt.figure(i)
    with open(outfile_roc, 'rb') as f:
        data = f.readlines()
    
    for i in range(len(data)/2):
        fpr = np.genfromtxt([data[2*i].rstrip()])
        tpr = np.genfromtxt([data[2*i+1].rstrip()])
        plt.plot(fpr, tpr)

    plt.title("Receiver Operating Characteristic")
    plt.xlabel('False Positive Rate')
    plt.ylabel('True Positive Rate')
    plt.plot([0], [0], label=outfile_roc.replace(".csv", ""))
    plt.legend()
    i += 1

def show():
    plt.show()