#!/usr/bin/python

import os, sys, re, glob

if len(sys.argv) != 2:
	print "Usage: check-consistency <path>"

rexp = re.compile('.*{ CPU_FEATURE_([^,]+), "([^"]+)".*}.*')

for fn in glob.glob("%s/*.c" % sys.argv[1]):
	f = open(fn, "rt")
	line = 1
	for s in f.readlines():
		if rexp.match(s):
			res = rexp.findall(s)
			if len(res) > 1:
				raise "Too many matches"
			if res[0][0].lower() != res[0][1]:
				print "Mismatch - %s:%d - `%s' vs `%s'" % (fn, line, res[0][0], res[0][1])
		line += 1
