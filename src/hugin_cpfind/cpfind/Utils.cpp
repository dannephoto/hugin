/*
* Copyright (C) 2007-2008 Anael Orlinski
*
* This file is part of Panomatic.
*
* Panomatic is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* Panomatic is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Panomatic; if not, write to the Free Software
* <http://www.gnu.org/licenses/>.
*/

#include "Utils.h"
#if defined(HW_NCPU) || defined(__APPLE__)
#include <sys/sysctl.h>
#endif
#ifdef _WIN32
#include <windows.h>
#include <algorithm>
#elif defined __APPLE__
#include <CoreServices/CoreServices.h>  //for gestalt
#else
#include <unistd.h>
#endif

#ifdef _WIN32
unsigned long long utils::getTotalMemory()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
#ifndef _WIN64
    // when compiled as 32 bit version, we can only use about 2 GB
    // even if we have more memory available on a 64 bit system
    return std::min<unsigned long long>(status.ullTotalPhys, 1500*1024*1024);
#else
    return status.ullTotalPhys;
#endif
};
#elif defined __APPLE__
unsigned long long utils::getTotalMemory()
{
    SInt32 ramSize;
    if(Gestalt(gestaltPhysicalRAMSizeInMegabytes, &ramSize)==noErr)
    {
        unsigned long long _ramSize = ramSize;
        return _ramSize * 1024 * 1024;
    }
    else
    {
        // if query was not successful return 1 GB, 
        // return 0 would result in crash in calling function
        return 1024*1024*1024;
    }
};
#else
unsigned long long utils::getTotalMemory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
#endif

#if defined _WIN32
unsigned int utils::GetPhysicalCPUCount()
{
    // on Windows use GetLogicalProcessorInformationEx to get physical core count
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX procInfoTotal = NULL;
    DWORD length = 0;
    while (1)
    {
        if (GetLogicalProcessorInformationEx(RelationProcessorCore, procInfoTotal, &length))
        {
            break;
        };
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            return 1;
        };
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX tmpprocInfoTotal = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)realloc(procInfoTotal, length);
        if (!tmpprocInfoTotal)
        {
            free(procInfoTotal);
            return 1;
        }
        procInfoTotal = tmpprocInfoTotal;
    };

    unsigned int cpuCount = 0;
    for (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX procInfo = procInfoTotal;
        (void*)procInfo < (void*)((uintptr_t)procInfoTotal + length);
        procInfo = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)((uintptr_t)procInfo + procInfo->Size))
    {
        if (procInfo->Relationship == RelationProcessorCore)
        {
            ++cpuCount;
        };
    };
    free(procInfoTotal);
    return cpuCount > 0 ? cpuCount : 1;
}
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/sysctl.h>

unsigned int utils::GetPhysicalCPUCount()
{
    uint32_t num_cores = 0;
    size_t num_cores_len = sizeof(num_cores);
    sysctlbyname("hw.physicalcpu", &num_cores, &num_cores_len, 0, 0);
    return num_cores;
}
#else
// for Linux, read from /proc/cpuinfo
#include <cstring>
#include <stdio.h>

unsigned int utils::GetPhysicalCPUCount()
{
    char str[256];
    int cpuCount = 0;
    FILE* fp;
    if ((fp = fopen("/proc/cpuinfo", "r")))
    {
        while (fgets(str, sizeof(str), fp))
        {
            if (memcmp(str, "core id", 7) == 0)
            {
                ++cpuCount;
            };
        };
        fclose(fp);
    };
    return cpuCount > 0 ? cpuCount : 1;
}
#endif
