#!/usr/bin/python

import os, sys, re, random


### Constants:
fields = [ "family", "model", "stepping", "extfamily", "extmodel", "cores",
           "logical", "l1d-cache", "l1i-cache", "l2-cache", "l3-cache",
	   "l1d-assoc", "l2-assoc", "l3-assoc", "l1d-cacheline",
	   "l2-cacheline", "l3-cacheline", "sse-size", "codename", "flags" ]

args = sys.argv

if len(args) != 3:
	print "Usage: run_tests <tests stash file> <cpiud_tool binary>"
	sys.exit(1)

f = open(args[1], "rt")
lines = f.readlines()
f.close()

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

def do_test(inp, expected_out, testno, binary):
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
		return "Mismatch in fields:\n%s" % "\n".join([fmt_error(err) for err in err_fields]) 

current_input = []
current_output = []
build_output = False
errors = False
test_no = 1
print "Testing..."
for line in lines:
	if line[:3] == "---":
		if build_output:
			codename = current_output[len(current_output) - 2]
			result = do_test(current_input, current_output, test_no, args[2])
			print "Test %d [%s]: %s" % (test_no, codename, result)
			if result != "OK":
				errors = True
			build_output = False
			test_no += 1
			current_input = []
			current_output = []
		else:
			build_output = True
	else:
		if build_output:
			current_output.append(line.strip())
		else:
			current_input.append(line.strip())

if not errors:
	print "All successfull!"
