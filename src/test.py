import argparse
import os
import sys
import subprocess
import csv
from time import sleep


def run(r, outfile, dataset, falling, wpa, max_num_nodes, b, c, p, val):
    data_minor = "../data/{}.minor".format(dataset)
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

    if val == False:
        cmd += "-o {} ".format(outfile)   
        if os.access(data_minor, os.F_OK) == False:
            cmd = cmd + "../data/{0}.out ../data/{0}.label".format(dataset)
        else:
            cmd = cmd + "../data/{0}.out ../data/{0}.label ../data/{0}.minor".format(dataset)
            
        print("\n" + cmd)
        subprocess.Popen(cmd, shell=True)

    ## Perform cross validation
    else:
        if os.access(data_minor, os.F_OK) == False:
            cmd = cmd + "../data/{0}.out ../data/{0}.label".format(dataset + "_train")
        else:
            cmd = cmd + "../data/{0}.out ../data/{0}.label ../data/{0}.minor".format(dataset + "_train")

        # Execute CORELS on training data
        print(cmd)
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        stdout = p.communicate()[0]
        print(stdout)

        # Read in rule list from log file
        start = stdout.find("../logs/")
        end = stdout.find("\n", start)
        rulelist_file = stdout[start:end]
        with open(rulelist_file, 'rb') as csvfile:
            csvreader = csv.reader(csvfile, delimiter=';')
            rules = csvreader.next()
        
        # Generate cross validation command string
        test_data = "{}_test".format(dataset)
        cmd1 = ['./cross-validate']
        cmd1.append("../data/{}.out".format(test_data))
        cmd1.append("../data/{}.label".format(test_data))
        for i in range(len(rules)-1):
            # Remove predicition from rules
            cmd1.append(rules[i][:rules[i].find('~')])

        # Run cross validation and write objective value to file
        p = subprocess.Popen(cmd1, stdout=subprocess.PIPE)
        objective = p.communicate()[0]
        output = "{} {}".format(r, objective)
        print(cmd1)
        print(output + "\n")
        f = open(outfile, 'ab')
        f.write(output + "\n")
        f.close()


def main():
    parser = argparse.ArgumentParser(description="Run test cases on corels")
    parser.add_argument("dataset", help="dataset to use (in ../data/)")
    parser.add_argument("--val", help="perform cross validation", action="store_true", dest="val")
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

    if args.val:
        os.system("gcc -Wall -Wextra -L/usr/local/lib -lgmpxx -lgmp -DGMP -o cross-validate cross-validate.c rulelib.c")
        sleep(2)
    
    # Create filename
    outfile = args.dataset
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
    if args.r_start != parser.get_default('r_start'):
        outfile += "_r{}".format(args.r_start)
    if args.step != None:
        outfile += "_s{}".format(args.step)
    if args.max_num_nodes != None:
        outfile += "_n{}".format(args.max_num_nodes)
    if args.val:
        outfile += "_val"
    if args.text != None:
        outfile += "_{}".format(args.text)

    outfile += ".csv"

    if os.access(outfile, os.F_OK):
        c = raw_input("File {} already exists. Are you sure you want to continue? (y/N/a) ".format(outfile))
        if c.lower() == "y":
            os.unlink(outfile)
        elif c.lower() == "a":
            pass
        else:
            sys.exit()
    
    r = args.r_start
    if args.step != None and args.r_start != None:
        iter = int(args.r_start/args.step)
    else:
        iter = 200
    for i in range(iter):
        run(r, outfile, args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, args.val)
            
        if args.step == None:
            r /= 1.0525
        else:
            r -= args.step
        
    while r > 0.0000001 and args.step != None:
        run(r, outfile, args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, args.val)
        r /= 1.0525

    #Run with zero regularity
    run(0, outfile, args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, args.val)
    
    

if __name__ == "__main__":
    main()
