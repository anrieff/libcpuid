#!/usr/bin/python

import os, sys, re

args = sys.argv

if not len(args) in (2, 3):
	print "Usage: extract_test.py <test num> [tests stash file]"
	print "If the last is not specified, `tests_stash.txt' is assumed"
	sys.exit(1)

tnum = int(args[1])

stash = "tests_stash.txt"
if len(args) == 3:
	stash = args[2]
f = open(stash, "rt")
data = f.readlines()
f.close()
delimiter = "-" * 80
i = 1
lnum = 0
while i < tnum:
	while lnum < len(data) and data[lnum][:3] != '---':
		lnum += 1
	lnum += 1
	while lnum < len(data) and data[lnum][:3] != '---':
		lnum += 1
	lnum += 1
	i += 1
if lnum >= len(data):
	print "Test index out of range"
	sys.exit(1)

xd = []
while data[lnum][:3] != '---':
	xd.append(data[lnum])
	lnum += 1
f = open("raw.txt", "wt")
f.writelines(xd)
f.close()
print "Done -- it's in `raw.txt'"
