//! A module to handle all errors

/// An error enum
#[derive(Debug)]
pub enum Error {
    /// Io error
    #[cfg(feature = "std")]
    Io(std::io::Error),
    /// Io error
    #[cfg(not(feature = "std"))]
    Io,
    /// Utf8 Error
    Utf8(::core::str::Utf8Error),
    /// Deprecated feature (in protocol buffer specification)
    Deprecated(&'static str),
    /// Unknown wire type
    UnknownWireType(u8),
    /// Varint decoding error
    Varint,
    /// Error while parsing protocol buffer message
    #[cfg(feature = "std")]
    Message(String),
    /// Unexpected map tag
    Map(u8),
    /// Out of data when reading from or writing to a byte buffer
    UnexpectedEndOfBuffer,
    /// The supplied output buffer is not large enough to serialize the message
    OutputBufferTooSmall,
}

/// A wrapper for `Result<T, Error>`
pub type Result<T> = ::core::result::Result<T, Error>;

#[cfg(feature = "std")]
impl Into<std::io::Error> for Error {
    fn into(self) -> ::std::io::Error {
        match self {
            Error::Io(x) => x,
            Error::Utf8(x) => std::io::Error::new(std::io::ErrorKind::InvalidData, x),
            x => std::io::Error::new(std::io::ErrorKind::Other, x),
        }
    }
}

#[cfg(feature = "std")]
impl From<std::io::Error> for Error {
    fn from(e: std::io::Error) -> Error {
        Error::Io(e)
    }
}

impl From<::core::str::Utf8Error> for Error {
    fn from(e: ::core::str::Utf8Error) -> Error {
        Error::Utf8(e)
    }
}

#[cfg(feature = "std")]
impl std::error::Error for Error {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            Error::Io(e) => Some(e),
            Error::Utf8(e) => Some(e),
            _ => None,
        }
    }
}

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        match self {
            #[cfg(feature = "std")]
            Error::Io(e) => write!(f, "{}", e),
            #[cfg(not(feature = "std"))]
            Error::Io => write!(f, "IO error"),
            Error::Utf8(e) => write!(f, "{}", e),
            Error::Deprecated(feature) => write!(f, "Feature '{}' has been deprecated", feature),
            Error::UnknownWireType(e) => {
                write!(f, "Unknown wire type '{}', must be less than 6", e)
            }
            Error::Varint => write!(f, "Cannot decode varint"),
            #[cfg(feature = "std")]
            Error::Message(msg) => write!(f, "Error while parsing message: {}", msg),
            Error::Map(tag) => write!(f, "Unexpected map tag: '{}', expecting 1 or 2", tag),
            Error::UnexpectedEndOfBuffer => write!(f, "Unexpected end of buffer"),
            Error::OutputBufferTooSmall => write!(f, "Output buffer too small"),
        }
    }
}
