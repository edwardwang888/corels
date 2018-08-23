import os
import sys

def check_outfile(outfile):
    if os.access(outfile, os.F_OK):
        c = raw_input("File {} already exists. Are you sure you want to continue? (y/N/a) ".format(outfile))
        if c.lower() == "y":
            os.unlink(outfile)
        elif c.lower() == "a":
            pass
        else:
            sys.exit()

        return c.lower()

def check_outfile_roc(outfile_roc):
    if os.access(outfile_roc, os.F_OK):
        c = raw_input("File {} already exists. Do you want to run again? (y/N/a) ".format(outfile_roc))
        if c.lower() == "y":
            os.unlink(outfile_roc)
            return "y"
        elif c.lower() == "a":
            return "a"
        else:
            return "n"