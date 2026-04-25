// ctl_error.rs
use thiserror::Error;

#[derive(Debug, Error)]
pub enum SysctlError {
    #[error("no such sysctl: {0}")]
    NotFound(String),

    #[error("no matching type for value")]
    #[cfg(not(any(target_os = "macos", target_os = "ios")))]
    UnknownType,

    #[error("Error extracting value")]
    ExtractionError,

    #[error("Error parsing value")]
    ParseError,

    #[error("Support for type not implemented")]
    MissingImplementation,

    #[error("IO Error: {0}")]
    IoError(#[from] std::io::Error),

    #[error("Error parsing UTF-8 data: {0}")]
    Utf8Error(#[from] std::str::Utf8Error),

    #[error("Value is not readable")]
    NoReadAccess,

    #[error("Value is not writeable")]
    NoWriteAccess,

    #[error("Not supported by this platform")]
    NotSupported,

    #[error(
        "sysctl returned a short read: read {read} bytes, while a size of {reported} was reported"
    )]
    ShortRead { read: usize, reported: usize },

    #[error("Error reading C String: String was not NUL-terminated.")]
    InvalidCStr(#[from] std::ffi::FromBytesWithNulError),

    #[error("Error Rust string contains nul bytes")]
    InvalidCString(#[from] std::ffi::NulError),
}
