#[derive(Debug)]
pub enum Error {
    NoAuditData,
    InputLimitExceeded,
    OutputLimitExceeded,
    Io(std::io::Error),
    BinaryParsing(auditable_extract::Error),
    Decompression(DecompressError),
    #[cfg(feature = "serde")]
    Json(serde_json::Error),
    Utf8(std::str::Utf8Error),
}

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Error::NoAuditData => write!(f, "No audit data found in the binary! Was it built with 'cargo auditable'?"),
            Error::InputLimitExceeded => write!(f, "The input file is too large. Increase the input size limit to scan it."),
            Error::OutputLimitExceeded => write!(f, "Audit data size is over the specified limit. Increase the output size limit to scan it."),
            Error::Io(e) => write!(f, "Failed to read the binary: {e}"),
            Error::BinaryParsing(e) => write!(f, "Failed to parse the binary: {e}"),
            Error::Decompression(e) => write!(f, "Failed to decompress audit data: {e}"),
            #[cfg(feature = "serde")]
            Error::Json(e) => write!(f, "Failed to deserialize audit data from JSON: {e}"),
            Error::Utf8(e) => write!(f, "Invalid UTF-8 in audit data: {e}"),
        }
    }
}

impl std::error::Error for Error {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            Error::NoAuditData => None,
            Error::InputLimitExceeded => None,
            Error::OutputLimitExceeded => None,
            Error::Io(e) => Some(e),
            Error::BinaryParsing(e) => Some(e),
            Error::Decompression(e) => Some(e),
            #[cfg(feature = "serde")]
            Error::Json(e) => Some(e),
            Error::Utf8(e) => Some(e),
        }
    }
}

impl From<std::io::Error> for Error {
    fn from(e: std::io::Error) -> Self {
        Self::Io(e)
    }
}

impl From<auditable_extract::Error> for Error {
    fn from(e: auditable_extract::Error) -> Self {
        match e {
            auditable_extract::Error::NoAuditData => Error::NoAuditData,
            other_err => Self::BinaryParsing(other_err),
        }
    }
}

impl From<DecompressError> for Error {
    fn from(e: DecompressError) -> Self {
        match e.status {
            TINFLStatus::HasMoreOutput => Error::OutputLimitExceeded,
            _ => Error::Decompression(e),
        }
    }
}

impl From<std::string::FromUtf8Error> for Error {
    fn from(e: std::string::FromUtf8Error) -> Self {
        Self::Utf8(e.utf8_error())
    }
}

#[cfg(feature = "serde")]
impl From<serde_json::Error> for Error {
    fn from(e: serde_json::Error) -> Self {
        Self::Json(e)
    }
}

/// A copy of [miniz_oxide::inflate::DecompressError].
///
/// We use our copy instead of the miniz_oxide type directly
/// so that we don't have to bump semver every time `miniz_oxide` does.
#[derive(Debug)]
pub struct DecompressError {
    /// Decompressor status on failure. See [TINFLStatus] for details.
    pub status: TINFLStatus,
    /// The currently decompressed data if any.
    pub output: Vec<u8>,
}

impl std::fmt::Display for DecompressError {
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        f.write_str(match self.status {
            TINFLStatus::FailedCannotMakeProgress => "Truncated input stream",
            TINFLStatus::BadParam => "Invalid output buffer size",
            TINFLStatus::Adler32Mismatch => "Adler32 checksum mismatch",
            TINFLStatus::Failed => "Invalid input data",
            TINFLStatus::Done => unreachable!(),
            TINFLStatus::NeedsMoreInput => "Truncated input stream",
            TINFLStatus::HasMoreOutput => "Output size exceeded the specified limit",
        })
    }
}

impl std::error::Error for DecompressError {}

impl DecompressError {
    pub(crate) fn from_miniz(err: miniz_oxide::inflate::DecompressError) -> Self {
        Self {
            status: TINFLStatus::from_miniz(err.status),
            output: err.output,
        }
    }
}

#[repr(i8)]
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub enum TINFLStatus {
    FailedCannotMakeProgress,
    BadParam,
    Adler32Mismatch,
    Failed,
    Done,
    NeedsMoreInput,
    HasMoreOutput,
}

impl TINFLStatus {
    pub(crate) fn from_miniz(status: miniz_oxide::inflate::TINFLStatus) -> Self {
        use miniz_oxide::inflate;
        match status {
            inflate::TINFLStatus::FailedCannotMakeProgress => Self::FailedCannotMakeProgress,
            inflate::TINFLStatus::BadParam => Self::BadParam,
            inflate::TINFLStatus::Adler32Mismatch => Self::Adler32Mismatch,
            inflate::TINFLStatus::Failed => Self::Failed,
            inflate::TINFLStatus::Done => Self::Done,
            inflate::TINFLStatus::NeedsMoreInput => Self::NeedsMoreInput,
            inflate::TINFLStatus::HasMoreOutput => Self::HasMoreOutput,
        }
    }
}
