#!/usr/bin/env python3

import argparse, sys, re


### Constants:
delimiter = "-" * 80

def readRawFile():
	rawdata = []
	for line in args.raw_file.readlines():
		lookfor = [
			"Logical CPU", "CPUID", "CPU#", # common
			"basic_cpuid", "ext_cpuid", "intel_fn4", "intel_fn11", "amd_fn8000001dh", # x86
			"arm_midr", "arm_mpidr", "arm_revidr", # ARM common
			"arm_id_afr", "arm_id_dfr", "arm_id_isar", "arm_id_mmfr", "arm_id_pfr", # ARM (AArch32)
			"arm_id_aa64afr", "arm_id_aa64dfr", "arm_id_aa64fpfr", "arm_id_aa64isar", "arm_id_aa64mmfr", "arm_id_aa64pfr", "arm_id_aa64smfr", "arm_id_aa64zfr" # ARM (AArch64)
		]
		ignore  = ["MSR Register"]
		good = False
		for match in lookfor:
			if line.find(match) != -1:
				good = True
				break
		for match in ignore:
			if line.find(match) != -1:
				good = False
				break
		if good:
			rawdata.append(line.strip())
	return rawdata

def readResultFile():
	repdata = []
	rexp = re.compile('(-?[0-9]+).*')
	for line in args.report_file.readlines():
		s = line.strip()
		if s.find(":") == -1:
			continue
		numeric = ["family", "model", "stepping", "ext_family", "ext_model",
				"implementer", "variant", "part_num", "revision",
				"num_cores", "num_logical",
				"L1 D cache",  "L1 I cache",  "L2 cache",   "L3 cache",   "L4 cache",
				"L1D assoc.",  "L1I assoc.",  "L2 assoc.",  "L3 assoc.",  "L4 assoc.",
				"L1D line sz", "L1I line sz", "L2 line sz", "L3 line sz", "L4 line sz",
				"L1D inst.",   "L1I inst.",   "L2 inst.",   "L3 inst.",   "L4 inst.",
				]
		field = s[:s.find(":")].strip()
		if field in numeric:
			value = s[s.find(":")+1:].strip()
			if not rexp.match(value):
				raise f"Bad format of value: [{s}]"
			repdata.append(rexp.findall(value)[0])
		if "CPU Info for type" in field:
			repdata.append(delimiter)
		if field == "arch":
			value = s[s.find(":") + 2:]
			repdata.append(value)
		if field == "feat_level":
			value = s[s.find(":") + 2:]
			repdata.append(value)
		if field == "purpose":
			value = s[s.find(":") + 2:]
			repdata.append(value)
		if field == "code name":
			value = s[s.find("`") + 1: s.find("'")]
			repdata.append(value)
		if field == "technology":
			value = s[s.find("`") + 1: s.find("'")]
			repdata.append(value)
		if field == "features":
			value = s[s.find(":") + 2:]
			repdata.append(value)
		if field == "SSE units":
			value = s[s.find(":") + 2:]
			# the value here is something like "XX bits (authoritative)". We remove the "bits" part:
			i = value.find("bits")
			if i != -1:
				value = value[:i] + value[i + 5:]
			repdata.append(value)
	return repdata

# Parse arguments
parser = argparse.ArgumentParser(description="Create a new test file by using cpuid_tool raw and report files.")
parser.add_argument("raw_file",
	nargs='?',
	type=argparse.FileType('r'),
	default="raw.txt",
	help="an existing raw data file")
parser.add_argument("report_file",
	nargs='?',
	type=argparse.FileType('r'),
	default="report.txt",
	help="an existing report file")
parser.add_argument("test_file",
	nargs='?',
	type=argparse.FileType('w'),
	default=sys.stdout,
	help="test file to create (default is standard output)")
args = parser.parse_args()

# Create test file
lines = readRawFile() + readResultFile()
args.test_file.writelines([s + "\n" for s in lines])
