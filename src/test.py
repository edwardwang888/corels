import argparse
import os
import sys
sys.path[:0] = ["/Library/Python/2.7/site-packages/"]
import subprocess
import csv
from time import sleep
import numpy as np
from sklearn import metrics
import matplotlib.pyplot as plt
import roc


def run(r, outfile, outfile_roc, data_train, data_test, falling, wpa, max_num_nodes, b, c, p):
    data_minor = "../data/{}.minor".format(data_train)
    #Start assembling command string
    cmd = "./corels -r {} ".format(r)
    if b:
        cmd += "-b "
    if c != None:
        cmd += "-c {} ".format(c)
    if p != None:
        cmd += "-p {} ".format(p)
    if falling:
        cmd = cmd + "-d "
    if wpa:
        cmd = cmd + '-w '
    if max_num_nodes != None:
        cmd += "-n {} ".format(max_num_nodes)
    if os.access(data_minor, os.F_OK) == False:
        cmd = cmd + "../data/{0}.out ../data/{0}.label".format(data_train)
    else:
        cmd = cmd + "../data/{0}.out ../data/{0}.label ../data/{0}.minor".format(data_train)

    # Execute CORELS on training data
    print(cmd)
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    stdout = p.communicate()[0]
    print(stdout)

    # Read in rule list from log file
    if p.poll() != 0:
        return
    start = stdout.find("../logs/")
    end = stdout.find("\n", start)
    rulelist_file = stdout[start:end]
    with open(rulelist_file, 'rb') as csvfile:
        csvreader = csv.reader(csvfile, delimiter=';')
        rules = csvreader.next()
    
    # Generate test command string
    cmd1 = ['./test']
    cmd1.append("../data/{}.out".format(data_test))
    cmd1.append("../data/{}.label".format(data_test))
    for i in range(len(rules)-1):
        # Remove predicition from rules
        cmd1.append(rules[i][:rules[i].find('~')])

    # Calculate objective and ROC
    p = subprocess.Popen(cmd1, stdout=subprocess.PIPE)
    stdout = p.communicate()[0]

    objective = stdout[:stdout.find('\n')]
    output = "{} {}".format(r, objective)
    print(cmd1)
    print(output + "\n")
    f = open(outfile, 'ab')
    f.write(output + "\n")
    f.close()

    y_test = np.genfromtxt("../data/{}.label".format(data_test), delimiter = ' ', skip_header=1)
    y_test = y_test[1:y_test.shape[0]]
    scores = np.genfromtxt([stdout[stdout.find('\n'):]], delimiter=' ')
    fpr, tpr, thresholds = metrics.roc_curve(y_test, scores)
    roc.write_to_file(fpr, tpr, outfile_roc)


def main():
    parser = argparse.ArgumentParser(description="Run test cases on corels")
    parser.add_argument("data_train", help="Training data (in ../data/)")
    parser.add_argument("data_test", help="Test data (in ../data)")
    parser.add_argument("--roc", help="Plot ROC curve", action="store_true", dest="roc")
    parser.add_argument("-r", help="starting regularization", action="store", type=float, default=0.7515, dest="r_start")
    parser.add_argument("-b", help="breadth first search", action="store_true", dest="b")
    parser.add_argument("-c", help="best first search policy", type=int, choices=[1,2,3,4], action="store", dest="c")
    parser.add_argument("-p", help="symmetry aware map", type=int, choices=[0,1,2], action="store", dest="p")
    parser.add_argument("-d", help="use falling constraint", action="store_true", dest="falling")
    parser.add_argument("-w", help="use WPA objective function", action="store_true", dest="wpa")
    parser.add_argument("-W", help="add text to file name", action="store", dest="text")
    parser.add_argument("-s", help="regularization step", action="store", type=float, dest="step")
    parser.add_argument("-n", help="maximum number of nodes (default 100000)", type=int, action="store", dest="max_num_nodes")
    args = parser.parse_args()

    os.system("gcc -L/usr/local/lib -lgmpxx -lgmp -DGMP -o test test.c rulelib.c")
    sleep(2)
    
    # Create filename
    outfile = args.data_train
    if args.wpa:
        outfile += "_wpa"
    if args.falling:
        outfile += "_falling"
    if args.b:
        outfile += "_b"
    if args.c != None:
        outfile += "_c{}".format(args.c)
    if args.p != None:
        outfile += "_p{}".format(args.p)
    if args.r_start != parser.get_default('r_start') and args.step != None:
        outfile += "_r{}".format(args.r_start)
    if args.step != None:
        outfile += "_s{}".format(args.step)
    if args.max_num_nodes != None:
        outfile += "_n{}".format(args.max_num_nodes)
    if args.data_train != args.data_test:
        outfile += "_val-{}".format(args.data_test)
    # if args.roc:
    outfile += "_roc"
    if args.text != None:
        outfile += "_{}".format(args.text)

    outfile += ".csv"

    outfile_roc = outfile
    outfile = outfile.replace("_roc", "")

    if os.access(outfile_roc, os.F_OK):
        c = raw_input("File {} already exists. Do you want to run again? (y/N/a) ".format(outfile_roc))
        if c.lower() == "y":
            os.unlink(outfile_roc)
        elif c.lower() == "a":
            pass
        else:
            roc.plot(outfile_roc)
            roc.show()
            sys.exit()

    if os.access(outfile, os.F_OK):
        c = raw_input("File {} already exists. Are you sure you want to continue? (y/N/a) ".format(outfile))
        if c.lower() == "y":
            os.unlink(outfile)
        elif c.lower() == "a":
            pass
        else:
            sys.exit()

    
    r = parser.get_default('r_start')
    if args.step != None and args.r_start != None:
        iter = int(args.r_start/args.step)
    else:
        iter = 200
    for i in range(iter):
        if r <= args.r_start:
            run(r, outfile, outfile_roc, args.data_train, args.data_test, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p)
            
        if args.step == None:
            r /= 1.0525
        else:
            r -= args.step
        
    while r > 0.0000001 and args.step != None:
        run(r, outfile, outfile_roc, args.data_train, args.data_test, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p)
        r /= 1.0525

    #Run with zero regularity
    run(0, outfile, outfile_roc, args.data_train, args.data_test, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p)

    if args.roc:
        roc.plot(outfile_roc)
        roc.show()
   

if __name__ == "__main__":
    main()
