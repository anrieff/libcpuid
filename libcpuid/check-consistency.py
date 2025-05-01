#!/usr/bin/env python3

import argparse, os, sys, re, glob
from pathlib import Path


### Constants:
git_root_dir = os.popen("git rev-parse --show-toplevel").read().splitlines()

err = 0
def getEnumElements(enumName):
	f = open(f"{args.root_dir}/libcpuid.h", "r")
	l = []
	on = False
	rexp = re.compile(r'^\s*([A-Z0-9_]+)(\s*=\s*[A-Z0-9_]+)?\s*,.*$')
	for line in f:
		line = line.strip()
		if line.startswith("typedef enum {"):
			l = []
			on = True
		if on and rexp.match(line):
			l.append(rexp.findall(line)[0][0])
		if on and line.startswith("} "):
			on = False
			if line.find(enumName) != -1:
				return l
	f.close()
	return []

def getConstant(constantName):
	f = open(f"{args.root_dir}/libcpuid_constants.h", "r")
	value = 0
	for line in f:
		items = line.strip().split()
		if len(items) >= 3 and items[0] == "#define" and items[1] == constantName:
			value = int(items[2])
	f.close()
	return value

def checkEnumSize(enumName, constantName):
	print(f"Checking enum `{enumName}':", end=' ')
	count = len(getEnumElements(enumName)) - 1
	themax = getConstant(constantName)
	print(f"{count} elements; max size ({constantName}={themax})...", end=' ')
	if count > themax:
		err += 1
		print("FAILED")
		global firstError
		firstError = False
	else:
		print("OK")

def check_type_directory(directory):
	if not Path(directory).is_dir():
		raise argparse.ArgumentTypeError(f"{directory} is not a directory")
	return directory

# Parse arguments
parser = argparse.ArgumentParser(description="Check library consistency.")
parser.add_argument("root_dir",
	nargs='?',
	type=check_type_directory,
	default=f"{git_root_dir[-1]}/libcpuid" if len(git_root_dir) > 0 else "",
	help="path to the libcpuid sub-directory")
args = parser.parse_args()

checkEnumSize("cpu_feature_t", "CPU_FLAGS_MAX")
checkEnumSize("cpu_hint_t", "CPU_HINTS_MAX")
checkEnumSize("cpu_sgx_feature_t", "SGX_FLAGS_MAX")

rexp = re.compile('.*{ CPU_FEATURE_([^,]+), "([^"]+)".*}.*')
print("Finding features:")
for fn in glob.glob(f"{args.root_dir}/*.c"):
	f = open(fn, "rt")
	line = 1
	nfeat = 0
	for s in f.readlines():
		if rexp.match(s) and "FEATURE_LEVEL_" not in s:
			nfeat += 1
			res = rexp.findall(s)
			assert len(res) == 1, "Too many matches"
			if res[0][0].lower() != res[0][1]:
				err += 1
				print(f"..Mismatch - {os.path.basename(fn)}:{line} - `{res[0][0]}' vs `{res[0][1]}'")
		line += 1
	if nfeat:
		print(f"  {os.path.basename(fn)}: {nfeat} features described")
	f.close()

# Check whether all features are converted by cpu_feature_str():

allf = []
f = open(f"{args.root_dir}/libcpuid.h", "rt")
rexp = re.compile('\t(CPU_FEATURE_[^, ]+).*')
for s in f.readlines():
	if rexp.match(s):
		allf.append(rexp.findall(s)[0])

f.close()

impf = []
rexp = re.compile('\t+{ (CPU_FEATURE_[^,]+).*')
f = open(f"{args.root_dir}/cpuid_main.c", "rt")
for s in f.readlines():
	if rexp.match(s):
		entry = rexp.findall(s)[0]
		if entry in impf:
			err += 1
			print(f"cpu_feature_str(): duplicate entry: {entry}")
		impf.append(entry)
f.close()

print(f"Found {len(allf)} total features and {len(impf)} named features")

for feature in allf:
	if not feature in impf:
		err += 1
		print(f"cpu_feature_str(): don't have entry for {feature}")

# Check whether all features have detection code:
print("Checking whether all features have detection code...", end=' ')
firstError = True

files_code = {}

rexp1 = re.compile(r'.*\[(CPU_FEATURE_[^ \]]+)\]\s*=\s*{.*') # e.g. "[CPU_FEATURE_SWAP] = {"
rexp2 = re.compile(r'.*(CPU_FEATURE_[^ ,]+),\s*FEATURE_LEVEL_ARM_.*') # e.g. "set_feature_status(data, ext_status, (mte_frac == 0b0000), CPU_FEATURE_MTE_ASYNC,           FEATURE_LEVEL_ARM_V8_5_A, -1);"
rexp3 = re.compile(r'.*(CPU_FEATURE_[^ }]+).*') # e.g. "{ 28, CPU_FEATURE_HT },"

for fn in glob.glob(f"{args.root_dir}/*.c"):
	f = open(fn, "rt")
	files_code[fn] = []
	for s in f.readlines():
		if rexp1.match(s):
			entry = rexp1.findall(s)[0]
			files_code[fn].append(entry)
		elif rexp2.match(s):
			entry = rexp2.findall(s)[0]
			files_code[fn].append(entry)
		elif rexp3.match(s):
			entry = rexp3.findall(s)[0]
			files_code[fn].append(entry)

	f.close()

features_whitelist = [
	"CPU_FEATURE_SSE5", # deprecated, never defined
	"CPU_FEATURE_AES", # defined twice (x86 + ARM)
]
for feature in allf:
	matching_files = []
	for fn in files_code:
		if feature in files_code[fn]:
			matching_files.append(fn)
	if len(matching_files) == 0 and feature not in features_whitelist:
		if firstError:
			print("FAILED:")
			firstError = False
		err += 1
		print(f"..No detection code for {feature}")
	if len(matching_files) > 1 and feature not in features_whitelist:
		if firstError:
			print("FAILED:")
			firstError = False
		err += 1
		print(f"..Conflicting detection code for {feature} in files {' and '.join(matching_files)}")

if firstError:
	print("All OK.")
print("")

print("Checking processor definitions:")
cache_exp = re.compile(r".*([\(/ ][0-9]+K).*")
# Check whether CPU codenames for consistency:
#   - Codenames should not exceed 31 characters
#   - Check for common typos
definitions = 0
match_entry_fields = 12 # this number needs to change if the definition of match_entry_t ever changes
codename_str_max = 64-1 # this number needs to change if the value of CODENAME_STR_MAX ever changes
technology_str_max = 16-1 # this number needs to change if the value of TECHNOLOGY_STR_MAX ever changes
common_cache_sizes = ["8", "16", "32", "64", "128", "256", "512", "1024", "2048", "3072", "4096", "6144", "8192", "12288", "16384"]
for fn in glob.glob(f"{args.root_dir}/*.c"):
	bfn = os.path.basename(fn)
	nline = 0
	f = open(fn, "rt")
	has_matchtable = False
	cdefs = 0
	allok = True
	for line in f.readlines():
		nline += 1
		if line.find("struct match_entry_t") != -1:
			has_matchtable = True
		if not has_matchtable:
			continue
		i = line.find("{")
		j = line.rfind("}")
		if i == -1 or j == -1 or i > j:
			continue
		inner = line[i+1:j]
		parts = inner.split(",")
		if len(parts) == match_entry_fields:
			cdefs += 1
			# Check CPU codename string
			codename = parts[10].strip()
			if codename[0] != '"' or codename[-1] != '"':
				err += 1
				print(f"..Warning, {bfn}:{nline} - cannot correctly handle the cpu codename ({codename})")
				allok = False
				continue
			codename = codename[1:-1]
			if len(codename) > codename_str_max:
				err += 1
				print(f"..{bfn}:{nline} - codename ({codename}) is longer than {codename_str_max} characters!")
				allok = False
			if cache_exp.match(codename):
				cache_size = cache_exp.findall(codename)[0][1:-1]
				if not cache_size in common_cache_sizes:
					err += 1
					print(f"..Warning, {bfn}:{nline} - suspicious cache size in codename [{codename}] ({cache_size})")
					allok = False
			# Check CPU technology string
			technology = parts[11].strip()
			if (technology[0] != '"' or technology[-1] != '"') and technology != "UNKN_STR":
				err += 1
				print(f"..Warning, {bfn}:{nline} - cannot correctly handle the cpu technology ({technology})")
				allok = False
				continue
			technology = technology[1:-1]
			if len(technology) > technology_str_max:
				err += 1
				print(f"..{bfn}:{nline} - technology ({technology}) is longer than {technology_str_max} characters!")
				allok = False
	if cdefs:
		definitions += 1
		print(f"  {bfn}: {cdefs} processor definitions,", end=' ')
		if allok:
			print("all OK")
		else:
			print("some errors/warnings")
	f.close()
if definitions == 0:
	err += 1
	print("..Warning, no processor definitions found")

sys.exit(err)
