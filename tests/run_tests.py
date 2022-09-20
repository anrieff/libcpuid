#!/usr/bin/env python3


import os, sys, re, random


### Constants:
fields = [ "architecture", "purpose", "family", "model", "stepping", "extfamily",
	   "extmodel", "cores", "logical",
	   "l1d-cache",     "l1i-cache",     "l2-cache",     "l3-cache",     "l4-cache",
	   "l1d-assoc",     "l1i-assoc",     "l2-assoc",     "l3-assoc",     "l4-assoc",
	   "l1d-cacheline", "l1i-cacheline", "l2-cacheline", "l3-cacheline", "l4-cacheline",
	   "l1d-instances", "l1i-instances", "l2-instances", "l3-instances", "l4-instances",
	   "sse-size", "codename", "flags" ]

args = sys.argv
fix = False
show_test_fast_warning = False

if len(args) < 3:
	print("""
Usage: run_tests.py <cpuid_tool binary> <test file/dir> [test file/dir ...] [OPTIONS]

If a test file is given, it is tested by itself.
If a directory is given, process all *.test files there, subdirectories included.

If the --fix option is given, the behaviour of the cpuid_tool binary is deemed correct
and any failing tests are updated.
""")
	sys.exit(1)

filelist = []
cpuid_tool = args[1]
for arg in args[2:]:
	if arg == "--fix":
		fix = True
		continue
	if arg == "--show-test-fast-warning":
		show_test_fast_warning = True
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
	for i in range(26):
		chars += chr(97+i)
		chars += chr(65+i)
	for i in range(10):
		chars += chr(48+i)
	for i in range(6):
		prefix += random.choice(chars)
	return prefix

def fmt_error(err):
	pfix = "  %s: " % err[0]
	return "%sexpected `%s'\n%sgot      `%s'" % (pfix, err[1], " "*len(pfix), err[2])

def fixFile(filename, input_lines, output_lines):
	f = open(filename, "wt")
	f.writelines([s + "\n" for s in input_lines])
	f.write("--------------------------------------------------------------------------------\n")
	f.writelines([s + "\n" for s in output_lines])
	f.close()

def do_test(inp, expected_out, binary, test_file_name, num_cpu_type):
	fninp = make_tempname("cpuidin")
	fnoutp = make_tempname("cpuidout")
	f = open(fninp, "wt")
	f.writelines([s + "\n" for s in inp])
	f.close()
	cmd = "%s --load=%s --outfile=%s %s" % (binary, fninp, fnoutp, " ".join(["--" + s for s in fields]))
	os.system(cmd)
	os.unlink(fninp)
	real_out = []
	try:
		f = open(fnoutp, "rt")
		for s in f.readlines():
			if "-----" not in s:
				real_out.append(s.strip())
		f.close()
		os.unlink(fnoutp)
	except IOError:
		return "Exception"
	if len(real_out) != len(expected_out) or len(real_out) != len(fields) * num_cpu_type:
		return "Unexpected number of records returned"
	err_fields = []
	for i in range(len(real_out)):
		if real_out[i] != expected_out[i]:
			err_fields.append((fields[i % len(fields)], expected_out[i], real_out[i]))
	if not err_fields:
		return "OK"
	else:
		if fix:
			fixFile(test_file_name, inp, real_out)
			return "Mismatch, fixed."
		else:
			return "Mismatch in fields:\n%s" % "\n".join([fmt_error(err) for err in err_fields])

errors = False
delimiter = "-" * 80
print("Testing...")
for test_file_name in filelist:
	num_cpu_type = 0
	current_input = []
	current_output = []
	build_output = False
	with open(test_file_name, "rt") as f:
		for line in f.readlines():
			line = line.strip()
			if line == delimiter:
				build_output = True
				num_cpu_type += 1
			else:
				if build_output:
					current_output.append(line)
				else:
					current_input.append(line)
		#codename = current_output[len(current_output) - 2]
		result = do_test(current_input, current_output, cpuid_tool, test_file_name, num_cpu_type)
		print("Test [%s]: %s" % (test_file_name[:-5], result))
		if result != "OK":
			errors = True
		build_output = False

if errors:
	if show_test_fast_warning:
		print("""
You're running tests in fast mode; before taking any action on the errors
above, please confirm that the slow mode ('make test-old') also fails.""")
	sys.exit(1)
else:
	print("All successfull!")
	sys.exit(0)
