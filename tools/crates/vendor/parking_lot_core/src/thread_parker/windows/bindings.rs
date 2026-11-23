#![allow(non_snake_case)]

//! Manual bindings to the small subset of the Win32 API used by this module.
//!
//! Keeping these stable system interfaces here avoids adding `windows-sys` or
//! `winapi` as a dependency of the foundational `parking_lot_core` crate.

pub const INFINITE: u32 = 4294967295;
pub const ERROR_TIMEOUT: u32 = 1460;
pub const GENERIC_READ: u32 = 2147483648;
pub const GENERIC_WRITE: u32 = 1073741824;
pub const STATUS_SUCCESS: i32 = 0;
pub const STATUS_TIMEOUT: i32 = 258;

pub type HANDLE = *mut std::ffi::c_void;
pub type HINSTANCE = *mut std::ffi::c_void;
pub type BOOL = i32;
pub type BOOLEAN = u8;
pub type NTSTATUS = i32;
pub type FARPROC = Option<unsafe extern "system" fn() -> isize>;
pub type NtCreateKeyedEvent = unsafe extern "system" fn(
    KeyedEventHandle: *mut HANDLE,
    DesiredAccess: u32,
    ObjectAttributes: *mut std::ffi::c_void,
    Flags: u32,
) -> NTSTATUS;
pub type NtReleaseKeyedEvent = unsafe extern "system" fn(
    EventHandle: HANDLE,
    Key: *mut std::ffi::c_void,
    Alertable: BOOLEAN,
    Timeout: *mut i64,
) -> NTSTATUS;
pub type NtWaitForKeyedEvent = unsafe extern "system" fn(
    EventHandle: HANDLE,
    Key: *mut std::ffi::c_void,
    Alertable: BOOLEAN,
    Timeout: *mut i64,
) -> NTSTATUS;
pub type WaitOnAddress = unsafe extern "system" fn(
    Address: *const std::ffi::c_void,
    CompareAddress: *const std::ffi::c_void,
    AddressSize: usize,
    dwMilliseconds: u32,
) -> BOOL;
pub type WakeByAddressSingle = unsafe extern "system" fn(Address: *const std::ffi::c_void);

windows_link::link!("kernel32.dll" "system" fn GetLastError() -> u32);
windows_link::link!("kernel32.dll" "system" fn CloseHandle(hObject: HANDLE) -> BOOL);
windows_link::link!("kernel32.dll" "system" fn GetModuleHandleA(lpModuleName: *const u8) -> HINSTANCE);
windows_link::link!("kernel32.dll" "system" fn GetProcAddress(hModule: HINSTANCE, lpProcName: *const u8) -> FARPROC);
windows_link::link!("kernel32.dll" "system" fn Sleep(dwMilliseconds: u32) -> ());
