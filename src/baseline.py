import sys
sys.path[:0] = ["/Library/Python/2.7/site-packages/", "/Users/edwardwang/Documents/falling_rule_list/"]
import numpy as np
import pandas as pd
from sklearn import linear_model
from sklearn.ensemble import RandomForestClassifier
import monotonic.monotonic.sklearn_wrappers as sklearn_wrappers
import os
import argparse
import subprocess
from sklearn import metrics
import matplotlib.pyplot as plt
import roc

def run(X_train, y_train, X_test, y_test, outfile, outfile_roc, args, append=False):
      if os.access(outfile_roc, os.F_OK) and args.roc == True and append == False:
            c = raw_input("File {} already exists. Do you want to run again? (y/N/a) ".format(outfile))
            if c.lower() == "y":
                  os.unlink(outfile)
            elif c.lower() == "a":
                  pass
            else:
                  return

      if os.access(outfile, os.F_OK) and args.roc == False and append == False:
            c = raw_input("File {} already exists. Are you sure you want to continue? (y/N/a) ".format(outfile))
            if c.lower() == "y":
                  os.unlink(outfile)
            elif c.lower() == "a":
                  pass
            else:
                  return

      if "logistic" not in outfile and "rforest" not in outfile and "frl" not in outfile:
            print("Invalid classfier")
            sys.exit(1)
      
      if args.roc == False:
            print("Writing to file: {}".format(outfile))
      else:
            print("\nModel: {}".format(outfile[:-4]))

      nsamples = len(X_test)
      r = args.r_start
      n1 = np.sum(y_test)
      for i in range(140):
            if r <= args.r_start:
                  if "logistic" in outfile:
                        clf = linear_model.LogisticRegression(C=1/r)
                  elif "rforest" in outfile:
                        clf = RandomForestClassifier(n_estimators=int(1/r), n_jobs=2)
                  elif "frl" in outfile:
                        clf = sklearn_wrappers.monotonic_sklearn_fitter(num_steps = int(1/r), min_supp = 5, max_clauses = 2, prior_length_mean = 8, prior_gamma_l_alpha = 1., prior_gamma_l_beta = 0.1, temperature = 1)
                  
                  predictor = clf.fit(X_train, y_train)
                  if "frl" not in outfile:
                        scores = clf.predict_proba(X_test)[:,1]
                  else:
                        scores = predictor.decision_function(X_test)

                  # Calculate objective
                  initial_obj = n1 * (nsamples - n1)
                  score_string = ""
                  label_string = ""
                  for i in range(nsamples):
                        score_string += "{} ".format(scores[i])
                        label_string += "{} ".format(int(y_test[i]))
                  
                  p = subprocess.Popen("./baseline {} {} {} ".format(outfile, initial_obj, r) + score_string + label_string, shell=True)
                  p.wait()

                  # Calculate ROC
                  fpr, tpr, thresholds = metrics.roc_curve(y_test, scores)
                  roc.write_to_file(fpr, tpr, outfile_roc)
                        
            r /= 1.0525

def get_data(dataset, binary):
      if binary:
            file = "../data/{}-binary.csv".format(dataset)
            if (os.access(file, os.F_OK)) == False:
                  file = "../data/{}_binary.csv".format(dataset)
            data = np.genfromtxt(file, delimiter=',', skip_header=1)
            ncol = data.shape[data.ndim-1]
            X_data = data[:,0:ncol-1]
            y_data = data[:,ncol-1]

      else:
            X_data = np.genfromtxt("../data/{}.out".format(dataset), delimiter=' ')
            X_data = X_data[:,1:]
            X_data = X_data.transpose()
            y_data = np.genfromtxt("../data/{}.label".format(dataset), delimiter=' ', skip_header=1)
            y_data = y_data[1:]

      return X_data, y_data
 

def parent_parser():
      parser = argparse.ArgumentParser(add_help=False)
      parser.add_argument("data_train", help="Training data (in ../data/)")
      parser.add_argument("--bin", help="Use binary csv", action="store_true", dest="binary")
      parser.add_argument("--log", help="Run logistic regression", action="store_true", dest="logistic")
      parser.add_argument("--rf", help="Run random forests classifier", action="store_true", dest="rforest")
      parser.add_argument("--frl", help="Run falling rule lists classifier", action="store_true", dest="frl")
      parser.add_argument("--roc", help="Plot ROC curve", action="store_true", dest="roc")
      parser.add_argument("-r", help="starting regularization", type=float, default=1, action="store", dest="r_start")
      parser.add_argument("-W", help="append text to filename", action="store", dest="text")
      return parser


def main():
      parser = argparse.ArgumentParser(parents=[parent_parser()], description="Run baseline tests")
      parser.add_argument("data_test", help="Test data (in ../data/)")
      args = parser.parse_args()

      if args.logistic == False and args.rforest == False and args.frl == False:
            print("No classifier specified")
            sys.exit(1)

      os.system("gcc -Wall -Wextra -o baseline baseline.c")

      X_train, y_train = get_data(args.data_train, args.binary)
      X_test, y_test = get_data(args.data_test, args.binary)

      text = ""
      if args.binary:
            text += "_binary"
      if args.data_train != args.data_test:
            text += "_val-{}".format(args.data_test)
      if args.text != None:
            text += "_{}".format(args.text)

      if args.logistic:
            outfile_roc = "{}_logistic_roc{}.csv".format(args.data_train, text)
            outfile = outfile_roc.replace("_roc", "")
            run(X_train, y_train, X_test, y_test, outfile, outfile_roc, args)
            if args.roc:
                  roc.plot(outfile_roc)

      if args.rforest:
            outfile_roc = "{}_rforest_roc{}.csv".format(args.data_train, text)
            outfile = outfile_roc.replace("_roc", "")
            run(X_train, y_train, X_test, y_test, outfile, outfile_roc, args)
            if args.roc:
                  roc.plot(outfile_roc)

      if args.frl:
            outfile_roc = "{}_frl_roc{}.csv".format(args.data_train, text)
            outfile = outfile_roc.replace("_roc", "")
            run(X_train, y_train, X_test, y_test, outfile, outfile_roc, args)
            if args.roc:
                  roc.plot(outfile_roc)

      if args.roc:
            roc.show()


if __name__ == "__main__":
      main()


#ncol = data.shape[data.ndim-1]
#print(ncol)

# X_train = data[:,0:ncol-1]
# y_train = data[:,ncol-1]

#f = open("{}_logistic.csv".format(sys.argv[1]), "ab")
#f.write("Reg,WPA_Obj\n")


#print("Finished calling baseline")
            # print(pred_string)
            # print(label_string)

            # wpa = n1 * (nsamples - n1)
            # for i in range(nsamples):
            #       for j in range(i):
            #             pass
            #             wpa -= int(scores[i][1] > scores[j][1]) * int(y_train[i] > y_train[j])
            #             if scores[i][1] < scores[j][1] and y_train[i] < y_train[j]:
            #                   wpa -= 1
            #             #print(wpa)

            # output = "{},{}".format(r, wpa)
            # print(output)
            # f.write(output + "\n")
            # f.flush()
            # os.fsync(f.fileno())

"""
================================
Digits Classification Exercise
================================

A tutorial exercise regarding the use of classification techniques on
the Digits dataset.

This exercise is used in the :ref:`clf_tut` part of the
:ref:`supervised_learning_tut` section of the
:ref:`stat_learn_tut_index`.

print(__doc__)

from sklearn import datasets, neighbors, linear_model

digits = datasets.load_digits()
X_digits = digits.data
y_digits = digits.target

n_samples = len(X_digits)

X_train = X_digits[:int(.9 * n_samples)]
y_train = y_digits[:int(.9 * n_samples)]
X_test = X_digits[int(.9 * n_samples):]
y_test = y_digits[int(.9 * n_samples):]

knn = neighbors.KNeighborsClassifier()
logistic = linear_model.LogisticRegression()

print('KNN score: %f' % knn.fit(X_train, y_train).score(X_test, y_test))
print('LogisticRegression score: %f'
      % logistic.fit(X_train, y_train).score(X_test, y_test))
preds = logistic.predict(X_train)
print(len(preds))
print(preds)
"""