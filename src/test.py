import argparse
import os
import sys

def run(r, outfile, dataset, falling, wpa, max_num_nodes, b, c, p, override_obj):
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
    # if override_obj:
    #     cmd += '-W '
    if max_num_nodes != None:
        cmd += "-n {} ".format(max_num_nodes)

    cmd += "-o {} ".format(outfile)
        
    if os.access(data_minor, os.F_OK) == False:
        cmd = cmd + "../data/{0}.out ../data/{0}.label".format(dataset)
    else:
        cmd = cmd + "../data/{0}.out ../data/{0}.label ../data/{0}.minor".format(dataset)
        
    print("\n" + cmd)
    os.system(cmd)
    
def main():
    parser = argparse.ArgumentParser(description="Run test cases on corels")
    parser.add_argument("dataset", help="dataset to use (in ../data/)")
    parser.add_argument("-r", help="starting regularization", action="store", type=float, default=0.7515, dest="r_start")
    parser.add_argument("-b", help="breadth first search", action="store_true", dest="b")
    parser.add_argument("-c", help="best first search policy", type=int, choices=[1,2,3,4], action="store", dest="c")
    parser.add_argument("-p", help="symmetry aware map", type=int, choices=[0,1,2], action="store", dest="p")
    #parser.add_argument("--compare", help="compare runs by varying this parameter", action="store", choices=["falling", "wpa"], dest="compare")
    parser.add_argument("-d", help="use falling constraint", action="store_true", dest="falling")
    parser.add_argument("-w", help="use WPA objective function", action="store_true", dest="wpa")
    parser.add_argument("-W", help="add text to file name", action="store", dest="text")
    parser.add_argument("-s", help="regularization step", action="store", type=float, dest="step")
    parser.add_argument("-n", help="maximum number of nodes (default 100000)", type=int, action="store", dest="max_num_nodes")
    parser.add_argument("--plot", help="generate plots", action="store_true", dest="plot")
    args = parser.parse_args()
    
    # Default value
    # if args.b == False and args.c == None:
    #     args.c = 2

    # if args.compare != None:
    #     outfile = args.compare + ".csv"
    
    # for i in ["default.csv", "falling.csv", "wpa.csv"]:
    #     if os.access(i, os.F_OK):
    #         os.unlink(i);
    """
    if os.access("default.csv", os.F_OK):
        os.unlink("default.csv")
    if args.compare != None and os.access(outfile, os.F_OK):
        os.unlink(outfile)
    """
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
    if args.r_start != None:
        outfile += "_r{}".format(args.r_start)
    if args.step != None:
        outfile += "_s{}".format(args.step)
    if args.max_num_nodes != None:
        outfile += "_n{}".format(args.max_num_nodes)
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
    
    override_obj = True
    r = args.r_start
    if args.step != None and args.r_start != None:
        iter = int(args.r_start/args.step)
    else:
        iter = 200
    for i in range(iter):
        run(r, outfile, args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, override_obj)
        # if args.compare != None:
        #     run(r, outfile, args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, False)
            
        if args.step == None:
            r /= 1.0525
        else:
            r -= args.step
        
    while r > 0.0000001:
        run(r, outfile, args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, override_obj)
        r /= 1.0525

    #Run with zero regularity
    run(0, outfile, args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, override_obj)
    
    # if args.compare != None:
    #     run(0, outfile, args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, False)

    """     
    os.system("cat default.csv | uniq > default1.csv")
    os.rename("default1.csv", "default.csv")  
    if args.falling:
        os.system("cat falling.csv | uniq > falling1.csv")
        os.rename("falling1.csv", "falling.csv")
    """
    
    if args.plot:
        os.system("python plot.py")

    

if __name__ == "__main__":
    main()
