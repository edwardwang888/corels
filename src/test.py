import argparse
import os

def run(r, outfile, dataset):
    data_minor = "../data/{}.minor".format(dataset)
    #Start assembling command string
    cmd = "./corels -r {} -c 2 -p 1 -o {} ".format(r, outfile)
    if outfile == "falling.csv":
        cmd = cmd + "-d "
        
    if os.access(data_minor, os.F_OK) == False:
        cmd = cmd + "../data/{0}.out ../data/{0}.label".format(dataset)
    else:
        cmd = cmd + "../data/{0}.out ../data/{0}.label ../data/{0}.minor".format(dataset)
        
    print(cmd)
    os.system(cmd)
    
def main():
    parser = argparse.ArgumentParser(description="Run test cases on corels")
    parser.add_argument("dataset", help="dataset to use (in ../data/)")
    parser.add_argument("-z", help="don't run falling constraint", action="store_false", dest="falling")
    args = parser.parse_args()
    
    r = 0.15
    os.unlink("default.csv")
    if args.falling:
        os.unlink("falling.csv")
        
    for i in range(70):
        run(r, "default.csv", args.dataset)
        if args.falling:
            run(r, "falling.csv", args.dataset)
            
        r /= 1.25
        
    #Run with zero regularity
    run(0, "default.csv", args.dataset)
    if (args.falling):
        run(0, "falling.csv", args.dataset)
        
    os.system("cat default.csv | uniq > default1.csv")
    os.rename("default1.csv", "default.csv")  
    if args.falling:
        os.system("cat falling.csv | uniq > falling1.csv")
        os.rename("falling1.csv", "falling.csv")
    
if __name__ == "__main__":
    main()