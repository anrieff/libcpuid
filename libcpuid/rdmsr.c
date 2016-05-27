/*
 * Copyright 2009  Veselin Georgiev,
 * anrieffNOSPAM @ mgail_DOT.com (convert to gmail)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include "libcpuid.h"
#include "asm-bits.h"
#include "libcpuid_util.h"
#include "rdtsc.h"

#if defined (__linux__) || defined (__gnu_linux__)
/* Assuming linux with /dev/cpu/x/msr: */
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
struct msr_driver_t { int fd; };
static int rdmsr_supported(void);
struct msr_driver_t* cpu_msr_driver_open(void)
{
	return cpu_msr_driver_open_core(0);
}

struct msr_driver_t* cpu_msr_driver_open_core(unsigned core_num)
{
	char msr[32];
	struct msr_driver_t* handle;
	if (core_num >= cpuid_get_total_cpus())	{
		set_error(ERR_INVCNB);
		return NULL;
	}
	if (!rdmsr_supported()) {
		set_error(ERR_NO_RDMSR);
		return NULL;
	}
	sprintf(msr, "/dev/cpu/%u/msr", core_num);
	int fd = open(msr, O_RDONLY);
	if (fd < 0) {
		if (errno == EIO) {
			set_error(ERR_NO_RDMSR);
			return NULL;
		}
		set_error(ERR_NO_DRIVER);
		return NULL;
	}
	handle = (struct msr_driver_t*) malloc(sizeof(struct msr_driver_t));
	handle->fd = fd;
	return handle;
}

int cpu_rdmsr(struct msr_driver_t* driver, uint32_t msr_index, uint64_t* result)
{
	ssize_t ret;

	if (!driver || driver->fd < 0)
		return set_error(ERR_HANDLE);
	ret = pread(driver->fd, result, 8, msr_index);
	if (ret != 8)
		return set_error(ERR_INVMSR);
	return 0;
}

int cpu_msr_driver_close(struct msr_driver_t* drv)
{
	if (drv && drv->fd >= 0) {
		close(drv->fd);
		free(drv);
	}
	return 0;
}

/* #endif defined (__linux__) || defined (__gnu_linux__) */

#elif defined (__FreeBSD__) || defined (__DragonFly__)
/* Assuming FreeBSD with /dev/cpuctlX */
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/cpuctl.h>

struct msr_driver_t { int fd; };
static int rdmsr_supported(void);
struct msr_driver_t* cpu_msr_driver_open(void)
{
	return cpu_msr_driver_open_core(0);
}

struct msr_driver_t* cpu_msr_driver_open_core(unsigned core_num)
{
	char msr[32];
	struct msr_driver_t* handle;
	if (core_num >= cpuid_get_total_cpus()) {
		set_error(ERR_INVCNB);
		return NULL;
	}
	if (!rdmsr_supported()) {
		set_error(ERR_NO_RDMSR);
		return NULL;
	}
	sprintf(msr, "/dev/cpuctl%u", core_num);
	int fd = open(msr, O_RDONLY);
	if (fd < 0) {
		if (errno == EIO) {
			set_error(ERR_NO_RDMSR);
			return NULL;
		}
		set_error(ERR_NO_DRIVER);
		return NULL;
	}
	handle = (struct msr_driver_t*) malloc(sizeof(struct msr_driver_t));
	handle->fd = fd;
	return handle;
}

int cpu_rdmsr(struct msr_driver_t* driver, uint32_t msr_index, uint64_t* result)
{
	cpuctl_msr_args_t args;
	args.msr = msr_index;

	if (!driver || driver->fd < 0)
		return set_error(ERR_HANDLE);

	if(ioctl(driver->fd, CPUCTL_RDMSR, &args))
		return set_error(ERR_INVMSR);

	*result = args.data; 
	return 0;
}

int cpu_msr_driver_close(struct msr_driver_t* drv)
{
	if (drv && drv->fd >= 0) {
		close(drv->fd);
		free(drv);
	}
	return 0;
}

/* #endif defined (__FreeBSD__) || defined (__DragonFly__) */

#elif defined (_WIN32)
#include <windows.h>
#include <winioctl.h>
#include <winerror.h>

extern uint8_t cc_x86driver_code[];
extern int cc_x86driver_code_size;
extern uint8_t cc_x64driver_code[];
extern int cc_x64driver_code_size;

struct msr_driver_t {
	char driver_path[MAX_PATH + 1];
	SC_HANDLE scManager;
	volatile SC_HANDLE scDriver;
	HANDLE hhDriver;
	OVERLAPPED ovl;
	int errorcode;
};

static int rdmsr_supported(void);
static int extract_driver(struct msr_driver_t* driver);
static int load_driver(struct msr_driver_t* driver);

struct msr_driver_t* cpu_msr_driver_open(void)
{
	struct msr_driver_t* drv;
	int status;
	if (!rdmsr_supported()) {
		set_error(ERR_NO_RDMSR);
		return NULL;
	}
	
	drv = (struct msr_driver_t*) malloc(sizeof(struct msr_driver_t));
	if (!drv) {
		set_error(ERR_NO_MEM);
		return NULL;
	}
	memset(drv, 0, sizeof(struct msr_driver_t));

	if (!extract_driver(drv)) {
		free(drv);
		set_error(ERR_EXTRACT);
		return NULL;
	}
	
	status = load_driver(drv);
	if (!DeleteFile(drv->driver_path))
		debugf(1, "Deleting temporary driver file failed.\n");
	if (!status) {
		set_error(drv->errorcode ? drv->errorcode : ERR_NO_DRIVER);
		free(drv);
		return NULL;
	}
	return drv;
}

struct msr_driver_t* cpu_msr_driver_open_core(unsigned core_num)
{
	warnf("cpu_msr_driver_open_core(): parameter ignored (function is the same as cpu_msr_driver_open)\n");
	return cpu_msr_driver_open();
}

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
static BOOL is_running_x64(void)
{
	BOOL bIsWow64 = FALSE;

	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(__TEXT("kernel32")), "IsWow64Process");
	if(NULL != fnIsWow64Process)
		fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
	return bIsWow64;
}


static int extract_driver(struct msr_driver_t* driver)
{
	FILE *f;
	if (!GetTempPath(sizeof(driver->driver_path), driver->driver_path)) return 0;
	strcat(driver->driver_path, "TmpRdr.sys");
	
	f = fopen(driver->driver_path, "wb");
	if (!f) return 0;
	if (is_running_x64())
		fwrite(cc_x64driver_code, 1, cc_x64driver_code_size, f);
	else
		fwrite(cc_x86driver_code, 1, cc_x86driver_code_size, f);
	fclose(f);
	return 1;
}

static BOOL wait_for_service_state(SC_HANDLE hService, DWORD dwDesiredState, SERVICE_STATUS *lpsrvStatus){
	BOOL fOK = FALSE;
	DWORD dwWaitHint;

	if(hService != NULL){
		while(TRUE){
			fOK = QueryServiceStatus(hService, lpsrvStatus);
			if(!fOK) 
				break;
			if(lpsrvStatus->dwCurrentState == dwDesiredState) 
				break;

			dwWaitHint = lpsrvStatus->dwWaitHint / 10;    // Poll 1/10 of the wait hint
			if (dwWaitHint <  1000) 
				dwWaitHint = 1000;  // At most once per second
			if (dwWaitHint > 10000) 
				dwWaitHint = 10000; // At least every 10 seconds
			Sleep(dwWaitHint);
		}
	}

	return fOK;
}

static int load_driver(struct msr_driver_t* drv)
{
	LPTSTR		lpszInfo = __TEXT("RDMSR Executor Driver");
	USHORT		uLen = 0;
	SERVICE_STATUS srvStatus = {0};
	BOOL		fRunning = FALSE;
	DWORD		dwLastError;
	LPTSTR		lpszDriverServiceName = __TEXT("TmpRdr");
	TCHAR		lpszDriverName[] = __TEXT("\\\\.\\Global\\TmpRdr");

	if((LPVOID)(drv->scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) != NULL) {
		drv->scDriver = CreateService(drv->scManager, lpszDriverServiceName, lpszInfo, SERVICE_ALL_ACCESS,
		                              SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
				                      drv->driver_path, NULL, NULL, NULL, NULL, NULL);
		if(drv->scDriver == NULL){
			switch(dwLastError = GetLastError()){
				case ERROR_SERVICE_EXISTS:
				case ERROR_SERVICE_MARKED_FOR_DELETE:{
					LPQUERY_SERVICE_CONFIG lpqsc;
					DWORD dwBytesNeeded;

					drv->scDriver = OpenService(drv->scManager, lpszDriverServiceName, SERVICE_ALL_ACCESS);
					if(drv->scDriver == NULL){
						debugf(1, "Error opening service: %d\n", GetLastError());
						break;
					}

					QueryServiceConfig(drv->scDriver, NULL, 0, &dwBytesNeeded);
					if((dwLastError = GetLastError()) == ERROR_INSUFFICIENT_BUFFER){
						lpqsc = calloc(1, dwBytesNeeded);
						if(!QueryServiceConfig(drv->scDriver, lpqsc, dwBytesNeeded, &dwBytesNeeded)){
							free(lpqsc);
							debugf(1, "Error query service config(adjusted buffer): %d\n", GetLastError());
							goto clean_up;
						}
						else{
							free(lpqsc);
						}
					}
					else{
						debugf(1, "Error query service config: %d\n", dwLastError);
						goto clean_up;
					}

					break;
				}
				case ERROR_ACCESS_DENIED:
					drv->errorcode = ERR_NO_PERMS;
					break;
				default:
					debugf(1, "Create driver service failed: %d\n", dwLastError);
					break;
			}				
		}
		if(drv->scDriver != NULL){
			if(StartService(drv->scDriver, 0, NULL)){
				if(!wait_for_service_state(drv->scDriver, SERVICE_RUNNING, &srvStatus)){
					debugf(1, "Driver load failed.\n");
					DeleteService(drv->scDriver);
					CloseServiceHandle(drv->scManager);
					drv->scDriver = NULL;
					goto clean_up;
				} else {
					fRunning = TRUE;
				}
			} else{
				if((dwLastError = GetLastError()) == ERROR_SERVICE_ALREADY_RUNNING)
					fRunning = TRUE;
				else{
					debugf(1, "Driver start failed.\n");
					DeleteService(drv->scDriver);
					CloseServiceHandle(drv->scManager);
					drv->scDriver = NULL;
					goto clean_up;
				}

			}
			if(fRunning)
				debugf(1, "Driver already running.\n");
			else
				debugf(1, "Driver loaded.\n"); 
			CloseServiceHandle(drv->scManager);
			drv->hhDriver = CreateFile(lpszDriverName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
			drv->ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			return 1;
		}
	} else {
		debugf(1, "Open SCM failed: %d\n", GetLastError());
	}

clean_up:
	if(drv->scManager != NULL){
		CloseServiceHandle(drv->scManager);
		drv->scManager = 0; // pointless
	}
	if(drv->scDriver != NULL){
		if(!DeleteService(drv->scDriver))
			debugf(1, "Delete driver service failed: %d\n", GetLastError());
		CloseServiceHandle(drv->scDriver);
		drv->scDriver = 0;
	}

	return 0;
}

#define FILE_DEVICE_UNKNOWN             0x00000022
#define IOCTL_UNKNOWN_BASE              FILE_DEVICE_UNKNOWN
#define IOCTL_PROCVIEW_RDMSR		CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0803, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

int cpu_rdmsr(struct msr_driver_t* driver, uint32_t msr_index, uint64_t* result)
{
	DWORD dwBytesReturned;
	__int64 msrdata;
	SERVICE_STATUS srvStatus = {0};

	if (!driver)
		return set_error(ERR_HANDLE);
	DeviceIoControl(driver->hhDriver, IOCTL_PROCVIEW_RDMSR, &msr_index, sizeof(int), &msrdata, sizeof(__int64), &dwBytesReturned, &driver->ovl);
	GetOverlappedResult(driver->hhDriver, &driver->ovl, &dwBytesReturned, TRUE);	
	*result = msrdata;
	return 0;
}

int cpu_msr_driver_close(struct msr_driver_t* drv)
{
	SERVICE_STATUS srvStatus = {0};
	if (drv == NULL) return 0;
	if(drv->scDriver != NULL){
		if (drv->hhDriver) CancelIo(drv->hhDriver);
		if(drv->ovl.hEvent != NULL)
			CloseHandle(drv->ovl.hEvent);
		if (drv->hhDriver) CloseHandle(drv->hhDriver);
		drv->hhDriver = NULL;
		drv->ovl.hEvent = NULL;
		if (ControlService(drv->scDriver, SERVICE_CONTROL_STOP, &srvStatus)){
			if (wait_for_service_state(drv->scDriver, SERVICE_STOPPED, &srvStatus)){
				DeleteService(drv->scDriver);
			}
		}
	}
	return 0;
}

/* endif defined (_WIN32) */

#else /* Unsupported OS */
/* On others OS (lie Darwin), we still do not support RDMSR, so supply dummy struct
   and functions */
struct msr_driver_t { int dummy; };
struct msr_driver_t* cpu_msr_driver_open(void)
{
	set_error(ERR_NOT_IMP);
	return NULL;
}

struct msr_driver_t* cpu_msr_driver_open_core(unsigned core_num)
{
	set_error(ERR_NOT_IMP);
	return NULL;
}

int cpu_rdmsr(struct msr_driver_t* driver, uint32_t msr_index, uint64_t* result)
{
	return set_error(ERR_NOT_IMP);
}

int cpu_msr_driver_close(struct msr_driver_t* driver)
{
	return set_error(ERR_NOT_IMP);
}

#define MSRINFO_DEFINED
int cpu_rdmsr_range(struct msr_driver_t* handle, uint32_t msr_index, uint8_t highbit,
                    uint8_t lowbit, uint64_t* result)
{
	return set_error(ERR_NOT_IMP);
}

int cpu_msrinfo(struct msr_driver_t* driver, cpu_msrinfo_request_t which)
{
	return set_error(ERR_NOT_IMP);
}

#endif /* Unsupported OS */


static int rdmsr_supported(void)
{
	struct cpu_id_t* id = get_cached_cpuid();
	return id->flags[CPU_FEATURE_MSR];
}

static int perfmsr_measure(struct msr_driver_t* handle, int msr)
{
	int err;
	uint64_t a, b;
	uint64_t x, y;
	err = cpu_rdmsr(handle, msr, &x);
	if (err) return CPU_INVALID_VALUE;
	sys_precise_clock(&a);
	busy_loop_delay(10);
	cpu_rdmsr(handle, msr, &y);
	sys_precise_clock(&b);
	if (a >= b || x > y) return CPU_INVALID_VALUE;
	return (int) ((y - x) / (b - a));
}

static double get_info_cur_multiplier(struct msr_driver_t* handle)
{
	int err, cur_clock;
	uint64_t r;
	static double bclk = 0.0;

	if(cpuid_get_vendor() == VENDOR_INTEL) {
		if(!bclk)
			bclk = (double) cpu_msrinfo(handle, INFO_BCLK) / 100;
		if(bclk > 0) {
			cur_clock = cpu_clock_by_ic(10, 4);
			if(cur_clock > 0)
				return cur_clock / bclk;
		}
	}

	err = cpu_rdmsr(handle, 0x2a, &r);
	if (err) return CPU_INVALID_VALUE;
	return (r>>22) & 0x1f;
}

static double get_info_max_multiplier(struct msr_driver_t* handle)
{
#define PLATFORM_INFO_MSR 206
#define PLATFORM_INFO_MSR_low 8
#define PLATFORM_INFO_MSR_high 15
	int err;
	uint64_t val, r;
	static int multiplier = 0;

	if(cpuid_get_vendor() == VENDOR_INTEL) {
		if(!multiplier) {
			cpu_rdmsr_range(handle, PLATFORM_INFO_MSR, PLATFORM_INFO_MSR_high, PLATFORM_INFO_MSR_low, &val);
			multiplier = (int) val;
		}
		if(multiplier > 0)
			return multiplier;
	}

	err = cpu_rdmsr(handle, 0x198, &r);
	if (err) return CPU_INVALID_VALUE;
	return (r >> 40) & 0x1f;
}

static int get_info_temperature(struct msr_driver_t* handle)
{
#define IA32_THERM_STATUS 0x19C
#define IA32_TEMPERATURE_TARGET 0x1a2
	uint64_t digital_readout, thermal_status, PROCHOT_temp;

	if(cpuid_get_vendor() == VENDOR_INTEL) {
		// https://github.com/ajaiantilal/i7z/blob/5023138d7c35c4667c938b853e5ea89737334e92/helper_functions.c#L59
		cpu_rdmsr_range(handle, IA32_THERM_STATUS, 23, 16, &digital_readout);
		cpu_rdmsr_range(handle, IA32_THERM_STATUS, 32, 31, &thermal_status);
		cpu_rdmsr_range(handle, IA32_TEMPERATURE_TARGET, 23, 16, &PROCHOT_temp);

		// These bits are thermal status : 1 if supported, 0 else
		if(thermal_status)
			return(PROCHOT_temp - digital_readout); // Temperature is prochot - digital readout
	}

	return CPU_INVALID_VALUE;
}

static double get_info_voltage(struct msr_driver_t* handle)
{
#define MSR_PERF_STATUS 0x198
#define MSR_PSTATE_S 0xC0010063
#define MSR_PSTATE_0 0xC0010064
	uint64_t val;

	if(cpuid_get_vendor() == VENDOR_INTEL) {
		cpu_rdmsr_range(handle, MSR_PERF_STATUS, 47, 32, &val);
		double ret = (double) val / (1 << 13);
		return (ret > 0) ? ret : CPU_INVALID_VALUE;
	}
	else if(cpuid_get_vendor() == VENDOR_AMD) {
		/* http://support.amd.com/TechDocs/42301_15h_Mod_00h-0Fh_BKDG.pdf
		   MSRC001_0063[2:0] = CurPstate
		   MSRC001_00[6B:64][15:9] = CpuVid */
		uint64_t CpuVid;
		cpu_rdmsr_range(handle, MSR_PSTATE_S, 2, 0, &val);
		if(val <= 7) { // Support 8 P-states
			cpu_rdmsr_range(handle, MSR_PSTATE_0 + val, 15, 9, &CpuVid);
			return 1.550 - 0.0125 * CpuVid; // 2.4.1.6.3 - Serial VID (SVI) Encodings
		}
	}

	return CPU_INVALID_VALUE;
}

static double get_info_bclk(struct msr_driver_t* handle)
{
	static int max_clock = 0, multiplier = 0;

	if(!max_clock)
		max_clock = cpu_clock_measure(100, 1); // Return the non-Turbo clock
	if(!multiplier)
		multiplier = cpu_msrinfo(handle, INFO_MAX_MULTIPLIER) / 100;
	if(max_clock > 0 && multiplier > 0)
		return (double) max_clock / multiplier;

	return CPU_INVALID_VALUE;
}

#ifndef MSRINFO_DEFINED

#define IA32_PACKAGE_THERM_STATUS 0x1b1
#define MSR_TURBO_RATIO_LIMIT 429

int cpu_rdmsr_range(struct msr_driver_t* handle, uint32_t msr_index, uint8_t highbit,
                    uint8_t lowbit, uint64_t* result)
{
	const uint8_t bits = highbit - lowbit + 1;

	if(highbit > 63 || lowbit > highbit)
		return set_error(ERR_INVRANGE);

	if(cpu_rdmsr(handle, msr_index, result))
		return set_error(ERR_HANDLE_R);

	if(bits < 64) {
		/* Show only part of register */
		*result >>= lowbit;
		*result &= (1ULL << bits) - 1;
	}

	return 0;
}

int cpu_msrinfo(struct msr_driver_t* handle, cpu_msrinfo_request_t which)
{
	if (handle == NULL)
		return set_error(ERR_HANDLE);
	switch (which) {
		case INFO_MPERF:
			return perfmsr_measure(handle, 0xe7);
		case INFO_APERF:
			return perfmsr_measure(handle, 0xe8);
		case INFO_CUR_MULTIPLIER:
			return (int) get_info_cur_multiplier(handle) * 100;
		case INFO_MAX_MULTIPLIER:
			return (int) get_info_max_multiplier(handle) * 100;
		case INFO_TEMPERATURE:
			return get_info_temperature(handle);
		case INFO_THROTTLING:
			return CPU_INVALID_VALUE;
		case INFO_VOLTAGE:
			return (int) get_info_voltage(handle) * 100;
		case INFO_BCLK:
			return (int) get_info_bclk(handle) * 100;
		default:
			return CPU_INVALID_VALUE;
	}
}
#endif // MSRINFO_DEFINED

