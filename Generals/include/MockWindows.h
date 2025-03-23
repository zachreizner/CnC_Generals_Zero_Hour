#ifndef __MOCK_WINDOWS_H__
#define __MOCK_WINDOWS_H__

#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define S_OK 0

typedef uint32_t HRESULT;
typedef void *HANDLE;
typedef HANDLE HKEY;
typedef HANDLE *PHKEY;
typedef HANDLE HWND;
typedef HANDLE HMODULE;
typedef void *CRITICAL_SECTION;
typedef CRITICAL_SECTION LPCRITICAL_SECTION;
typedef const void *LPCVOID;
typedef int (*FARPROC)();
typedef bool *LPBOOL;
typedef char *LPSTR;
typedef const char *LPCTSTR;
typedef char *LPTSTR;
typedef const char *LPCCH;
typedef const wchar_t *LPCWCH;
typedef const char *LPCSTR;
typedef const wchar_t *LPCWSTR;
typedef wchar_t *LPWSTR;
typedef char CHAR;
typedef uint16_t USHORT;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint32_t ULONG;
typedef uint32_t UINT;
typedef int64_t LONGLONG;
typedef uint32_t MMRESULT;
typedef int32_t BOOL;
typedef uint32_t REGSAM;
typedef uint32_t *LPDWORD;
typedef uint32_t LSTATUS;
typedef uintptr_t SIZE_T;
typedef uintptr_t ULONG_PTR;
typedef uintptr_t DWORD_PTR;
typedef float FLOAT;
MMRESULT timeBeginPeriod(UINT uPeriod);
MMRESULT timeEndPeriod(UINT uPeriod);
DWORD timeGetTime();
void Sleep(DWORD dwMilliseconds);

#define INVALID_HANDLE_VALUE ((HANDLE)(uintptr_t)-1)

#define _MAX_PATH 260
#define MAX_PATH _MAX_PATH

DWORD GetModuleFileName(
    HMODULE hModule,
    LPSTR lpFilename,
    DWORD nSize);

UINT GetDoubleClickTime();

#define CSIDL_PERSONAL 0 // chosen arbitrarily

BOOL SHGetSpecialFolderPath(
    HWND hwnd,
    LPSTR pszPath,
    int csidl,
    BOOL fCreate);

BOOL CreateDirectory(
    LPCTSTR lpPathName,
    void *lpSecurityAttributes // Always NULL
);

typedef struct _SYSTEMTIME
{
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
    LPSYSTEMTIME lpSystemTime);

#define HKEY_LOCAL_MACHINE ((void *)0) // chosen arbitrarily
#define HKEY_CURRENT_USER ((void *)1)  // chosen arbitrarily

LSTATUS RegCreateKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD Reserved,
    LPSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    const void *lpSecurityAttributes, // always NULL
    PHKEY phkResult,
    LPDWORD lpdwDisposition);

#define GMEM_FIXED 0x0000
#define GMEM_ZEROINIT 0x0040

void *GlobalAlloc(
    UINT uFlags,
    SIZE_T dwBytes);

void *GlobalFree(
    void *hMem);

typedef struct D3DXVECTOR4
{
    FLOAT x;
    FLOAT y;
    FLOAT z;
    FLOAT w;
#ifdef __cplusplus
    D3DXVECTOR4() = default;
    D3DXVECTOR4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w)
    : x(_x),
      y(_y),
      z(_z),
      w(_w)
    {
    }
#endif
} D3DXVECTOR4;

FLOAT D3DXVec4Dot(
    const D3DXVECTOR4 *pV1,
    const D3DXVECTOR4 *pV2
);

typedef struct D3DXMATRIX
{
    FLOAT _11;
    FLOAT _12;
    FLOAT _13;
    FLOAT _14;
    FLOAT _21;
    FLOAT _22;
    FLOAT _23;
    FLOAT _24;
    FLOAT _31;
    FLOAT _32;
    FLOAT _33;
    FLOAT _34;
    FLOAT _41;
    FLOAT _42;
    FLOAT _43;
    FLOAT _44;
#ifdef __cplusplus
    D3DXMATRIX(
        FLOAT __11,
        FLOAT __12,
        FLOAT __13,
        FLOAT __14,
        FLOAT __21,
        FLOAT __22,
        FLOAT __23,
        FLOAT __24,
        FLOAT __31,
        FLOAT __32,
        FLOAT __33,
        FLOAT __34,
        FLOAT __41,
        FLOAT __42,
        FLOAT __43,
        FLOAT __44) : _11(__11),
                      _12(__12),
                      _13(__13),
                      _14(__14),
                      _21(__21),
                      _22(__22),
                      _23(__23),
                      _24(__24),
                      _31(__31),
                      _32(__32),
                      _33(__33),
                      _34(__34),
                      _41(__41),
                      _42(__42),
                      _43(__43),
                      _44(__44)
    {
    }
#endif
} D3DXMATRIX;

D3DXVECTOR4* D3DXVec4Transform(
    D3DXVECTOR4 *pOut,
    const D3DXVECTOR4 *pV,
    const D3DXMATRIX  *pM
);

char *itoa(int value, char *str, int base);

FARPROC GetProcAddress(
    HMODULE hModule,
    LPCSTR lpProcName);

HMODULE LoadLibrary(
    LPCSTR lpLibFileName);

BOOL FreeLibrary(
    HMODULE hLibModule);

BOOL SetWindowTextW(
    HWND hWnd,
    LPCWSTR lpString);

typedef struct _MEMORYSTATUS
{
    DWORD dwLength;
    DWORD dwMemoryLoad;
    SIZE_T dwTotalPhys;
    SIZE_T dwAvailPhys;
    SIZE_T dwTotalPageFile;
    SIZE_T dwAvailPageFile;
    SIZE_T dwTotalVirtual;
    SIZE_T dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

void GlobalMemoryStatus(
    LPMEMORYSTATUS lpBuffer);

int iswascii(
    int c);

int AddFontResource(
    LPCSTR lpFileName);

BOOL RemoveFontResource(
    LPCSTR lpFileName);

#define VK_RETURN 0x0D

DWORD GetTickCount();

#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000

DWORD FormatMessageW(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPWSTR lpBuffer,
    DWORD nSize,
    void *Arguments // always NULL
);

DWORD FormatMessage(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPTSTR lpBuffer,
    DWORD nSize,
    void *Arguments // always NULL
);

DWORD GetLastError();

BOOL DeleteFile(
    LPCTSTR lpFileName);

BOOL CopyFile(
    LPCTSTR lpExistingFileName,
    LPCTSTR lpNewFileName,
    BOOL bFailIfExists);

void GetDesktopDirectory(char *path);

#define UNLEN 256

BOOL GetUserName(
    LPSTR lpBuffer,
    LPDWORD pcbBuffer);

#define MAX_COMPUTERNAME_LENGTH 256

BOOL GetComputerName(
    LPSTR lpBuffer,
    LPDWORD nSize);

typedef union _LARGE_INTEGER
{
    LONGLONG QuadPart;
} LARGE_INTEGER;

BOOL QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);

#define closesocket close

#define LOCALE_SYSTEM_DEFAULT 1
#define DATE_SHORTDATE 1

int GetDateFormat(
    int Locale,
    DWORD dwFlags,
    const SYSTEMTIME *lpDate,
    LPCSTR lpFormat,
    LPSTR lpDateStr,
    int cchDate);

int GetDateFormatW(
    int Locale,
    DWORD dwFlags,
    const SYSTEMTIME *lpDate,
    LPCWSTR lpFormat,
    LPWSTR lpDateStr,
    int cchDate);

#define TIME_NOSECONDS 1

int GetTimeFormatW(
    int Locale,
    DWORD dwFlags,
    const SYSTEMTIME *lpTime,
    LPCWSTR lpFormat,
    LPWSTR lpTimeStr,
    int cchTime);

static inline int _wtoi(const wchar_t *str)
{
    return (int)wcstol(str, 0, 10);
}

int _access(
    const char *path,
    int mode);

DWORD GetCurrentDirectory(
    DWORD nBufferLength,
    LPTSTR lpBuffer);

BOOL SetCurrentDirectory(
    LPCTSTR lpPathName);

#define FILE_ATTRIBUTE_DIRECTORY 0x10

typedef struct _WIN32_FIND_DATA
{
    DWORD dwFileAttributes;
    CHAR cFileName[MAX_PATH];
} WIN32_FIND_DATA, *LPWIN32_FIND_DATA;

HANDLE FindFirstFile(
    LPCSTR lpFileName,
    LPWIN32_FIND_DATA lpFindFileData);

BOOL FindNextFile(
    HANDLE hFindFile,
    LPWIN32_FIND_DATA lpFindFileData);

BOOL FindClose(
    HANDLE hFindFile);

#define CP_UTF8 65001

int MultiByteToWideChar(
    UINT CodePage,
    DWORD dwFlags,
    LPCCH lpMultiByteStr,
    int cbMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar);

int WideCharToMultiByte(
    UINT CodePage,
    DWORD dwFlags,
    LPCWCH lpWideCharStr,
    int cchWideChar,
    LPSTR lpMultiByteStr,
    int cbMultiByte,
    LPCCH lpDefaultChar,
    LPBOOL lpUsedDefaultChar);

void InitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection);

void EnterCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection);

void LeaveCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection);

void DeleteCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection);

#define WAIT_OBJECT_0 0
#define INFINITE 0xFFFFFFFF

DWORD WaitForSingleObject(
    HANDLE hHandle,
    DWORD dwMilliseconds);

HANDLE CreateMutex(
    void * lpMutexAttributes, // Always NULL
    BOOL                  bInitialOwner,
    LPCSTR                lpName
);

BOOL ReleaseMutex(
    HANDLE hMutex);

BOOL CloseHandle(
    HANDLE hObject);

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif
#define THREAD_PRIORITY_NORMAL 4


DWORD GetCurrentThreadId();

BOOL TerminateThread(
    HANDLE hThread, 
    DWORD  dwExitCode
);

uintptr_t _beginthread(
    void( *start_address )( void * ),
    unsigned stack_size,
    void *arglist
);

BOOL SetThreadPriority(
    HANDLE hThread,
    int    nPriority
  );
  

HANDLE CreateEvent(
    void* lpEventAttributes, // Always NULL
    BOOL                  bManualReset,
    BOOL                  bInitialState,
    LPCSTR                lpName
  );

#ifdef __cplusplus
}
#endif

#endif
