#![allow(non_camel_case_types)]
#![allow(unused_variables)]
#![allow(non_snake_case)]

pub const S_OK: u32 = 0;
pub const _MAX_PATH: u32 = 260;
pub const MAX_PATH: u32 = 260;
pub const CSIDL_PERSONAL: u32 = 0;
pub const GMEM_FIXED: u32 = 0;
pub const GMEM_ZEROINIT: u32 = 64;
pub const VK_RETURN: u32 = 13;
pub const FORMAT_MESSAGE_FROM_SYSTEM: u32 = 4096;
pub const UNLEN: u32 = 256;
pub const MAX_COMPUTERNAME_LENGTH: u32 = 256;
pub const LOCALE_SYSTEM_DEFAULT: u32 = 1;
pub const DATE_SHORTDATE: u32 = 1;
pub const TIME_NOSECONDS: u32 = 1;
pub const FILE_ATTRIBUTE_DIRECTORY: u32 = 16;
pub const CP_UTF8: u32 = 65001;
pub const WAIT_OBJECT_0: u32 = 0;
pub const INFINITE: u32 = 4294967295;
pub type wchar_t = ::std::os::raw::c_int;
pub type HRESULT = u32;
pub type HANDLE = *mut ::std::os::raw::c_void;
pub type HKEY = HANDLE;
pub type PHKEY = *mut HANDLE;
pub type HWND = HANDLE;
pub type HMODULE = HANDLE;
pub type CRITICAL_SECTION = *mut ::std::os::raw::c_void;
pub type LPCRITICAL_SECTION = CRITICAL_SECTION;
pub type LPCVOID = *const ::std::os::raw::c_void;
pub type FARPROC = ::std::option::Option<unsafe extern "C" fn() -> ::std::os::raw::c_int>;
pub type LPBOOL = *mut bool;
pub type LPSTR = *mut ::std::os::raw::c_char;
pub type LPCTSTR = *const ::std::os::raw::c_char;
pub type LPTSTR = *mut ::std::os::raw::c_char;
pub type LPCCH = *const ::std::os::raw::c_char;
pub type LPCWCH = *const wchar_t;
pub type LPCSTR = *const ::std::os::raw::c_char;
pub type LPCWSTR = *const wchar_t;
pub type LPWSTR = *mut wchar_t;
pub type CHAR = ::std::os::raw::c_char;
pub type USHORT = u16;
pub type WORD = u16;
pub type DWORD = u32;
pub type ULONG = u32;
pub type UINT = u32;
pub type LONGLONG = i64;
pub type MMRESULT = u32;
pub type BOOL = i32;
pub type REGSAM = u32;
pub type LPDWORD = *mut u32;
pub type LSTATUS = u32;
pub type SIZE_T = usize;
pub type ULONG_PTR = usize;
pub type DWORD_PTR = usize;
pub type FLOAT = f32;

#[unsafe(no_mangle)]
pub unsafe extern "C" fn timeBeginPeriod(uPeriod: UINT) -> MMRESULT {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn timeEndPeriod(uPeriod: UINT) -> MMRESULT {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn timeGetTime() -> DWORD {
    todo!()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn Sleep(dwMilliseconds: DWORD) {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetModuleFileName(
    hModule: HMODULE,
    lpFilename: LPSTR,
    nSize: DWORD,
) -> DWORD {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetDoubleClickTime() -> UINT {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn SHGetSpecialFolderPath(
    hwnd: HWND,
    pszPath: LPSTR,
    csidl: ::std::os::raw::c_int,
    fCreate: BOOL,
) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn CreateDirectory(
    lpPathName: LPCTSTR,
    lpSecurityAttributes: *mut ::std::os::raw::c_void,
) -> BOOL {
    todo!()
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct _SYSTEMTIME {
    pub wYear: WORD,
    pub wMonth: WORD,
    pub wDayOfWeek: WORD,
    pub wDay: WORD,
    pub wHour: WORD,
    pub wMinute: WORD,
    pub wSecond: WORD,
    pub wMilliseconds: WORD,
}
#[allow(clippy::unnecessary_operation, clippy::identity_op)]
const _: () = {
    ["Size of _SYSTEMTIME"][::std::mem::size_of::<_SYSTEMTIME>() - 16usize];
    ["Alignment of _SYSTEMTIME"][::std::mem::align_of::<_SYSTEMTIME>() - 2usize];
    ["Offset of field: _SYSTEMTIME::wYear"][::std::mem::offset_of!(_SYSTEMTIME, wYear) - 0usize];
    ["Offset of field: _SYSTEMTIME::wMonth"][::std::mem::offset_of!(_SYSTEMTIME, wMonth) - 2usize];
    ["Offset of field: _SYSTEMTIME::wDayOfWeek"]
        [::std::mem::offset_of!(_SYSTEMTIME, wDayOfWeek) - 4usize];
    ["Offset of field: _SYSTEMTIME::wDay"][::std::mem::offset_of!(_SYSTEMTIME, wDay) - 6usize];
    ["Offset of field: _SYSTEMTIME::wHour"][::std::mem::offset_of!(_SYSTEMTIME, wHour) - 8usize];
    ["Offset of field: _SYSTEMTIME::wMinute"]
        [::std::mem::offset_of!(_SYSTEMTIME, wMinute) - 10usize];
    ["Offset of field: _SYSTEMTIME::wSecond"]
        [::std::mem::offset_of!(_SYSTEMTIME, wSecond) - 12usize];
    ["Offset of field: _SYSTEMTIME::wMilliseconds"]
        [::std::mem::offset_of!(_SYSTEMTIME, wMilliseconds) - 14usize];
};
pub type SYSTEMTIME = _SYSTEMTIME;
pub type LPSYSTEMTIME = *mut _SYSTEMTIME;
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetLocalTime(lpSystemTime: LPSYSTEMTIME) {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn RegCreateKeyExA(
    hKey: HKEY,
    lpSubKey: LPCSTR,
    Reserved: DWORD,
    lpClass: LPSTR,
    dwOptions: DWORD,
    samDesired: REGSAM,
    lpSecurityAttributes: *const ::std::os::raw::c_void,
    phkResult: PHKEY,
    lpdwDisposition: LPDWORD,
) -> LSTATUS {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GlobalAlloc(uFlags: UINT, dwBytes: SIZE_T) -> *mut ::std::os::raw::c_void {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GlobalFree(
    hMem: *mut ::std::os::raw::c_void,
) -> *mut ::std::os::raw::c_void {
    todo!()
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct D3DXVECTOR4 {
    pub x: FLOAT,
    pub y: FLOAT,
    pub z: FLOAT,
    pub w: FLOAT,
}
#[allow(clippy::unnecessary_operation, clippy::identity_op)]
const _: () = {
    ["Size of D3DXVECTOR4"][::std::mem::size_of::<D3DXVECTOR4>() - 16usize];
    ["Alignment of D3DXVECTOR4"][::std::mem::align_of::<D3DXVECTOR4>() - 4usize];
    ["Offset of field: D3DXVECTOR4::x"][::std::mem::offset_of!(D3DXVECTOR4, x) - 0usize];
    ["Offset of field: D3DXVECTOR4::y"][::std::mem::offset_of!(D3DXVECTOR4, y) - 4usize];
    ["Offset of field: D3DXVECTOR4::z"][::std::mem::offset_of!(D3DXVECTOR4, z) - 8usize];
    ["Offset of field: D3DXVECTOR4::w"][::std::mem::offset_of!(D3DXVECTOR4, w) - 12usize];
};

#[unsafe(no_mangle)]
pub unsafe extern "C" fn D3DXVec4Dot(pV1: *const D3DXVECTOR4, pV2: *const D3DXVECTOR4) -> FLOAT {
    todo!()
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct D3DXMATRIX {
    pub _11: FLOAT,
    pub _12: FLOAT,
    pub _13: FLOAT,
    pub _14: FLOAT,
    pub _21: FLOAT,
    pub _22: FLOAT,
    pub _23: FLOAT,
    pub _24: FLOAT,
    pub _31: FLOAT,
    pub _32: FLOAT,
    pub _33: FLOAT,
    pub _34: FLOAT,
    pub _41: FLOAT,
    pub _42: FLOAT,
    pub _43: FLOAT,
    pub _44: FLOAT,
}
#[allow(clippy::unnecessary_operation, clippy::identity_op)]
const _: () = {
    ["Size of D3DXMATRIX"][::std::mem::size_of::<D3DXMATRIX>() - 64usize];
    ["Alignment of D3DXMATRIX"][::std::mem::align_of::<D3DXMATRIX>() - 4usize];
    ["Offset of field: D3DXMATRIX::_11"][::std::mem::offset_of!(D3DXMATRIX, _11) - 0usize];
    ["Offset of field: D3DXMATRIX::_12"][::std::mem::offset_of!(D3DXMATRIX, _12) - 4usize];
    ["Offset of field: D3DXMATRIX::_13"][::std::mem::offset_of!(D3DXMATRIX, _13) - 8usize];
    ["Offset of field: D3DXMATRIX::_14"][::std::mem::offset_of!(D3DXMATRIX, _14) - 12usize];
    ["Offset of field: D3DXMATRIX::_21"][::std::mem::offset_of!(D3DXMATRIX, _21) - 16usize];
    ["Offset of field: D3DXMATRIX::_22"][::std::mem::offset_of!(D3DXMATRIX, _22) - 20usize];
    ["Offset of field: D3DXMATRIX::_23"][::std::mem::offset_of!(D3DXMATRIX, _23) - 24usize];
    ["Offset of field: D3DXMATRIX::_24"][::std::mem::offset_of!(D3DXMATRIX, _24) - 28usize];
    ["Offset of field: D3DXMATRIX::_31"][::std::mem::offset_of!(D3DXMATRIX, _31) - 32usize];
    ["Offset of field: D3DXMATRIX::_32"][::std::mem::offset_of!(D3DXMATRIX, _32) - 36usize];
    ["Offset of field: D3DXMATRIX::_33"][::std::mem::offset_of!(D3DXMATRIX, _33) - 40usize];
    ["Offset of field: D3DXMATRIX::_34"][::std::mem::offset_of!(D3DXMATRIX, _34) - 44usize];
    ["Offset of field: D3DXMATRIX::_41"][::std::mem::offset_of!(D3DXMATRIX, _41) - 48usize];
    ["Offset of field: D3DXMATRIX::_42"][::std::mem::offset_of!(D3DXMATRIX, _42) - 52usize];
    ["Offset of field: D3DXMATRIX::_43"][::std::mem::offset_of!(D3DXMATRIX, _43) - 56usize];
    ["Offset of field: D3DXMATRIX::_44"][::std::mem::offset_of!(D3DXMATRIX, _44) - 60usize];
};

#[unsafe(no_mangle)]
pub unsafe extern "C" fn D3DXVec4Transform(
    pOut: *mut D3DXVECTOR4,
    pV: *const D3DXVECTOR4,
    pM: *const D3DXMATRIX,
) -> *mut D3DXVECTOR4 {
    todo!()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn D3DXMatrixInverse(
    pOut: *mut D3DXMATRIX,
    pDeterminant: *mut FLOAT,
    pM: *const D3DXMATRIX,
) -> *mut D3DXMATRIX {
    todo!()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn itoa(
    value: ::std::os::raw::c_int,
    str_: *mut ::std::os::raw::c_char,
    base: ::std::os::raw::c_int,
) -> *mut ::std::os::raw::c_char {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetProcAddress(hModule: HMODULE, lpProcName: LPCSTR) -> FARPROC {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn LoadLibrary(lpLibFileName: LPCSTR) -> HMODULE {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn FreeLibrary(hLibModule: HMODULE) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn SetWindowTextW(hWnd: HWND, lpString: LPCWSTR) -> BOOL {
    todo!()
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct _MEMORYSTATUS {
    pub dwLength: DWORD,
    pub dwMemoryLoad: DWORD,
    pub dwTotalPhys: SIZE_T,
    pub dwAvailPhys: SIZE_T,
    pub dwTotalPageFile: SIZE_T,
    pub dwAvailPageFile: SIZE_T,
    pub dwTotalVirtual: SIZE_T,
    pub dwAvailVirtual: SIZE_T,
}
#[allow(clippy::unnecessary_operation, clippy::identity_op)]
const _: () = {
    ["Size of _MEMORYSTATUS"][::std::mem::size_of::<_MEMORYSTATUS>() - 56usize];
    ["Alignment of _MEMORYSTATUS"][::std::mem::align_of::<_MEMORYSTATUS>() - 8usize];
    ["Offset of field: _MEMORYSTATUS::dwLength"]
        [::std::mem::offset_of!(_MEMORYSTATUS, dwLength) - 0usize];
    ["Offset of field: _MEMORYSTATUS::dwMemoryLoad"]
        [::std::mem::offset_of!(_MEMORYSTATUS, dwMemoryLoad) - 4usize];
    ["Offset of field: _MEMORYSTATUS::dwTotalPhys"]
        [::std::mem::offset_of!(_MEMORYSTATUS, dwTotalPhys) - 8usize];
    ["Offset of field: _MEMORYSTATUS::dwAvailPhys"]
        [::std::mem::offset_of!(_MEMORYSTATUS, dwAvailPhys) - 16usize];
    ["Offset of field: _MEMORYSTATUS::dwTotalPageFile"]
        [::std::mem::offset_of!(_MEMORYSTATUS, dwTotalPageFile) - 24usize];
    ["Offset of field: _MEMORYSTATUS::dwAvailPageFile"]
        [::std::mem::offset_of!(_MEMORYSTATUS, dwAvailPageFile) - 32usize];
    ["Offset of field: _MEMORYSTATUS::dwTotalVirtual"]
        [::std::mem::offset_of!(_MEMORYSTATUS, dwTotalVirtual) - 40usize];
    ["Offset of field: _MEMORYSTATUS::dwAvailVirtual"]
        [::std::mem::offset_of!(_MEMORYSTATUS, dwAvailVirtual) - 48usize];
};
pub type MEMORYSTATUS = _MEMORYSTATUS;
pub type LPMEMORYSTATUS = *mut _MEMORYSTATUS;
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GlobalMemoryStatus(lpBuffer: LPMEMORYSTATUS) {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn iswascii(c: ::std::os::raw::c_int) -> ::std::os::raw::c_int {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn AddFontResource(lpFileName: LPCSTR) -> ::std::os::raw::c_int {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn RemoveFontResource(lpFileName: LPCSTR) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetTickCount() -> DWORD {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn FormatMessageW(
    dwFlags: DWORD,
    lpSource: LPCVOID,
    dwMessageId: DWORD,
    dwLanguageId: DWORD,
    lpBuffer: LPWSTR,
    nSize: DWORD,
    Arguments: *mut ::std::os::raw::c_void,
) -> DWORD {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn FormatMessage(
    dwFlags: DWORD,
    lpSource: LPCVOID,
    dwMessageId: DWORD,
    dwLanguageId: DWORD,
    lpBuffer: LPTSTR,
    nSize: DWORD,
    Arguments: *mut ::std::os::raw::c_void,
) -> DWORD {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetLastError() -> DWORD {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn DeleteFile(lpFileName: LPCTSTR) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn CopyFile(
    lpExistingFileName: LPCTSTR,
    lpNewFileName: LPCTSTR,
    bFailIfExists: BOOL,
) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetDesktopDirectory(path: *mut ::std::os::raw::c_char) {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetUserName(lpBuffer: LPSTR, pcbBuffer: LPDWORD) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetComputerName(lpBuffer: LPSTR, nSize: LPDWORD) -> BOOL {
    todo!()
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union _LARGE_INTEGER {
    pub QuadPart: LONGLONG,
}
#[allow(clippy::unnecessary_operation, clippy::identity_op)]
const _: () = {
    ["Size of _LARGE_INTEGER"][::std::mem::size_of::<_LARGE_INTEGER>() - 8usize];
    ["Alignment of _LARGE_INTEGER"][::std::mem::align_of::<_LARGE_INTEGER>() - 8usize];
    ["Offset of field: _LARGE_INTEGER::QuadPart"]
        [::std::mem::offset_of!(_LARGE_INTEGER, QuadPart) - 0usize];
};
pub type LARGE_INTEGER = _LARGE_INTEGER;
#[unsafe(no_mangle)]
pub unsafe extern "C" fn QueryPerformanceCounter(lpPerformanceCount: *mut LARGE_INTEGER) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn QueryPerformanceFrequency(lpFrequency: *mut LARGE_INTEGER) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetDateFormat(
    Locale: ::std::os::raw::c_int,
    dwFlags: DWORD,
    lpDate: *const SYSTEMTIME,
    lpFormat: LPCSTR,
    lpDateStr: LPSTR,
    cchDate: ::std::os::raw::c_int,
) -> ::std::os::raw::c_int {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetDateFormatW(
    Locale: ::std::os::raw::c_int,
    dwFlags: DWORD,
    lpDate: *const SYSTEMTIME,
    lpFormat: LPCWSTR,
    lpDateStr: LPWSTR,
    cchDate: ::std::os::raw::c_int,
) -> ::std::os::raw::c_int {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetTimeFormatW(
    Locale: ::std::os::raw::c_int,
    dwFlags: DWORD,
    lpTime: *const SYSTEMTIME,
    lpFormat: LPCWSTR,
    lpTimeStr: LPWSTR,
    cchTime: ::std::os::raw::c_int,
) -> ::std::os::raw::c_int {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn _access(
    path: *const ::std::os::raw::c_char,
    mode: ::std::os::raw::c_int,
) -> ::std::os::raw::c_int {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetCurrentDirectory(nBufferLength: DWORD, lpBuffer: LPTSTR) -> DWORD {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn SetCurrentDirectory(lpPathName: LPCTSTR) -> BOOL {
    todo!()
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct _WIN32_FIND_DATA {
    pub dwFileAttributes: DWORD,
    pub cFileName: [CHAR; 260usize],
}
#[allow(clippy::unnecessary_operation, clippy::identity_op)]
const _: () = {
    ["Size of _WIN32_FIND_DATA"][::std::mem::size_of::<_WIN32_FIND_DATA>() - 264usize];
    ["Alignment of _WIN32_FIND_DATA"][::std::mem::align_of::<_WIN32_FIND_DATA>() - 4usize];
    ["Offset of field: _WIN32_FIND_DATA::dwFileAttributes"]
        [::std::mem::offset_of!(_WIN32_FIND_DATA, dwFileAttributes) - 0usize];
    ["Offset of field: _WIN32_FIND_DATA::cFileName"]
        [::std::mem::offset_of!(_WIN32_FIND_DATA, cFileName) - 4usize];
};
pub type WIN32_FIND_DATA = _WIN32_FIND_DATA;
pub type LPWIN32_FIND_DATA = *mut _WIN32_FIND_DATA;
#[unsafe(no_mangle)]
pub unsafe extern "C" fn FindFirstFile(
    lpFileName: LPCSTR,
    lpFindFileData: LPWIN32_FIND_DATA,
) -> HANDLE {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn FindNextFile(
    hFindFile: HANDLE,
    lpFindFileData: LPWIN32_FIND_DATA,
) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn FindClose(hFindFile: HANDLE) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn MultiByteToWideChar(
    CodePage: UINT,
    dwFlags: DWORD,
    lpMultiByteStr: LPCCH,
    cbMultiByte: ::std::os::raw::c_int,
    lpWideCharStr: LPWSTR,
    cchWideChar: ::std::os::raw::c_int,
) -> ::std::os::raw::c_int {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn WideCharToMultiByte(
    CodePage: UINT,
    dwFlags: DWORD,
    lpWideCharStr: LPCWCH,
    cchWideChar: ::std::os::raw::c_int,
    lpMultiByteStr: LPSTR,
    cbMultiByte: ::std::os::raw::c_int,
    lpDefaultChar: LPCCH,
    lpUsedDefaultChar: LPBOOL,
) -> ::std::os::raw::c_int {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn InitializeCriticalSection(lpCriticalSection: LPCRITICAL_SECTION) {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn EnterCriticalSection(lpCriticalSection: LPCRITICAL_SECTION) {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn LeaveCriticalSection(lpCriticalSection: LPCRITICAL_SECTION) {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn DeleteCriticalSection(lpCriticalSection: LPCRITICAL_SECTION) {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn WaitForSingleObject(hHandle: HANDLE, dwMilliseconds: DWORD) -> DWORD {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn CreateMutex(
    lpMutexAttributes: *mut ::std::os::raw::c_void,
    bInitialOwner: BOOL,
    lpName: LPCSTR,
) -> HANDLE {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ReleaseMutex(hMutex: HANDLE) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn CloseHandle(hObject: HANDLE) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetCurrentThreadId() -> DWORD {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn TerminateThread(hThread: HANDLE, dwExitCode: DWORD) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn _beginthread(
    start_address: ::std::option::Option<unsafe extern "C" fn(arg1: *mut ::std::os::raw::c_void)>,
    stack_size: ::std::os::raw::c_uint,
    arglist: *mut ::std::os::raw::c_void,
) -> usize {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn SetThreadPriority(
    hThread: HANDLE,
    nPriority: ::std::os::raw::c_int,
) -> BOOL {
    todo!()
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn CreateEvent(
    lpEventAttributes: *mut ::std::os::raw::c_void,
    bManualReset: BOOL,
    bInitialState: BOOL,
    lpName: LPCSTR,
) -> HANDLE {
    todo!()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ShowWindow(hWnd: HWND, nCmdShow: ::std::os::raw::c_int) -> BOOL {
    todo!()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn SetWindowPos(
    hWnd: HWND,
    hWndInsertAfter: HWND,
    X: ::std::os::raw::c_int,
    Y: ::std::os::raw::c_int,
    cx: ::std::os::raw::c_int,
    cy: ::std::os::raw::c_int,
    uFlags: UINT,
) -> BOOL {
    todo!()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn MessageBox(
    hWnd: HWND,
    lpText: LPCTSTR,
    lpCaption: LPCTSTR,
    uType: UINT,
) -> ::std::os::raw::c_int {
    todo!()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn DebugBreak() {
    todo!()
}
