import aucplot
import argparse
import csv
import matplotlib.pyplot as plt

def main():
    parser = argparse.ArgumentParser(description="Run multiple AUC errorbar plots")
    parser.add_argument("file", help="input file containing command line arguments (for usage, see aucplot.py)", action="store")
    parser.add_argument("--title", help="plot title", action="store", dest="title")
    args = parser.parse_args()

    aucparser = aucplot.get_parser()
    colors = ['b','g','r','o','y']
    fmt = ['o','d','^','p','s']
    with open(args.file, 'rb') as csvfile:
        csvreader = csv.reader(csvfile, delimiter=' ')
        i = 0
        for row in csvreader:
            print(row)
            aucargs = aucparser.parse_args(row)
            print(aucargs)
            auc_matrix, len_matrix = aucplot.run(aucargs, aucparser)
            aucplot.errorbar(auc_matrix, len_matrix, ["{} ({})".format(aucargs.title, x) for x in aucargs.reg], fmt=fmt[i], color=colors[i])
            i += 1

    plt.xlabel("Model Size")
    plt.ylabel("AUC")
    if args.title != None:
        plt.title(args.title)

    plt.show()

if __name__ == "__main__":
    main()