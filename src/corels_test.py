import argparse
import os
import sys
sys.path[:0] = ["/Library/Python/2.7/site-packages/"]
import subprocess
import csv
import numpy as np
from sklearn import metrics
import matplotlib.pyplot as plt
import roc
from utils import wpa_objective

failed = 0

def run(r, outfile, outfile_roc, data_train, data_test, falling, wpa, max_num_nodes, b, c, p, ties, random, bound):
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
    if ties != None:
        cmd += "-t {} ".format(ties)
    if random != None:
        cmd += "-R {} ".format(random)
    if bound != None:
        cmd += "-B {} ".format(bound)
    if os.access(data_minor, os.F_OK) == False:
        cmd = cmd + "../data/{0}.out ../data/{0}.label".format(data_train)
    else:
        cmd = cmd + "../data/{0}.out ../data/{0}.label ../data/{0}.minor".format(data_train)

    # Execute CORELS on training data
    print(cmd)
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    stdout, stderr = p.communicate()
    print(stdout)
    print("corels.c: " + stderr)

    # Read in rule list from log file
    if p.poll() != 0:
	global failed
	failed += 1
        return
    start = stdout.find("../logs/")
    end = stdout.find("\n", start)
    rulelist_file = stdout[start:end]
    with open(rulelist_file, 'rb') as csvfile:
        csvreader = csv.reader(csvfile, delimiter=';')
        rules = csvreader.next()
    
    # Generate test command string
    cmd1 = ['./corels_test']
    if wpa:
        cmd1.append("-w")
    cmd1.append("../data/{}.out".format(data_test))
    cmd1.append("../data/{}.label".format(data_test))
    for i in range(len(rules)-1):
        # Remove predicition from rules
        cmd1.append(rules[i][:rules[i].find('~')])

    # Calculate objective and ROC
    p = subprocess.Popen(cmd1, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()

    print("corels_test.c: " + stderr)

    objective = stdout[:stdout.find('\n')]
    output = "{} {}".format(r, objective)
    print(cmd1)
    f = open(outfile, 'ab')
    f.write(output + "\n")
    f.close()

    y_test = np.genfromtxt("../data/{}.label".format(data_test), delimiter = ' ', skip_header=1)
    y_test = y_test[1:y_test.shape[0]]
    scores = np.genfromtxt([stdout[stdout.find('\n'):]], delimiter=' ')
    fpr, tpr, thresholds = metrics.roc_curve(y_test, scores)
    roc.write_to_file(fpr, tpr, outfile_roc)

    print(scores)
    print(output + "\n")

    # Objective sanity check
    # print(wpa_objective(scores, y_test))


def gen_filename_roc(args, parser, include_val=True):
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
    if args.max_num_nodes != None:
        outfile += "_n-{}".format(args.max_num_nodes)
    if args.ties != None:
        outfile += "_t-{}".format(args.ties)
    if args.random != None:
        outfile += "_R-{}".format(args.random)
    if args.bound != None:
        outfile += "_B-{}".format(args.bound)
    if args.r != None:
        outfile += "_r-{}".format(args.r)
    if include_val and args.data_train != args.data_test:
        outfile += "_val-{}".format(args.data_test)
    # if args.roc:
    outfile += "_roc"
    if args.text != None:
        outfile += "_{}".format(args.text)

    outfile += ".csv"
    return outfile



def parent_parser():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("data_train", help="Training data (in ../data/)")
    parser.add_argument("--roc", help="Plot ROC curve", action="store_true", dest="roc")
    parser.add_argument("-r_start", help="starting regularization", action="store", type=float, default=0.7515, dest="r_start")
    parser.add_argument("-r", help="single regularization to run", action="store", type=float, dest="r")
    parser.add_argument("-b", help="breadth first search", action="store_true", dest="b")
    parser.add_argument("-c", help="best first search policy", type=int, choices=[1,2,3,4], action="store", dest="c")
    parser.add_argument("-p", help="symmetry aware map", type=int, choices=[0,1,2], action="store", dest="p")
    parser.add_argument("-d", help="use falling constraint", action="store_true", dest="falling")
    parser.add_argument("-w", help="use WPA objective function", action="store_true", dest="wpa")
    parser.add_argument("-W", help="add text to file name", action="store", dest="text")
    parser.add_argument("-s", help="regularization step", action="store", type=float, dest="step")
    parser.add_argument("-n", help="maximum number of nodes (default 100000)", type=int, action="store", dest="max_num_nodes")
    parser.add_argument("-t", help="optimize with ties", type=float, action="store", dest="ties")
    parser.add_argument("-R", help="random search", type=float, action="store", dest="random")
    parser.add_argument("-B", help="lower bound threshold", type=float, action="store", dest="bound")
    return parser

def main():
    parser = argparse.ArgumentParser(parents=[parent_parser()], description="Run test cases on corels")
    parser.add_argument("data_test", help="Test data (in ../data)")
    parser.add_argument("-o", help="override default output file names [outfile, outfile_roc]", action="store", nargs=2, dest="outfile")
    parser.add_argument("--append", help="append to file", action="store_true", dest="append")
    args = parser.parse_args()

    if args.data_test == None:
        args.data_test = args.data_train

    os.system("make")
    os.system("make corels_test")
    
    # Create filename
    if args.outfile != None:
        outfile = args.outfile[0]
        outfile_roc = args.outfile[1]
    else:
        outfile_roc = gen_filename_roc(args, parser)
        outfile = outfile_roc.replace("_roc", "")

    if os.access(outfile_roc, os.F_OK) and args.append == False:
        c = raw_input("File {} already exists. Do you want to run again? (y/N/a) ".format(outfile_roc))
        if c.lower() == "y":
            os.unlink(outfile_roc)
        elif c.lower() == "a":
            pass
        else:
            roc.plot(outfile_roc)
            roc.show()
            sys.exit()

    if os.access(outfile, os.F_OK) and args.append == False:
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
        iter = 140

    if args.r == None:
        for i in range(iter):
            if r <= args.r_start:
                run(r, outfile, outfile_roc, args.data_train, args.data_test, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, args.ties, args.random, args.bound)

            if args.step == None:
                r /= 1.0525
            else:
                r -= args.step
            
        while r > 0.0000001 and args.step != None:
            run(r, outfile, outfile_roc, args.data_train, args.data_test, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, args.ties, args.random, args.bound)
            r /= 1.0525

        #Run with zero regularity
        run(0, outfile, outfile_roc, args.data_train, args.data_test, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, args.ties, args.random, args.bound)

    else:
        run(args.r, outfile, outfile_roc, args.data_train, args.data_test, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, args.ties, args.random, args.bound)


    if args.roc:
        roc.plot(outfile_roc)
        roc.show()

    sys.stderr.write("{}".format(failed))
   

if __name__ == "__main__":
    main()
