import cross_validate
import corels_test
import argparse
import sys
import numpy as np
import pandas as pd
from sklearn.metrics import auc
import matplotlib.pyplot as plt


def run(args, parser):
    auc_matrix = np.ndarray(shape=(args.num_groups, len(args.reg)))
    len_matrix = np.ndarray(shape=(args.num_groups, len(args.reg)))
    for r in range(len(args.reg)):
        args.r = args.reg[r]

        if args.method == "corels":
            outfile_all, outfile_len = cross_validate.run_corels(args, parser)
        elif args.method == "baseline" and args.frl:
            outfile_all, outfile_len = cross_validate.run_baseline(args, parser, "frl")
        else:
            print("Only falling rule lists is allowed.")
            sys.exit(1)

        obj = pd.read_csv(outfile_all, sep=' ', header=None)
        for i in range(obj.shape[0]):
            auc_matrix[i,r] = obj.iloc[i,1]

        len_matrix[:,r] = np.fromfile(outfile_len, dtype=int, sep=" ")

    print(auc_matrix)
    print(len_matrix)
    return auc_matrix, len_matrix

def get_parser():
    parser = argparse.ArgumentParser(parents=[cross_validate.get_parser()], description="Generate AUC boxplot")
    parser.add_argument("-r", help="regularization value (multiple allowed)", action="append", type=float, dest="reg")
    parser.add_argument("--title", help="plot title", action="store", dest="title")
    return parser

def errorbar(auc_matrix, len_matrix, labels, fmt='o', color='b'):
    auc_mean = np.mean(auc_matrix, axis=0)
    auc_std = np.std(auc_matrix, axis=0)
    len_mean = np.mean(len_matrix, axis=0)
    len_std = np.std(len_matrix, axis=0)

    for i in range(len(labels)):
        plt.errorbar(len_mean[i], auc_mean[i], xerr=len_std[i], yerr=auc_std[i], fmt=fmt, label=labels[i], mew=2*i, color=color)

    plt.legend()

def boxplot(auc_matrix, len_matrix, args, parser):
    boxplot = plt.boxplot(auc_matrix, positions=np.mean(len_matrix, axis=0), widths=0.3, labels=args.reg)
    print(boxplot)
    colors=['b','g','r','c','m','y','k']
    ls = ['-','--','--','-']
    mk = ['','','>','o']

    def set_color(str):
        for i in range(len(boxplot[str])):
            plt.setp(boxplot[str][i], color=colors[int(i*len(boxplot['boxes'])/len(boxplot[str]))], linewidth=2, linestyle=ls[int(i*len(boxplot['boxes'])/len(boxplot[str]))], marker=mk[int(i*len(boxplot['boxes'])/len(boxplot[str]))], fillstyle='full')

    set_color('boxes')
    set_color('fliers')
    set_color('whiskers')
    set_color('medians')
    set_color('caps')

    print(boxplot['boxes'][0].properties())

    plt.xlabel("Model Size")
    plt.ylabel("AUC")
    x_max = int(np.max(np.mean(len_matrix, axis=0))) + 1
    plt.xticks(range(1, x_max+1), range(1, x_max+1))
    title = corels_test.gen_filename_roc(args, parser, include_val=False, include_r=False)
    title = title.replace("_roc", "_cross-{}".format(args.num_groups)).replace(".csv", "")
    plt.title(title)
    plt.legend(boxplot['boxes'], args.reg)

def main():
    parser = get_parser()
    args = parser.parse_args()
    auc_matrix, len_matrix = run(args, parser)
    boxplot(auc_matrix, len_matrix, args, parser)
    errorbar(auc_matrix, len_matrix, args.reg)
    plt.show()

if __name__ == "__main__":
    main()