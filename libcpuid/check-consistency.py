#!/usr/bin/python

import os, sys, re, glob

if len(sys.argv) != 2:
	print "Usage: check-consistency <path>"
	sys.exit(0)

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
	f.close()

# Check whether all features are converted by cpu_feature_str():

allf = []
f = open("%s/libcpuid.h" % sys.argv[1], "rt")
rexp = re.compile('\t(CPU_FEATURE_[^, ]+).*')
for s in f.readlines():
	if rexp.match(s):
		allf.append(rexp.findall(s)[0])

f.close()

impf = []
rexp = re.compile('\t+{ (CPU_FEATURE_[^,]+).*')
f = open("%s/cpuid_main.c" % sys.argv[1], "rt")
for s in f.readlines():
	if rexp.match(s):
		entry = rexp.findall(s)[0]
		if entry in impf:
			print "cpu_feature_str(): duplicate entry: %s" % entry
		impf.append(entry)
f.close()

for feature in allf:
	if not feature in impf:
		print "cpu_feature_str(): don't have entry for %s" % feature

# Check whether all features have detection code:

files_code = {}

rexp = re.compile('.*{ *[0-9]+, (CPU_FEATURE_[^ }]+).*')

for fn in glob.glob("%s/*.c" % sys.argv[1]):
	f = open(fn, "rt")
	files_code[fn] = []
	for s in f.readlines():
		if rexp.match(s):
			entry = rexp.findall(s)[0]
			files_code[fn].append(entry)

for feature in allf:
	matching_files = []
	for fn in files_code:
		if feature in files_code[fn]:
			matching_files.append(fn)
	if len(matching_files) == 0:
		print "No detection code for %s" % feature
	if len(matching_files) > 1:
		print "Conflicting detection code for %s in files %s" % (feature, " and ".join(matching_files))
