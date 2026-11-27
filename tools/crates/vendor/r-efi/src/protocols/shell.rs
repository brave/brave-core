//! Shell Protocol
//!
//! Provides shell services to UEFI applications.

pub const PROTOCOL_GUID: crate::base::Guid = crate::base::Guid::from_fields(
    0x6302d008,
    0x7f9b,
    0x4f30,
    0x87,
    0xac,
    &[0x60, 0xc9, 0xfe, 0xf5, 0xda, 0x4e],
);

pub const MAJOR_VERSION: u32 = 0x00000002;
pub const MINOR_VERSION: u32 = 0x00000002;

pub type FileHandle = *mut core::ffi::c_void;

pub type DeviceNameFlags = u32;
pub const DEVICE_NAME_USE_COMPONENT_NAME: DeviceNameFlags = 0x00000001;
pub const DEVICE_NAME_USE_DEVICE_PATH: DeviceNameFlags = 0x00000002;

#[repr(C)]
pub struct ListEntry {
    pub flink: *mut ListEntry,
    pub blink: *mut ListEntry,
}

#[repr(C)]
pub struct FileInfo {
    pub link: ListEntry,
    pub status: crate::base::Status,
    pub full_name: *mut crate::base::Char16,
    pub file_name: *mut crate::base::Char16,
    pub handle: FileHandle,
    pub info: *mut crate::protocols::file::Info,
}

pub type Execute = unsafe extern "efiapi" fn(
    *mut crate::base::Handle,
    *mut crate::base::Char16,
    *mut *mut crate::base::Char16,
    *mut crate::base::Status,
) -> crate::base::Status;

pub type GetEnv = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
) -> *mut crate::base::Char16;

pub type SetEnv = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
    *mut crate::base::Char16,
    crate::base::Boolean,
) -> crate::base::Status;

pub type GetAlias = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
    *mut crate::base::Boolean,
) -> *mut crate::base::Char16;

pub type SetAlias = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
    *mut crate::base::Char16,
    crate::base::Boolean,
    crate::base::Boolean,
) -> crate::base::Status;

pub type GetHelpText = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
    *mut crate::base::Char16,
    *mut *mut crate::base::Char16,
) -> crate::base::Status;

pub type GetDevicePathFromMap = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
) -> *mut crate::protocols::device_path::Protocol;

pub type GetMapFromDevicePath = unsafe extern "efiapi" fn(
    *mut *mut crate::protocols::device_path::Protocol,
) -> *mut crate::base::Char16;

pub type GetDevicePathFromFilePath = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
) -> *mut crate::protocols::device_path::Protocol;

pub type GetFilePathFromDevicePath = unsafe extern "efiapi" fn(
    *mut crate::protocols::device_path::Protocol,
) -> *mut crate::base::Char16;

pub type SetMap = unsafe extern "efiapi" fn(
    *mut crate::protocols::device_path::Protocol,
    *mut crate::base::Char16,
) -> crate::base::Status;

pub type GetCurDir = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
) -> *mut crate::base::Char16;

pub type SetCurDir = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
    *mut crate::base::Char16,
) -> crate::base::Status;

pub type OpenFileList = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
    u64,
    *mut *mut FileInfo,
) -> crate::base::Status;

pub type FreeFileList = unsafe extern "efiapi" fn(
    *mut *mut FileInfo,
) -> crate::base::Status;

pub type RemoveDupInFileList = unsafe extern "efiapi" fn(
    *mut *mut FileInfo,
) -> crate::base::Status;

pub type BatchIsActive = unsafe extern "efiapi" fn() -> crate::base::Boolean;

pub type IsRootShell = unsafe extern "efiapi" fn() -> crate::base::Boolean;

pub type EnablePageBreak = unsafe extern "efiapi" fn();

pub type DisablePageBreak = unsafe extern "efiapi" fn();

pub type GetPageBreak = unsafe extern "efiapi" fn() -> crate::base::Boolean;

pub type GetDeviceName = unsafe extern "efiapi" fn(
    crate::base::Handle,
    DeviceNameFlags,
    *mut crate::base::Char8,
    *mut *mut crate::base::Char16,
) -> crate::base::Status;

pub type GetFileInfo = unsafe extern "efiapi" fn(
    FileHandle,
) -> *mut crate::protocols::file::Info;

pub type SetFileInfo = unsafe extern "efiapi" fn(
    FileHandle,
    *mut crate::protocols::file::Info
) -> crate::base::Status;

pub type OpenFileByName = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
    *mut FileHandle,
    u64,
) -> crate::base::Status;

pub type CloseFile = unsafe extern "efiapi" fn(
    FileHandle,
) -> crate::base::Status;

pub type CreateFile = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
    u64,
    *mut FileHandle,
) -> crate::base::Status;

pub type ReadFile = unsafe extern "efiapi" fn(
    FileHandle,
    *mut usize,
    *mut core::ffi::c_void,
) -> crate::base::Status;

pub type WriteFile = unsafe extern "efiapi" fn(
    FileHandle,
    *mut usize,
    *mut core::ffi::c_void,
) -> crate::base::Status;

pub type DeleteFile = unsafe extern "efiapi" fn(
    FileHandle,
) -> crate::base::Status;

pub type DeleteFileByName = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
) -> crate::base::Status;

pub type GetFilePosition = unsafe extern "efiapi" fn(
    FileHandle,
    *mut u64,
) -> crate::base::Status;

pub type SetFilePosition = unsafe extern "efiapi" fn(
    FileHandle,
    u64,
) -> crate::base::Status;

pub type FlushFile = unsafe extern "efiapi" fn(
    FileHandle,
) -> crate::base::Status;

pub type FindFiles = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
    *mut *mut FileInfo,
) -> crate::base::Status;

pub type FindFilesInDir = unsafe extern "efiapi" fn(
    FileHandle,
    *mut *mut FileInfo,
) -> crate::base::Status;

pub type GetFileSize = unsafe extern "efiapi" fn(
    FileHandle,
    *mut u64,
) -> crate::base::Status;

pub type OpenRoot = unsafe extern "efiapi" fn(
    *mut crate::protocols::device_path::Protocol,
    *mut FileHandle,
) -> crate::base::Status;

pub type OpenRootByHandle = unsafe extern "efiapi" fn(
    crate::base::Handle,
    *mut FileHandle,
) -> crate::base::Status;

pub type RegisterGuidName = unsafe extern "efiapi" fn(
    *mut crate::base::Guid,
    *mut crate::base::Char16,
) -> crate::base::Status;

pub type GetGuidName = unsafe extern "efiapi" fn(
    *mut crate::base::Guid,
    *mut *mut crate::base::Char16,
) -> crate::base::Status;

pub type GetGuidFromName = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
    *mut crate::base::Guid,
) -> crate::base::Status;

pub type GetEnvEx = unsafe extern "efiapi" fn(
    *mut crate::base::Char16,
    *mut u32,
) -> *mut crate::base::Char16;

#[repr(C)]
pub struct Protocol {
    pub execute: Execute,
    pub get_env: GetEnv,
    pub set_env: SetEnv,
    pub get_alias: GetAlias,
    pub set_alias: SetAlias,
    pub get_help_text: GetHelpText,
    pub get_device_path_from_map: GetDevicePathFromMap,
    pub get_map_from_device_path: GetMapFromDevicePath,
    pub get_device_path_from_file_path: GetDevicePathFromFilePath,
    pub get_file_path_from_device_path: GetFilePathFromDevicePath,
    pub set_map: SetMap,

    pub get_cur_dir: GetCurDir,
    pub set_cur_dir: SetCurDir,
    pub open_file_list: OpenFileList,
    pub free_file_list: FreeFileList,
    pub remove_dup_in_file_list: RemoveDupInFileList,

    pub batch_is_active: BatchIsActive,
    pub is_root_shell: IsRootShell,
    pub enable_page_break: EnablePageBreak,
    pub disable_page_break: DisablePageBreak,
    pub get_page_break: GetPageBreak,
    pub get_device_name: GetDeviceName,

    pub get_file_info: GetFileInfo,
    pub set_file_info: SetFileInfo,
    pub open_file_by_name: OpenFileByName,
    pub close_file: CloseFile,
    pub create_file: CreateFile,
    pub read_file: ReadFile,
    pub write_file: WriteFile,
    pub delete_file: DeleteFile,
    pub delete_file_by_name: DeleteFileByName,
    pub get_file_position: GetFilePosition,
    pub set_file_position: SetFilePosition,
    pub flush_file: FlushFile,
    pub find_files: FindFiles,
    pub find_files_in_dir: FindFilesInDir,
    pub get_file_size: GetFileSize,

    pub open_root: OpenRoot,
    pub open_root_by_handle: OpenRootByHandle,

    pub execution_break: crate::base::Event,

    pub major_version: u32,
    pub minor_version: u32,
    pub register_guid_name: RegisterGuidName,
    pub get_guid_name: GetGuidName,
    pub get_guid_from_name: GetGuidFromName,

    // Shell 2.1
    pub get_env_ex: GetEnvEx,
}
