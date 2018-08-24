import corels_test
import baseline
import roc
import argparse
import tempfile
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy import interp
from sklearn.metrics import auc
import csv
import subprocess
import os
import sys
from time import sleep
from utils import check_outfile, check_outfile_roc

plot_i = 1

def plot_roc(outfile_roc):
    global plot_i
    plt.figure(plot_i)
    with open(outfile_roc, 'rb') as f:
        data = f.readlines()

    tprs = []
    aucs = []
    mean_fpr = np.linspace(0, 1, 100)
    for i in range(len(data)/2):
        fpr = np.genfromtxt([data[2*i].rstrip()])
        tpr = np.genfromtxt([data[2*i+1].rstrip()])
        tprs.append(interp(mean_fpr, fpr, tpr))
        tprs[-1][0] = 0.0
        roc_auc = auc(fpr, tpr)
        aucs.append(roc_auc)
        plt.plot(fpr, tpr, lw=1, alpha=0.3, label='ROC fold %d (AUC = %0.2f)' % (i+1, roc_auc))

    plt.plot([0, 1], [0, 1], linestyle='--', lw=2, color='r',
         label='Luck', alpha=.8)

    mean_tpr = np.mean(tprs, axis=0)
    mean_tpr[-1] = 1.0
    mean_auc = auc(mean_fpr, mean_tpr)
    std_auc = np.std(aucs)
    plt.plot(mean_fpr, mean_tpr, color='b', label=r'Mean ROC (AUC = %0.2f $\pm$ %0.2f)' % (mean_auc, std_auc), lw=2, alpha=.8)
    
    std_tpr = np.std(tprs, axis=0)
    tprs_upper = np.minimum(mean_tpr + std_tpr, 1)
    tprs_lower = np.maximum(mean_tpr - std_tpr, 0)
    plt.fill_between(mean_fpr, tprs_lower, tprs_upper, color='grey', alpha=.2,
                    label=r'$\pm$ 1 std. dev.')

    plt.xlim([-0.05, 1.05])
    plt.ylim([-0.05, 1.05])
    plt.xlabel('False Positive Rate')
    plt.ylabel('True Positive Rate')
    plt.title('Receiver Operating Characteristic')
    plt.legend(loc="lower right")
    plt.text(0, 1, outfile_roc.replace(".csv", ""))
    plot_i += 1

def csv_gen(df, path, cols):
    df.to_csv(path_or_buf=path, columns=cols, sep=' ', header=False, index=False)

def run_corels(args, parser):
    ## Check if outfile exists
    final_outfile = corels_test.gen_filename_roc(args, parser, include_val=False).replace("_roc", "_cross-{}".format(args.num_groups))
    final_outfile_roc = final_outfile.replace(".csv", "_roc.csv")
    if check_outfile_roc(final_outfile_roc) == "n" and args.roc:
        print("Outfile: {}".format(final_outfile))
        plot_roc(final_outfile_roc)
        return

    check_outfile(final_outfile)

    ## Read data
    data = pd.read_csv("../data/{}.out".format(args.data_train), sep=' ', header=None)
    labels = pd.read_csv("../data/{}.label".format(args.data_train), sep=' ', header=None)
    nsamples = data.shape[1] - 1
    size = nsamples/args.num_groups

    outfile = tempfile.NamedTemporaryFile(dir="../data", suffix=".csv")

    ## Run cross validation
    for i in range(args.num_groups):
        print(i)
        outfile_roc = tempfile.NamedTemporaryFile(dir="../data", suffix="_roc.csv")

        X_train = tempfile.NamedTemporaryFile(dir="../data", suffix=".out")
        os.unlink(X_train.name)
        X_train.name = "../data/{}".format(args.data_train)
        if args.wpa:
            X_train.name += "_wpa"
        if args.falling:
            X_train.name += "_falling"

        X_train.name += "-cross-{}_i-{}.out".format(args.num_groups, i)
        
        X_test = tempfile.NamedTemporaryFile(dir="../data", suffix=".out")
        
        y_train = tempfile.NamedTemporaryFile(dir="../data")
        os.unlink(y_train.name)
        y_train.name = X_train.name.replace(".out", ".label")
        
        y_test = tempfile.NamedTemporaryFile(dir="../data")
        os.unlink(y_test.name)
        y_test.name = X_test.name.replace(".out", ".label")

        test_cols = [0] + range(i*size + 1, (i+1)*size + 1)
        train_cols = range(0, i*size + 1) + range((i+1)*size + 1, data.shape[1])

        csv_gen(data, X_test.name, test_cols)
        csv_gen(data, X_train.name, train_cols)
        csv_gen(labels, y_test.name, test_cols)
        csv_gen(labels, y_train.name, train_cols)
        
        cmd = ['python', 'corels_test.py']
        if args.r_start != parser.get_default('r_start'):
            cmd += ['-r', "{}".format(args.r_start)]
        if args.b:
            cmd.append('-b')
        if args.c != None:
            cmd += ['-c', "{}".format(args.c)]
        if args.p != None:
            cmd += ['-p', "{}".format(args.p)]
        if args.falling:
            cmd.append('-d')
        if args.wpa:
            cmd.append('-w')
        if args.step != None:
            cmd += ['-s', args.step]
        if args.max_num_nodes != None:
            cmd += ['-n', args.max_num_nodes]

        cmd += ['-o', outfile.name, outfile_roc.name, '--append', os.path.basename(X_train.name).replace(".out", ""), os.path.basename(X_test.name).replace(".out", "")]
        
        print(cmd)
        p = subprocess.Popen(cmd)
        p.wait()

        fpr, tpr = roc.find_max_roc(outfile_roc.name)
        roc.write_to_file(fpr, tpr, final_outfile_roc)
        outfile_roc.close()

    ## Average runs and output
    csv = pd.read_csv(outfile.name, sep=' ', header=None)
    csv.groupby(0).mean().to_csv(final_outfile, sep=' ', header=False, index=True)

    if args.roc:
        plot_roc(final_outfile_roc)


def get_scores_file(args, name, i):
    outfile_scores = "../data/{}_{}_scores_cross-{}_i-{}.csv".format(args.data_train, name, args.num_groups, i)
    if args.binary:
        outfile_scores = outfile_scores.replace(".csv", "_binary.csv")

    return outfile_scores


def run_baseline(args, parser, name):
    ## Check if outfile exists
    final_outfile = args.data_train + "_{}".format(name)
    if args.binary:
        final_outfile += "_binary"
    
    final_outfile += "_cross-{}.csv".format(args.num_groups)
    final_outfile_roc = final_outfile.replace(".csv", "_roc.csv")
    if check_outfile_roc(final_outfile_roc) == "n" and args.roc:
        print("Outfile: {}".format(final_outfile))
        plot_roc(final_outfile_roc)
        return
    
    check_outfile(final_outfile)
    outfile_scores = get_scores_file(args, name, 0)
    """ if check_outfile_roc(outfile_scores) == "y":
        for i in range(1, args.num_groups):
            outfile_scores = get_scores_file(args, name, i)
            if os.access(outfile_scores, os.F_OK):
                print("Also deleting {}".format(outfile_scores))
                os.unlink(outfile_scores) """

    ## Read data
    data, labels = baseline.get_data(args.data_train, args.binary)
    nsamples = labels.shape[0]
    size = nsamples/args.num_groups
    
    outfile = tempfile.NamedTemporaryFile(dir="../data", suffix="_{}.csv".format(name))

    ## Run cross validation
    for i in range(args.num_groups):
        print(i)
        test_rows = range(i*size, (i+1)*size)
        train_rows = range(0, i*size) + range((i+1)*size, data.shape[0])
        
        X_train = data[train_rows,:]
        X_test = data[test_rows,:]
        y_train = labels[train_rows]
        y_test = labels[test_rows]

        outfile_roc = tempfile.NamedTemporaryFile(dir="../data", suffix="_{}_roc.csv".format(name))
        outfile_scores = get_scores_file(args, name, i)
        baseline.run(X_train, y_train, X_test, y_test, outfile.name, outfile_roc.name, args, append=True, outfile_scores=outfile_scores)
        fpr, tpr = roc.find_max_roc(outfile_roc.name)
        roc.write_to_file(fpr, tpr, final_outfile_roc)
        outfile_roc.close()

        
    ## Average runs and output
    csv = pd.read_csv(outfile.name, sep=' ', header=None)
    csv.groupby(0).mean().to_csv(final_outfile, sep=' ', header=False, index=True)

    if args.roc:
        plot_roc(final_outfile_roc)

def run_baseline_main(args, parser):
    os.system("make baseline")
    sleep(2)
    if args.logistic:
        run_baseline(args, parser, "logistic")
    if args.rforest:
        run_baseline(args, parser, "rforest")
    if args.frl:
        run_baseline(args, parser, "frl")


def main():
    parser = argparse.ArgumentParser(description="Perform cross validation")
    parser.add_argument("--val", help="Number of groups to split dataset into", action="store", required=True, type=int, dest="num_groups")
    subparsers = parser.add_subparsers()

    corels_parser = subparsers.add_parser('corels', parents=[corels_test.parent_parser()], help="Run corels")
    corels_parser.set_defaults(func=run_corels)

    baseline_parser = subparsers.add_parser('baseline', parents=[baseline.parent_parser()], help="Run baseline")
    baseline_parser.set_defaults(func=run_baseline_main)

    args = parser.parse_args()
    args.func(args, parser)

    if args.roc:
        plt.show()


if __name__ == "__main__":
    main()