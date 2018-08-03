import argparse
import os
import sys

def run(r, outfile, dataset, falling, wpa, max_num_nodes, b, c, p, override_obj):
    data_minor = "../data/{}.minor".format(dataset)
    #Start assembling command string
    cmd = "./corels -r {} ".format(r)
    if c != None:
        cmd += "-c {} ".format(c)
    if b:
        cmd += "-b "

    cmd += "-p {} ".format(p)

    if wpa or outfile == "wpa.csv":
        cmd = cmd + '-w '
    if override_obj:
        cmd += '-W '
    if falling or outfile == "falling.csv":
        cmd = cmd + "-d "
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
    parser.add_argument("-d", help="use falling constraint", action="store_true", dest="falling")
    parser.add_argument("-w", help="use WPA objective function", action="store_true", dest="wpa")
    parser.add_argument("-n", help="maximum number of nodes (default 100000)", type=int, action="store", dest="max_num_nodes")
    parser.add_argument("-b", help="breadth first search", action="store_true", dest="b")
    parser.add_argument("-c", help="best first search policy", type=int, choices=[1,2,3,4], action="store", dest="c")
    parser.add_argument("-p", help="symmetry aware map", type=int, choices=[0,1,2], action="store", default=1, dest="p")
    parser.add_argument("--compare", help="compare runs by varying this parameter", action="store", choices=["falling", "wpa"], dest="compare")
    parser.add_argument("--plot", help="generate plots", action="store_true", dest="plot")
    parser.add_argument("-W", help="override objective", action="store_true", dest="W")
    args = parser.parse_args()
    
    #Default value
    if args.b == False and args.c == None:
        args.c = 2

    if args.compare != None:
        outfile = args.compare + ".csv"
    
    for i in ["default.csv", "falling.csv", "wpa.csv"]:
        if os.access(i, os.F_OK):
            os.unlink(i);
    """
    if os.access("default.csv", os.F_OK):
        os.unlink("default.csv")
    if args.compare != None and os.access(outfile, os.F_OK):
        os.unlink(outfile)
    """  
    override_obj = (args.compare == "wpa") or args.W
    r = 0.7515
    for i in range(200):
        run(r, "default.csv", args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, override_obj)
        if args.compare != None:
            run(r, outfile, args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, False)
            
        r /= 1.0525
        
    #Run with zero regularity
    run(0, "default.csv", args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, override_obj)
    if args.compare != None:
        run(0, outfile, args.dataset, args.falling, args.wpa, args.max_num_nodes, args.b, args.c, args.p, False)

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
