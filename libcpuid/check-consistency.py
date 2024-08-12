#!/usr/bin/env python3

import os, sys, re, glob

if len(sys.argv) != 2:
	print("Usage: check-consistency <path>")
	sys.exit(0)

err = 0
def getEnumElements(enumName):
	f = open("%s/libcpuid.h" % sys.argv[1], "r")
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
	f = open("%s/libcpuid_constants.h" % sys.argv[1], "r")
	value = 0
	for line in f:
		items = line.strip().split()
		if len(items) >= 3 and items[0] == "#define" and items[1] == constantName:
			value = int(items[2])
	f.close()
	return value

def checkEnumSize(enumName, constantName):
	print("Checking enum `%s':" % enumName, end=' ')
	count = len(getEnumElements(enumName)) - 1
	themax = getConstant(constantName)
	print("%d elements; max size (%s=%d)..." % (count, constantName, themax), end=' ')
	if count > themax:
		err += 1
		print("FAILED")
		global firstError
		firstError = False
	else:
		print("OK")

checkEnumSize("cpu_feature_t", "CPU_FLAGS_MAX")
checkEnumSize("cpu_hint_t", "CPU_HINTS_MAX")
checkEnumSize("cpu_sgx_feature_t", "SGX_FLAGS_MAX")

rexp = re.compile('.*{ CPU_FEATURE_([^,]+), "([^"]+)".*}.*')
print("Finding features:")
for fn in glob.glob("%s/*.c" % sys.argv[1]):
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
				print("..Mismatch - %s:%d - `%s' vs `%s'" % (os.path.basename(fn), line, res[0][0], res[0][1]))
		line += 1
	if nfeat:
		print("  %s: %d features described" % (os.path.basename(fn), nfeat))
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
			err += 1
			print("cpu_feature_str(): duplicate entry: %s" % entry)
		impf.append(entry)
f.close()

print("Found %d total features and %d named features" % (len(allf), len(impf)))

for feature in allf:
	if not feature in impf:
		err += 1
		print("cpu_feature_str(): don't have entry for %s" % feature)

# Check whether all features have detection code:
print("Checking whether all features have detection code...", end=' ')
firstError = True

files_code = {}

rexp1 = re.compile(r'.*\[(CPU_FEATURE_[^ \]]+)\]\s*=\s*{.*') # e.g. "[CPU_FEATURE_SWAP] = {"
rexp2 = re.compile(r'.*(CPU_FEATURE_[^ ,]+),\s*FEATURE_LEVEL_ARM_.*') # e.g. "set_feature_status(data, ext_status, (mte_frac == 0b0000), CPU_FEATURE_MTE_ASYNC,           FEATURE_LEVEL_ARM_V8_5_A, -1);"
rexp3 = re.compile(r'.*(CPU_FEATURE_[^ }]+).*') # e.g. "{ 28, CPU_FEATURE_HT },"

for fn in glob.glob("%s/*.c" % sys.argv[1]):
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
		print("..No detection code for %s" % feature)
	if len(matching_files) > 1 and feature not in features_whitelist:
		if firstError:
			print("FAILED:")
			firstError = False
		err += 1
		print("..Conflicting detection code for %s in files %s" % (feature, " and ".join(matching_files)))

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
common_cache_sizes = ["8", "16", "32", "64", "128", "256", "512", "1024", "2048", "3072", "4096", "6144", "8192", "12288", "16384"]
for fn in glob.glob("%s/*.c" % sys.argv[1]):
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
		j = line.find("}")
		if i == -1 or j == -1 or i > j:
			continue
		inner = line[i+1:j]
		parts = inner.split(",")
		if len(parts) == match_entry_fields:
			cdefs += 1
			s = parts[match_entry_fields-1].strip()
			if s[0] != '"' or s[-1] != '"':
				err += 1
				print("..Warning, %s:%d - cannot correctly handle the cpu codename" % (bfn, nline))
				allok = False
				continue
			s = s[1:-1]
			if len(s) > codename_str_max:
				err += 1
				print("..%s:%d - codename (%s) is longer than %d characters!" % (bfn, nline, s, codename_str_max))
				allok = False
			if cache_exp.match(s):
				cache_size = cache_exp.findall(s)[0][1:-1]
				if not cache_size in common_cache_sizes:
					err += 1
					print("..Warning, %s:%d - suspicious cache size in codename [%s] (%s)" % (bfn, nline, s, cache_size))
					allok = False
	if cdefs:
		definitions += 1
		print("  %s: %d processor definitions," % (bfn, cdefs), end=' ')
		if allok:
			print("all OK")
		else:
			print("some errors/warnings")
	f.close()
if definitions == 0:
	err += 1
	print("..Warning, no processor definitions found")

sys.exit(err)
