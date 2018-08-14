#!/usr/bin/python

# version 6.0

import os
from os import system
import fileinput
import shutil
import sys
import argparse

# PARAMETERS TO CHANGE:
# PERIOD 60 means 60 packets per hour
# PERIOD 6 means 600 packets per hour

def ParseCommandLine():
    parser = argparse.ArgumentParser(description="Changing RPL parameters dynamically")

    # Altering freaquency of messages
    parser.add_argument('-mp', '--mobilePeriod', help='Period for sending UDP packets from the MOBILE stations only',
                        required=True)
    parser.add_argument('-fp', '--fixedPeriod', help='Period for sending UDP packets from the FIXED stations only',
                        required=True)

    # Altering Idouble
    parser.add_argument('-mi', '--mobileIdouble', help='Idouble for mobiles. USUALLY, we dont change this',
                        required=False)
    parser.add_argument('-fi', '--fixedIdouble', help='Idouble for statics & server.', required=False)

    args = parser.parse_args()

    print("Mobile Period: {}".format(args.mobilePeriod))
    print("Fixed Stations Period: {}".format(args.fixedPeriod))

    if args.mobileIdouble:
        print("Mobile Idouble: {}".format(args.mobileIdouble))
    if args.fixedIdouble:
        print("Fixed Stations Idouble: {}".format(args.fixedIdouble))

    return args


if __name__ == '__main__':
    args = ParseCommandLine()

    textForPeriod = "#define PERIOD "
    textForIdouble = "#define IDOUBLE "

#=================== delete all old compiled code. Force fresh compilation ===========
    try:
        files = os.listdir(".") #scan the current directory for all files
        for file in files:
            if file.endswith(".z1"):
                print("Deleting file: " + str(file))
                os.remove(file)
        try:
           os.remove("contiki-z1.map")
           print("removed contiki-z1.map")
        except:
            print("contiki-z1.map NOT existed")

        try:
           os.remove("symbols.c")
           print("removed symbols.c")
           os.remove("symbols.h")
           print("removed symbols.h")
        except:
            print("One of symbols.* NOT existed")

        try: 
           os.remove("contiki-z1.a")
           print("removed contiki-z1.a")
        except:
            print("contiki-z1.a NOT existed")
        try:
           shutil.rmtree("./obj_z1")
        except:
           print("./obj_z1 NOT existed")
    except:
        print("\nNo file(s) found to delete")

    print("\n-------- Altering *.c files--------------------\n")

#======================= OPENING FILES TO ALTER PARAMETERS ============================
    files = os.listdir(".")  # scan the current directory AGAIN for all files
    for file in files:
        if not os.path.isdir(file):  # exclude directories

            if file.endswith("-mobile-A.c") or file.endswith("-mobile-De.c") or file.endswith("-mobile-Dy.c"):
                str(file)
                print("Opening file: "+str(file))
                newPeriodText = textForPeriod + str(args.mobilePeriod)
                if args.mobileIdouble:
                    newIdoubleText = textForIdouble + str(args.mobileIdouble)

                tempFile = open(file, 'r+')
                for line in fileinput.input(file):
                    if textForPeriod in line:
                        print("added" + newPeriodText)
                        line = newPeriodText + "\n"
                    tempFile.write(line)
                tempFile.close()

                #open the file again
                tempFile = open(file, 'r+')
                for line in fileinput.input(file):
                    if args.mobileIdouble:
                        if textForIdouble in line:
                            print("added " + newIdoubleText)
                            line = newIdoubleText + "\n"
                        tempFile.write(line)
                tempFile.close()

            else:
                if file.endswith("-static-A.c") or file.endswith("-static-De.c") or file.endswith("-static-Dy.c") or file.endswith("-server-A.c") or file.endswith("udp-server-De.c") or file.endswith("udp-server-Dy.c"):
                    print("Opening file: "+str(file))
                    newPeriodText = textForPeriod + str(args.fixedPeriod)
                    if args.fixedIdouble:
                        newIdoubleText = textForIdouble + str(args.fixedIdouble)

                    tempFile = open(file, 'r+')
                    for line in fileinput.input(file):
                        if textForPeriod in line:
                            print("added: " + newPeriodText)
                            line = newPeriodText + "\n"
                        tempFile.write(line)
                    tempFile.close()

                    tempFile = open(file, 'r+')
                    for line in fileinput.input(file):
                        if args.fixedIdouble:
                            if textForIdouble in line:
                                print("added " + newIdoubleText)
                                line = newIdoubleText + "\n"
                            tempFile.write(line)
                    tempFile.close()

                else:
                    continue
