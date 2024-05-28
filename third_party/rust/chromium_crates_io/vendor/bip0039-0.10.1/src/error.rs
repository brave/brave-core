#[cfg(not(feature = "std"))]
use alloc::string::String;
use core::fmt;

/// The BIP-0039 error.
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Error {
    /// Mnemonic only support 12/15/18/21/24 words.
    BadWordCount(usize),
    /// Entropy was not a multiple of 32 bits or between 128-256n bits in length.
    BadEntropyBitCount(usize),
    /// Mnemonic contains an unknown word.
    UnknownWord(String),
    /// The mnemonic has an invalid checksum.
    InvalidChecksum,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::BadWordCount(count) => write!(
                f,
                "BIP-0039 mnemonic only supports 12/15/18/21/24 words: {}",
                count
            ),
            Error::BadEntropyBitCount(count) => write!(
                f,
                "entropy was not between 128-256 bits or not a multiple of 32 bits: {} bits",
                count
            ),
            Error::UnknownWord(word) => write!(f, "mnemonic contains an unknown word: {}", word),
            Error::InvalidChecksum => write!(f, "mnemonic has an invalid checksum"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}
