import numpy as np
import matplotlib.pyplot as plt
from sklearn.metrics import auc

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

def find_max_roc(outfile_roc):
    with open(outfile_roc, 'rb') as f:
        data = f.readlines()
    
    max_auc = 0
    max_i = 0
    for i in range(len(data)/2):
        fpr = np.genfromtxt([data[2*i].rstrip()])
        tpr = np.genfromtxt([data[2*i+1].rstrip()])
        roc_auc = auc(fpr, tpr)
        if roc_auc > max_auc:
            max_auc = roc_auc
            max_i = i

    fpr = np.genfromtxt([data[2*max_i].rstrip()])
    tpr = np.genfromtxt([data[2*max_i+1].rstrip()])
    return fpr, tpr

def plot(outfile_roc):
    global i
    plt.figure(i)
    fpr, tpr = find_max_roc(outfile_roc)
    max_auc = auc(fpr, tpr)
    plt.plot(fpr, tpr)

    plt.title("Receiver Operating Characteristic")
    plt.xlabel('False Positive Rate')
    plt.ylabel('True Positive Rate')
    plt.plot([0], [0], label=outfile_roc.replace(".csv", "") + "\n(AUC = {})".format(max_auc))
    plt.legend()
    i += 1

def show():
    plt.show()