#!/usr/bin/env python
#
# Runner for valexprun tests
#  arg 1: /path/to/valexprun-executable
#  arg 2: /path/to/srcdir
#  arg 3: name of data file in data-valexp-in/
#EXE="$1"
#SRCDIR="$2"
#INFILE="$3"

#INDIR="$SRCDIR"/data-valexp-in/
#INPUT="$INDIR$INFILE"
#OUTPUT="$SRCDIR/data-valexp-out/$INFILE"

#test -x "$EXE" || { echo "Missing executable" ; exit 1 ; }
#test -f "$INPUT" || { echo "Missing input $INPUT" ; exit 1 ; }
#test -f "$OUTPUT" || { echo "Missing output $OUTPUT" ; exit 1 ; }


#"$EXE" `cat "$INPUT"` > out-"$INFILE"
#diff -u "$OUTPUT" out-"$INFILE"

import sys
import subprocess

#print(sys.argv)
exe = sys.argv[1]
srcdir = sys.argv[2]
infile = sys.argv[3]

indir = srcdir + "/data-valexp-in/"
input = indir + infile
output = srcdir + "/data-valexp-out/" + infile
#print(exe)
#print(srcdir)
#print(infile)
#print(output)
with open(input, 'r') as finput:
	data = finput.read().splitlines()
#print data
data.insert(0, exe)
#print data
outinfile = "out-" + infile
with open(outinfile, 'w') as foutput:
	subprocess.call(data, stdout=foutput)
diffcall = [ 'diff', output, outinfile ]
#print diffcall
rc = subprocess.call(diffcall)
sys.exit(rc)
