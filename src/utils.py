import os
import sys
import numpy as np

def check_outfile(outfile, override=False):
    if os.access(outfile, os.F_OK):
        if override == False:
            c = raw_input("File {} already exists. Are you sure you want to continue? (y/N/a) ".format(outfile))
        else:
            c = "y"

        if c.lower() == "y":
            os.unlink(outfile)
        elif c.lower() == "a":
            pass
        else:
            sys.exit()

        return c.lower()

def check_outfile_roc(outfile_roc, override=False):
    if os.access(outfile_roc, os.F_OK):
        if override == False:
            c = raw_input("File {} already exists. Do you want to run again? (y/N/a) ".format(outfile_roc))
        else:
            c = "y"

        if c.lower() == "y":
            os.unlink(outfile_roc)
            return "y"
        elif c.lower() == "a":
            return "a"
        else:
            return "n"

def wpa_objective(z, y):
    wpa = 0
    nsamples = y.shape[0]
    for i in range(nsamples):
        for j in range(nsamples):
            wpa += (int(z[i] >= z[j]) - 0.5 * int(z[i] == z[j])) * int(y[i] > y[j])
    
    n1 = np.sum(y)
    wpa_max = n1 * (nsamples - n1)
    return wpa/wpa_max