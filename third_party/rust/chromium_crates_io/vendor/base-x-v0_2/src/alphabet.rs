#[cfg(not(feature = "std"))]
use alloc::{string::String, vec::Vec};
use DecodeError;

use decoder::*;
use encoder;

pub trait Alphabet {
    fn encode(self, input: &[u8]) -> String;

    fn decode(self, input: &str) -> Result<Vec<u8>, DecodeError>;
}

impl<'a> Alphabet for &[u8] {
    #[inline(always)]
    fn encode(self, input: &[u8]) -> String {
        if !self.is_ascii() {
            panic!("Alphabet must be ASCII");
        }

        let mut out = encoder::encode(self, input);
        out.reverse();
        unsafe { String::from_utf8_unchecked(out) }
    }

    #[inline(always)]
    fn decode(self, input: &str) -> Result<Vec<u8>, DecodeError> {
        U8Decoder::new(self).decode(input)
    }
}

impl<'a> Alphabet for &str {
    #[inline(always)]
    fn encode(self, input: &[u8]) -> String {
        if self.is_ascii() {
            let mut out = encoder::encode(self.as_bytes(), input);
            out.reverse();
            unsafe { String::from_utf8_unchecked(out) }
        } else {
            let alphabet: Vec<char> = self.chars().collect();
            let out = encoder::encode(&alphabet, input);
            out.iter().rev().collect()
        }
    }

    #[inline(always)]
    fn decode(self, input: &str) -> Result<Vec<u8>, DecodeError> {
        if self.is_ascii() {
            U8Decoder::new(self.as_bytes()).decode(input)
        } else {
            let alphabet: Vec<char> = self.chars().collect();
            CharDecoder(&alphabet).decode(input)
        }
    }
}
