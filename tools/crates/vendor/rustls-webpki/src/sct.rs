use core::fmt;
use core::marker::PhantomData;

use untrusted::{Input, Reader};

pub(crate) struct SctParser<'a> {
    reader: Reader<'a>,
    done: bool,
}

impl<'a> SctParser<'a> {
    pub(crate) fn new(input: Option<Input<'a>>) -> Result<Self, Error> {
        Ok(SctParser {
            reader: match input {
                Some(input) => Reader::new(
                    input.read_all(Error::MalformedSct, |rd| read_field(rd, non_zero_u16_len))?,
                ),
                None => Reader::new(Input::from(&[])),
            },
            done: false,
        })
    }
}

impl<'a> Iterator for SctParser<'a> {
    type Item = Result<SignedCertificateTimestamp<'a>, Error>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.done || self.reader.at_end() {
            return None;
        }

        let result = read_field(&mut self.reader, non_zero_u16_len)
            .and_then(|item| SignedCertificateTimestamp::try_from(item.as_slice_less_safe()));
        match result {
            Err(Error::UnsupportedSctVersion) | Ok(_) => {}
            Err(_) => self.done = true,
        }
        Some(result)
    }
}

/// This is `SignedCertificateTimestamp` defined in [RFC6962][].
///
/// [RFC6962]: https://www.rfc-editor.org/rfc/rfc6962.html#section-3.2
#[derive(Debug, PartialEq)]
pub(crate) struct SignedCertificateTimestamp<'a> {
    log_id: [u8; 32],
    timestamp_ms: u64,
    _future_lifetime_for_signature: PhantomData<&'a ()>,
}

impl SignedCertificateTimestamp<'_> {
    pub(crate) fn log_id_and_timestamp(&self) -> LogIdAndTimestamp {
        LogIdAndTimestamp {
            log_id: self.log_id,
            timestamp_ms: self.timestamp_ms,
        }
    }
}

impl<'a> TryFrom<&'a [u8]> for SignedCertificateTimestamp<'a> {
    type Error = Error;

    fn try_from(bytes: &'a [u8]) -> Result<Self, Self::Error> {
        let input = Input::from(bytes);
        input.read_all(Error::MalformedSct, |rd| {
            match read_array(rd)? {
                [0] => {}
                _ => return Err(Error::UnsupportedSctVersion),
            };

            let log_id = read_array(rd)?;
            let timestamp_ms = u64::from_be_bytes(read_array(rd)?);
            let _extensions = read_field(rd, any_u16_len)?;
            let _signature_algorithm = u16::from_be_bytes(read_array(rd)?);
            let _signature = read_field(rd, non_zero_u16_len)?;

            Ok(SignedCertificateTimestamp {
                log_id,
                timestamp_ms,
                _future_lifetime_for_signature: PhantomData,
            })
        })
    }
}

/// The certificate transparency log ID and associated inclusion timestamp.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct LogIdAndTimestamp {
    /// Log ID
    pub log_id: [u8; 32],
    /// Inclusion timestamp in milliseconds.
    pub timestamp_ms: u64,
}

/// Read a length-prefixed field from `rd`.
///
/// The length is encoded in `N` bytes and those bytes are decoded by `into_size`.
fn read_field<'a, const N: usize>(
    rd: &mut Reader<'a>,
    into_size: fn([u8; N]) -> Result<usize, Error>,
) -> Result<Input<'a>, Error> {
    let len = into_size(read_array::<N>(rd)?)?;
    rd.read_bytes(len).map_err(|_| Error::MalformedSct)
}

/// Read `N` bytes from `rd` as an array.
fn read_array<const N: usize>(rd: &mut Reader<'_>) -> Result<[u8; N], Error> {
    rd.read_bytes(N)
        .map_err(|_| Error::MalformedSct)?
        .as_slice_less_safe()
        .try_into()
        .map_err(|_| Error::MalformedSct)
}

/// Converts two bytes into a usize.
///
/// Does not fail.
fn any_u16_len(bytes: [u8; 2]) -> Result<usize, Error> {
    Ok(usize::from(u16::from_be_bytes(bytes)))
}

/// Converts two bytes into a usize, erroring if the result is zero.
fn non_zero_u16_len(bytes: [u8; 2]) -> Result<usize, Error> {
    match any_u16_len(bytes)? {
        0 => Err(Error::MalformedSct),
        len => Ok(len),
    }
}

/// Possible errors from SCT parsing and processing
#[derive(Clone, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum Error {
    /// The SCT was somehow misencoded, truncated or otherwise corrupt.
    MalformedSct,

    /// An unsupported SCT version was encountered.
    ///
    /// This library only supports `v1(0)`.
    UnsupportedSctVersion,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::MalformedSct => write!(f, "malformed SCT"),
            Self::UnsupportedSctVersion => write!(f, "unsupported SCT version"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}

#[cfg(test)]
mod tests {
    use alloc::vec::Vec;

    use super::*;

    #[test]
    fn absent_sct_sequence() {
        assert!(SctParser::new(None).unwrap().next().is_none());
    }

    #[test]
    fn empty_sct_sequence() {
        assert_eq!(
            Some(Error::MalformedSct),
            SctParser::new(Some(Input::from(&[]))).err()
        );
    }

    #[test]
    fn truncated_sct_length_in_sequence() {
        assert_eq!(
            Some(Error::MalformedSct),
            SctParser::new(Some(Input::from(&[0]))).err()
        );
    }

    #[test]
    fn empty_sct_in_sequence() {
        assert_eq!(
            Some(Error::MalformedSct),
            SctParser::new(Some(Input::from(&[0, 0]))).err()
        );
    }

    #[test]
    fn truncated_sct_in_sequence() {
        assert_eq!(
            Some(Error::MalformedSct),
            SctParser::new(Some(Input::from(&[0, 1]))).err()
        );
    }

    #[test]
    fn sample_sct() {
        let bytes = encode_sct_list([TestSct::default().encode()]);

        SignedCertificateTimestamp::try_from(&bytes[4..]).unwrap();

        assert_eq!(
            SignedCertificateTimestamp {
                log_id: [b'l'; 32],
                timestamp_ms: 0x74747474_74747474,
                _future_lifetime_for_signature: PhantomData
            },
            SctParser::new(Some(Input::from(&bytes)))
                .unwrap()
                .next()
                .unwrap()
                .unwrap()
        );
    }

    #[test]
    fn terminate_iterator_after_error_within_sct() {
        let bytes = encode_sct_list([TestSct {
            signature: Vec::new(),
            ..Default::default()
        }
        .encode()]);

        SignedCertificateTimestamp::try_from(&bytes[4..]).unwrap_err();

        let mut iter = SctParser::new(Some(Input::from(&bytes))).unwrap();
        assert_eq!(iter.next(), Some(Err(Error::MalformedSct)));
        assert_eq!(iter.next(), None);
    }

    #[test]
    fn continue_iterator_after_unsupported_version() {
        let bytes = encode_sct_list([
            TestSct::default().encode(),
            vec![
                0x1, // unsupported SCT version
                0x7, 0x0, 0x0, 0x1, b'?', // version-specific contents
            ],
            TestSct {
                log_id: [b'm'; 32],
                timestamp_ms: u64::from_be_bytes([b'u'; 8]),
                ..Default::default()
            }
            .encode(),
        ]);

        let mut iter = SctParser::new(Some(Input::from(&bytes))).unwrap();
        assert_eq!(
            iter.next().unwrap().unwrap().log_id_and_timestamp(),
            LogIdAndTimestamp {
                log_id: [b'l'; 32],
                timestamp_ms: 0x74747474_74747474,
            }
        );
        assert_eq!(iter.next(), Some(Err(Error::UnsupportedSctVersion)));
        assert_eq!(
            iter.next().unwrap().unwrap().log_id_and_timestamp(),
            LogIdAndTimestamp {
                log_id: [b'm'; 32],
                timestamp_ms: 0x75757575_75757575,
            }
        );
        assert_eq!(iter.next(), None);
    }

    #[test]
    fn illegal_empty_signature() {
        let bytes = TestSct {
            signature: Vec::new(),
            ..Default::default()
        }
        .encode();

        assert_eq!(
            Some(Error::MalformedSct),
            SignedCertificateTimestamp::try_from(&bytes[..]).err()
        );
    }

    #[test]
    fn illegal_unknown_version() {
        let bytes = TestSct {
            version: 1,
            ..Default::default()
        }
        .encode();

        assert_eq!(
            Some(Error::UnsupportedSctVersion),
            SignedCertificateTimestamp::try_from(&bytes[..]).err()
        );
    }

    #[test]
    fn illegal_trailing_extension_data() {
        let bytes = [
            0x0, 0x1,  // extension outer length
            b'?', // doesn't matter
            b'x', // unexpected trailing data
        ];

        assert_eq!(
            Some(Error::MalformedSct),
            SctParser::new(Some(Input::from(&bytes))).err()
        );
    }

    struct TestSct {
        version: u8,
        log_id: [u8; 32],
        timestamp_ms: u64,
        extensions: Vec<u8>,
        signature_algorithm: [u8; 2],
        signature: Vec<u8>,
    }

    impl TestSct {
        fn encode(self) -> Vec<u8> {
            let mut encoded = vec![self.version];
            encoded.extend_from_slice(&self.log_id);
            encoded.extend_from_slice(&self.timestamp_ms.to_be_bytes());
            encoded.extend(u16_length_prefixed(self.extensions));
            encoded.extend_from_slice(&self.signature_algorithm);
            encoded.extend(u16_length_prefixed(self.signature));
            encoded
        }
    }

    impl Default for TestSct {
        fn default() -> Self {
            Self {
                version: 0,
                log_id: [b'l'; 32],
                timestamp_ms: u64::from_be_bytes([b't'; 8]),
                extensions: Vec::new(),
                signature_algorithm: *b"sa",
                signature: vec![b's'],
            }
        }
    }

    fn encode_sct_list(scts: impl IntoIterator<Item = Vec<u8>>) -> Vec<u8> {
        let mut encoded = Vec::new();
        for sct in scts {
            encoded.extend(u16_length_prefixed(sct));
        }
        u16_length_prefixed(encoded)
    }

    fn u16_length_prefixed(body: Vec<u8>) -> Vec<u8> {
        let length = u16::try_from(body.len()).unwrap();
        let mut encoded = Vec::with_capacity(2 + body.len());
        encoded.extend_from_slice(&length.to_be_bytes());
        encoded.extend(body);
        encoded
    }
}
