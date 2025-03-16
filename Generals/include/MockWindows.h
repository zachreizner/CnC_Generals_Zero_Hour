#ifndef __MOCK_WINDOWS_H__
#define __MOCK_WINDOWS_H__

#include <stdint.h>

typedef void* HANDLE;
typedef HANDLE HKEY;
typedef HANDLE* PHKEY;
typedef HANDLE HWND;
typedef HANDLE HMODULE;
typedef char* LPSTR;
typedef const char* LPCTSTR;
typedef const char* LPCSTR;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint32_t ULONG;
typedef uint32_t UINT;
typedef uint32_t MMRESULT;
typedef int32_t BOOL;
typedef uint32_t REGSAM;
typedef uint32_t* LPDWORD;
typedef uint32_t LSTATUS;
typedef uintptr_t SIZE_T;
typedef uintptr_t ULONG_PTR;
typedef uintptr_t DWORD_PTR;
MMRESULT timeBeginPeriod(UINT uPeriod);
MMRESULT timeEndPeriod(UINT uPeriod);
DWORD timeGetTime();
void Sleep(DWORD dwMilliseconds);

#define _MAX_PATH 260
#define MAX_PATH _MAX_PATH

DWORD GetModuleFileName(
    HMODULE hModule,
    LPSTR   lpFilename,
    DWORD   nSize
);

UINT GetDoubleClickTime();

#define CSIDL_PERSONAL 0 // chosen arbitrarily

BOOL SHGetSpecialFolderPath(
    HWND  hwnd,
    LPSTR pszPath,
    int   csidl,
    BOOL  fCreate
);

BOOL CreateDirectory(
    LPCTSTR               lpPathName,
    void * lpSecurityAttributes // Always NULL
);

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

void GetLocalTime(
    LPSYSTEMTIME lpSystemTime
);

#define HKEY_LOCAL_MACHINE ((void*)0 )// chosen arbitrarily
#define HKEY_CURRENT_USER ((void*)1 )// chosen arbitrarily

LSTATUS RegCreateKeyExA(
    HKEY                        hKey,
    LPCSTR                      lpSubKey,
    DWORD                       Reserved,
    LPSTR                       lpClass,
    DWORD                       dwOptions,
    REGSAM                      samDesired,
    const void *                lpSecurityAttributes, // always NULL
    PHKEY                       phkResult,
    LPDWORD                     lpdwDisposition
);

#define GMEM_FIXED 0x0000
#define GMEM_ZEROINIT 0x0040

void * GlobalAlloc(
    UINT   uFlags,
    SIZE_T dwBytes
);

void* GlobalFree(
    void* hMem
);

#endif
