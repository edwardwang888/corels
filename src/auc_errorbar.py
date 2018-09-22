import aucplot
import argparse
import csv
import matplotlib.pyplot as plt

def main():
    parser = argparse.ArgumentParser(description="Run multiple AUC errorbar plots")
    parser.add_argument("file", help="input file containing command line arguments (for usage, see aucplot.py)", action="store")
    parser.add_argument("--title", help="plot title", action="store", dest="title")
    parser.add_argument("-e", help="plot another a() objective other than AUC", action="store", type=int, dest="e")
    args = parser.parse_args()

    aucparser = aucplot.get_parser()
    colors = ['b','g','r','m','y']
    fmt = ['o','d','^','p','s']
    with open(args.file, 'rb') as csvfile:
        csvreader = csv.reader(csvfile, delimiter=' ')
        i = 0
        for row in csvreader:
            print(row)
            aucargs = aucparser.parse_args(row)
            if args.e != None:
                aucargs.e = args.e

            print(aucargs)
            auc_matrix, len_matrix = aucplot.run(aucargs, aucparser)

            if aucargs.method == "corels":
                reg = aucargs.reg
            else:
                reg = [int(round(1/r)) for r in aucargs.reg]

            aucplot.errorbar(auc_matrix, len_matrix, ["{} ({})".format(aucargs.title, x) for x in reg], fmt=fmt[i], color=colors[i])
            i += 1

    plt.xlabel("Model Size")
    if args.e == None:
        plt.ylabel("AUC")
    else:
        plt.ylabel("WPA (p={})".format(args.e))

    if args.title != None:
        plt.title(args.title)

    plt.show()

if __name__ == "__main__":
    main()