#!/usr/bin/python

from __future__ import with_statement
import os, sys, re, random


### Constants:
fields = [ "family", "model", "stepping", "extfamily", "extmodel", "cores",
           "logical", "l1d-cache", "l1i-cache", "l2-cache", "l3-cache",
	   "l1d-assoc", "l2-assoc", "l3-assoc", "l1d-cacheline",
	   "l2-cacheline", "l3-cacheline", "sse-size", "codename", "flags" ]

args = sys.argv
fix = False

if len(args) < 3:
	print """
Usage: run_tests.py <cpuid_tool binary> <test file/dir> [test file/dir ...] [OPTIONS]

If a test file is given, it is tested by itself.
If a directory is given, process all *.test files there, subdirectories included.

If the --fix option is given, the behaviour of the cpuid_tool binary is deemed correct
and any failing tests are updated.
"""
	sys.exit(1)

filelist = []
cpuid_tool = args[1]
for arg in args[2:]:
	if arg == "--fix":
		fix = True
		continue
	if os.path.isdir(arg):
		# gather all *.test files from subdirs amd and intel:
		for dirpath, dirnames, filenames in os.walk(arg):
			filelist += [os.path.join(dirpath, fn) for fn in filenames if fn[-5:] == ".test"]
	else:
		filelist.append(arg)

#f = open(args[1], "rt")
#lines = f.readlines()
#f.close()

# One would usually use os.tempnam, but libc gives off hell a lot of
# warnings when you attempt to use that :(
def make_tempname(prefix):
	chars = ""
	for i in xrange(26):
		chars += chr(97+i)
		chars += chr(65+i)
	for i in xrange(10):
		chars += chr(48+i)
	for i in xrange(6):
		prefix += random.choice(chars)
	return prefix

def fmt_error(err):
	pfix = "  %s: " % err[0]
	return "%sexpected `%s'\n%sgot      `%s'" % (pfix, err[1], " "*len(pfix), err[2])

def fixFile(filename, input_lines, output_lines):
	f = open(filename, "wt")
	f.writelines(map(lambda s: s + "\n", input_lines))
	f.write("--------------------------------------------------------------------------------\n")
	f.writelines(map(lambda s: s + "\n", output_lines))
	f.close()

def do_test(inp, expected_out, binary, test_file_name):
	fninp = make_tempname("cpuidin")
	fnoutp = make_tempname("cpuidout")
	f = open(fninp, "wt")
	f.writelines(map(lambda s: s + "\n", inp))
	f.close()
	cmd = "%s --load=%s --outfile=%s %s" % (binary, fninp, fnoutp, " ".join(map(lambda s: "--" + s, fields)))
	os.system(cmd)
	os.unlink(fninp)
	real_out = []
	try:
		f = open(fnoutp, "rt")
		real_out = map(lambda s: s.strip(), f.readlines())
		f.close()
		os.unlink(fnoutp)
	except IOError:
		return "Exception"
	if len(real_out) != len(expected_out) or len(real_out) != len(fields):
		return "Unexpected number of records returned"
	err_fields = []
	for i in range(len(real_out)):
		if real_out[i] != expected_out[i]:
			err_fields.append((fields[i], expected_out[i], real_out[i]))
	if not err_fields:
		return "OK"
	else:
		if fix:
			fixFile(test_file_name, inp, real_out)
			return "Mismatch, fixed."
		else:
			return "Mismatch in fields:\n%s" % "\n".join([fmt_error(err) for err in err_fields]) 

errors = False
print "Testing..."
for test_file_name in filelist:
	current_input = []
	current_output = []
	build_output = False
	with open(test_file_name, "rt") as f:
		for line in f.readlines():
			if line[:3] == "---":
				build_output = True
			else:
				if build_output:
					current_output.append(line.strip())
				else:
					current_input.append(line.strip())
		#codename = current_output[len(current_output) - 2]
		result = do_test(current_input, current_output, cpuid_tool, test_file_name)
		print "Test [%s]: %s" % (test_file_name[:-5], result)
		if result != "OK":
			errors = True
		build_output = False

if not errors:
	print "All successfull!"
