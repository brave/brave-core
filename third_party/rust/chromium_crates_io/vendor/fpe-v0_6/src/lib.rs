//! Format-preserving encryption algorithms.
//!
//! # Example
//!
//! ```
//! extern crate aes;
//! extern crate fpe;
//!
//! use aes::Aes256;
//! use fpe::ff1::{BinaryNumeralString, FF1};
//!
//! let key = [0; 32];
//! let radix = 2;
//! let pt = [0xab, 0xcd, 0xef];
//!
//! let ff = FF1::<Aes256>::new(&key, radix).unwrap();
//! let ct = ff.encrypt(&[], &BinaryNumeralString::from_bytes_le(&pt)).unwrap();
//! assert_eq!(ct.to_bytes_le(), [0x75, 0xfb, 0x62]);
//!
//! let p2 = ff.decrypt(&[], &ct).unwrap();
//! assert_eq!(p2.to_bytes_le(), pt);
//! ```

#![cfg_attr(not(any(feature = "std", test)), no_std)]
// Refuse to compile if documentation is missing
#![deny(missing_docs)]
// Catch documentation errors caused by code changes.
#![deny(rustdoc::broken_intra_doc_links)]

#[cfg(feature = "alloc")]
extern crate alloc;

pub mod ff1;
