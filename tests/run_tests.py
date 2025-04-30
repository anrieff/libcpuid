#!/usr/bin/env python3


import argparse, textwrap, os, sys, re, random, lzma
from pathlib import Path, PurePath


### Constants:
os.environ["LIBCPUID_NO_WARN"] = "1"
delimiter = "-" * 80
fields_x86 = [ "architecture", "feature-level", "purpose", "family", "model", "stepping",
	   "extfamily", "extmodel", "cores", "logical",
	   "l1d-cache",     "l1i-cache",     "l2-cache",     "l3-cache",     "l4-cache",
	   "l1d-assoc",     "l1i-assoc",     "l2-assoc",     "l3-assoc",     "l4-assoc",
	   "l1d-cacheline", "l1i-cacheline", "l2-cacheline", "l3-cacheline", "l4-cacheline",
	   "l1d-instances", "l1i-instances", "l2-instances", "l3-instances", "l4-instances",
	   "sse-size", "codename", "flags" ]
fields_arm = [ "architecture", "feature-level", "purpose",
	   "implementer", "variant", "part-num", "revision",
	   "cores", "logical",
	   "codename", "flags" ]

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
	pfix = f"  {err[0]}: "
	return "{} expected `{}'\n{} got      `{}'".format(pfix, err[1], ' '*len(pfix), err[2])

def fixFile(filename, input_lines, output_lines):
	pfilename = Path(filename)
	if len(pfilename.suffixes) >= 2:
		# Compressed file
		if pfilename.suffixes[1] == ".xz":
			f = lzma.open(pfilename, "wt")
		else:
			print(f"Cannot fix {pfilename.name} because {''.join(pfilename.suffixes[1:])} is not supported")
			return
	else:
		# Plain text file
		f = open(pfilename, "wt")
	f.writelines([s + "\n" for s in input_lines])
	f.write(delimiter + "\n")
	f.writelines([s + "\n" for s in output_lines])
	f.close()

def do_test(inp, expected_out, binary, test_file_name, num_cpu_type):
	fninp = make_tempname("cpuidin")
	fnoutp = make_tempname("cpuidout")
	f = open(fninp, "wt")
	f.writelines([s + "\n" for s in inp])
	f.close()
	architecture = os.popen(f"{binary} --load={fninp} --architecture").read().splitlines()[-1]
	if architecture == "x86":
		fields = fields_x86
	elif architecture == "ARM":
		fields = fields_arm
	else:
		fields = []
	cmd = f"{binary} --load={fninp} --outfile={fnoutp} {' '.join(['--' + s for s in fields])}"
	os.system(cmd)
	os.unlink(fninp)
	real_out = []
	real_out_delim = [] # when fixing the file, delimiters are required
	try:
		f = open(fnoutp, "rt")
		for s in f.readlines():
			real_out_delim.append(s.strip())
			if delimiter not in s:
				real_out.append(s.strip())
		f.close()
		os.unlink(fnoutp)
	except IOError:
		return "Exception"
	if len(real_out) != len(expected_out) or len(real_out) != len(fields) * num_cpu_type:
		if args.fix:
			fixFile(test_file_name, inp, real_out_delim)
			return "Number of records, fixed."
		else:
			return "Unexpected number of records returned\n - expected length {}\n - real length {}\n - {} fields".format(len(expected_out), len(real_out), len(fields) * num_cpu_type)
	err_fields = []
	for i in range(len(real_out)):
		if real_out[i] != expected_out[i]:
			err_fields.append((fields[i % len(fields)], expected_out[i], real_out[i]))
	if not err_fields:
		return "OK"
	else:
		if args.fix:
			fixFile(test_file_name, inp, real_out_delim)
			return "Mismatch, fixed."
		else:
			return "Mismatch in fields:\n{}".format('\n'.join([fmt_error(err) for err in err_fields]))

def is_regular_file(filename):
	try:
		with open(filename, 'r') as fd:
			fd.read()
			return True
	except:
		return False

def check_type_binary_file(filename):
	if not Path(filename).is_file():
		raise argparse.ArgumentTypeError(f"{filename} is not a file")
	if is_regular_file(filename):
		raise argparse.ArgumentTypeError(f"{filename} is not a binary file")
	return filename

# Parse arguments
parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter,
	description=textwrap.dedent("""

Run test files.

If a test file is given, it is tested by itself.
If a directory is given, process all *.test files there, subdirectories included.

If the --fix option is given, the behaviour of the cpuid_tool binary is deemed correct and any failing tests are updated.
"""))
parser.add_argument("cpuid_tool",
	nargs='?',
	type=check_type_binary_file,
	default="./build/cpuid_tool/cpuid_tool",
	help="path to the cpuid_tool binary")
parser.add_argument("input_test_files",
	nargs='+',
	default=["./tests"],
	help="test file or directory containing test files")
parser.add_argument("--fix",
	action=argparse.BooleanOptionalAction,
	default=False,
	help="update failing tests (default is false)")
parser.add_argument("--show-test-fast-warning",
	dest="show_test_fast_warning",
	action=argparse.BooleanOptionalAction,
	default=False,
	help="show a warning on errors (default if false)")
args = parser.parse_args()

# Create test files list
filelist = []
for input_test_file in args.input_test_files:
	if Path(input_test_file).is_dir():
		# gather all *.test files from subdirs amd, intel and cie:
		for dirpath, dirnames, filenames in os.walk(input_test_file):
			filelist += [PurePath(dirpath).joinpath(fn) for fn in filenames if Path(fn).suffixes[0] == ".test"]
	else:
		filelist.append(input_test_file)

# Run tests
errors = False
print("Testing...")
for test_file_name_raw in filelist:
	test_file_name = Path(test_file_name_raw)
	num_cpu_type = 0
	current_input = []
	current_output = []
	build_output = False
	if len(test_file_name.suffixes) >= 2:
		# Compressed file
		if test_file_name.suffixes[1] == ".xz":
			f = lzma.open(test_file_name, "rt")
		else:
			print(f"Test [{test_file_name.name}]: skipped because {''.join(test_file_name.suffixes[1:])} is not supported")
			continue
	else:
		# Plain text file
		f = open(test_file_name, "rt")
	# Read file line by line
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
	f.close()
	#codename = current_output[len(current_output) - 2]
	result = do_test(current_input, current_output, args.cpuid_tool, test_file_name, num_cpu_type)
	print(f"Test [{test_file_name.name}]: {result}")
	if result != "OK":
		errors = True
	build_output = False

if errors:
	if args.show_test_fast_warning:
		print("""
You're running tests in fast mode; before taking any action on the errors
above, please confirm that the slow mode ('make test-old') also fails.""")
	sys.exit(1)
else:
	print("All successfull!")
	sys.exit(0)
