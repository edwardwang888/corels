import sys
import numpy as np
from sklearn import linear_model
from sklearn.ensemble import RandomForestClassifier
import os
import argparse
import subprocess

def run(X_train, y_train, outfile, r_start, X_test, y_test):
      if os.access(outfile, os.F_OK):
            c = raw_input("File {} already exists. Are you sure you want to continue? (y/N/a) ".format(outfile))
            if c.lower() == "y":
                  os.unlink(outfile)
            elif c.lower() == "a":
                  pass
            else:
                  sys.exit()

      if "logistic" not in outfile and "rforest" not in outfile:
            print("Invalid classfier")
            sys.exit(1)
      
      print("Writing to file: {}".format(outfile))
      nsamples = len(X_test)
      r = 1
      n1 = np.sum(y_test)
      for i in range(140):
            if r <= r_start:
                  if "logistic" in outfile:
                        clf = linear_model.LogisticRegression(C=1/r)
                  elif "rforest" in outfile:
                        clf = RandomForestClassifier(n_estimators=int(1/r), n_jobs=2)
                  
                  clf.fit(X_train, y_train)
                  scores = clf.predict_proba(X_test)

                  initial_obj = n1 * (nsamples - n1)
                  score_string = ""
                  label_string = ""
                  for i in range(nsamples):
                        score_string += "{} ".format(scores[i][1])
                        label_string += "{} ".format(int(y_test[i]))
                  
                  p = subprocess.Popen("./baseline {} {} {} ".format(outfile, initial_obj, r) + score_string + label_string, shell=True)
                  p.wait()
                        
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

def main():
      parser = argparse.ArgumentParser(description="Run baseline tests")
      parser.add_argument("data_train", help="Training data (in ../data/)")
      parser.add_argument("data_test", help="Test data (in ../data/)")
      parser.add_argument("--binary", help="Use binary csv", action="store_true", dest="binary")
      parser.add_argument("--logistic", help="Run logistic regression", action="store_true", dest="logistic")
      parser.add_argument("--rforest", help="Run random forests classifier", action="store_true", dest="rforest")
      parser.add_argument("-r", help="starting regularization", type=float, default=1, action="store", dest="r_start")
      parser.add_argument("-W", help="append text to filename", action="store", dest="text")
      args = parser.parse_args()

      if args.logistic == False and args.rforest == False:
            print("No classifier specified")
            sys.exit(1)

      os.system("gcc -Wall -Wextra -o baseline baseline.c")

      X_train, y_train = get_data(args.data_train, args.binary)
      X_test, y_test = get_data(args.data_test, args.binary)

      text = ""
      if args.binary:
            text += "_binary"
      if args.data_train != args.data_test:
            text += "_val"
      if args.text != None:
            text += "_{}".format(args.text)

      if args.logistic:
            run(X_train, y_train, "{}_logistic{}.csv".format(args.data_train, text), args.r_start, X_test, y_test)
      if args.rforest:
            run(X_train, y_train, "{}_rforest{}.csv".format(args.data_train, text), args.r_start, X_test, y_test)


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