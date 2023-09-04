#[cfg(not(feature = "std"))]
use alloc::vec::Vec;

#[cfg(not(feature = "std"))]
use core as std;

use std::{ptr, u32};

/// This is a pretty naive implementation of a BigUint abstracting all
/// math out to a vector of `u32` chunks.
///
/// It can only do a few things:
/// - Be instantiated from an arbitrary big-endian byte slice
/// - Be converted to a vector of big-endian bytes.
/// - Do a division by `u32`, mutating self and returning the remainder.
/// - Do a multiplication with addition in one pass.
/// - Check if it's zero.
///
/// Turns out those are all the operations you need to encode and decode
/// base58, or anything else, really.
pub struct BigUint {
    pub chunks: Vec<u32>,
}

impl BigUint {
    #[inline]
    pub fn with_capacity(capacity: usize) -> Self {
        let mut chunks = Vec::with_capacity(capacity);

        chunks.push(0);

        BigUint { chunks }
    }

    /// Divide self by `divider`, return the remainder of the operation.
    #[inline]
    pub fn div_mod(&mut self, divider: u32) -> u32 {
        let mut carry = 0u64;

        for chunk in self.chunks.iter_mut() {
            carry = (carry << 32) | u64::from(*chunk);
            *chunk = (carry / u64::from(divider)) as u32;
            carry %= u64::from(divider);
        }

        if let Some(0) = self.chunks.get(0) {
            self.chunks.remove(0);
        }

        carry as u32
    }

    /// Perform a multiplication followed by addition. This is a reverse
    /// of `div_mod` in the sense that when supplied remained for addition
    /// and the same base for multiplication as divison, the result is
    /// the original BigUint.
    #[inline]
    pub fn mul_add(&mut self, multiplicator: u32, addition: u32) {
        let mut carry = 0u64;

        {
            let mut iter = self.chunks.iter_mut().rev();

            if let Some(chunk) = iter.next() {
                carry = u64::from(*chunk) * u64::from(multiplicator) + u64::from(addition);
                *chunk = carry as u32;
                carry >>= 32;
            }

            for chunk in iter {
                carry += u64::from(*chunk) * u64::from(multiplicator);
                *chunk = carry as u32;
                carry >>= 32;
            }
        }

        if carry > 0 {
            self.chunks.insert(0, carry as u32);
        }
    }

    /// Check if self is zero.
    #[inline]
    pub fn is_zero(&self) -> bool {
        self.chunks.iter().all(|chunk| *chunk == 0)
    }

    #[inline]
    pub fn into_bytes_be(mut self) -> Vec<u8> {
        let mut skip = 0;

        for chunk in self.chunks.iter() {
            if *chunk != 0 {
                skip += chunk.leading_zeros() / 8;
                break;
            }

            skip += 4;
        }

        let len = self.chunks.len() * 4 - skip as usize;

        if len == 0 {
            return Vec::new();
        }

        for chunk in self.chunks.iter_mut() {
            *chunk = u32::to_be(*chunk);
        }

        let mut bytes = Vec::with_capacity(len);
        unsafe {
            bytes.set_len(len);

            let chunks_ptr = (self.chunks.as_ptr() as *const u8).offset(skip as isize);

            ptr::copy_nonoverlapping(chunks_ptr, bytes.as_mut_ptr(), len);

        }
            bytes
    }

    #[inline]
    pub fn from_bytes_be(bytes: &[u8]) -> Self {
        let modulo = bytes.len() % 4;

        let len = bytes.len() / 4 + (modulo > 0) as usize;

        let mut chunks = Vec::with_capacity(len);

        unsafe {
            chunks.set_len(len);

            let mut chunks_ptr = chunks.as_mut_ptr() as *mut u8;

            if modulo > 0 {
                *chunks.get_unchecked_mut(0) = 0u32;
                chunks_ptr = chunks_ptr.offset(4 - modulo as isize);
            }

            ptr::copy_nonoverlapping(bytes.as_ptr(), chunks_ptr, bytes.len());
        }

        for chunk in chunks.iter_mut() {
            *chunk = u32::from_be(*chunk);
        }

        BigUint { chunks }
    }
}

#[cfg(test)]
mod tests {
    #![allow(clippy::unreadable_literal)]
    use super::BigUint;

    #[test]
    fn big_uint_from_bytes() {
        let bytes: &[u8] = &[
            0xDE, 0xAD, 0x00, 0x00, 0x00, 0x13, 0x37, 0xAD, 0x00, 0x00, 0x00, 0x00, 0xDE, 0xAD,
        ];

        let big = BigUint::from_bytes_be(bytes);

        assert_eq!(
            big.chunks,
            vec![0x0000DEAD, 0x00000013, 0x37AD0000, 0x0000DEAD]
        );
    }

    #[test]
    fn big_uint_rem_div() {
        let mut big = BigUint {
            chunks: vec![0x136AD712, 0x84322759],
        };

        let rem = big.div_mod(58);
        let merged = (u64::from(big.chunks[0]) << 32) | u64::from(big.chunks[1]);

        assert_eq!(merged, 0x136AD71284322759 / 58);
        assert_eq!(u64::from(rem), 0x136AD71284322759 % 58);
    }

    #[test]
    fn big_uint_add_mul() {
        let mut big = BigUint {
            chunks: vec![0x000AD712, 0x84322759],
        };

        big.mul_add(58, 37);
        let merged = (u64::from(big.chunks[0]) << 32) | u64::from(big.chunks[1]);

        assert_eq!(merged, (0x000AD71284322759 * 58) + 37);
    }
}
