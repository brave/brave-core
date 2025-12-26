//! This crate provides ways of identifying an actor within the git repository both in shared and mutable variants.
//!
//! ## Feature Flags
#![cfg_attr(
    all(doc, feature = "document-features"),
    doc = ::document_features::document_features!()
)]
#![cfg_attr(all(doc, feature = "document-features"), feature(doc_cfg))]
#![deny(missing_docs, rust_2018_idioms, unsafe_code)]

/// The re-exported `bstr` crate.
///
/// For convenience to allow using `bstr` without adding it to own cargo manifest.
pub use bstr;
use bstr::{BStr, BString};
/// The re-exported `gix-date` crate.
///
/// For convenience to allow using `gix-date` without adding it to own cargo manifest.
pub use gix_date as date;

mod identity;
///
pub mod signature;

/// A person with name and email.
#[derive(Default, PartialEq, Eq, Debug, Hash, Ord, PartialOrd, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct Identity {
    /// The actors name, potentially with whitespace as parsed.
    ///
    /// Use [IdentityRef::trim()] or trim manually to be able to clean it up.
    pub name: BString,
    /// The actor's email, potentially with whitespace and garbage as parsed.
    ///
    /// Use [IdentityRef::trim()] or trim manually to be able to clean it up.
    pub email: BString,
}

/// A person with name and email, as reference.
#[derive(Default, PartialEq, Eq, Debug, Hash, Ord, PartialOrd, Clone, Copy)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct IdentityRef<'a> {
    /// The actors name, potentially with whitespace as parsed.
    ///
    /// Use [IdentityRef::trim()] or trim manually to be able to clean it up.
    #[cfg_attr(feature = "serde", serde(borrow))]
    pub name: &'a BStr,
    /// The actor's email, potentially with whitespace and garbage as parsed.
    ///
    /// Use [IdentityRef::trim()] or trim manually to be able to clean it up.
    pub email: &'a BStr,
}

/// A mutable signature that is created by an actor at a certain time.
///
/// Note that this is not a cryptographical signature.
#[derive(Default, PartialEq, Eq, Debug, Hash, Ord, PartialOrd, Clone)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct Signature {
    /// The actors name, potentially with whitespace as parsed.
    ///
    /// Use [SignatureRef::trim()] or trim manually to be able to clean it up.
    pub name: BString,
    /// The actor's email, potentially with whitespace and garbage as parsed.
    ///
    /// Use [SignatureRef::trim()] or trim manually to be able to clean it up.
    pub email: BString,
    /// The time stamp at which the signature is performed.
    pub time: date::Time,
}

/// An immutable signature that is created by an actor at a certain time.
///
/// All of its fields are references to the backing buffer to allow lossless
/// round-tripping, as decoding the `time` field could be a lossy transformation.
///
/// Note that this is not a cryptographical signature.
#[derive(Default, PartialEq, Eq, Debug, Hash, Ord, PartialOrd, Clone, Copy)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct SignatureRef<'a> {
    /// The actors name, potentially with whitespace as parsed.
    ///
    /// Use [SignatureRef::trim()] or trim manually for cleanup.
    #[cfg_attr(feature = "serde", serde(borrow))]
    pub name: &'a BStr,
    /// The actor's email, potentially with whitespace and garbage as parsed.
    ///
    /// Use [SignatureRef::trim()] or trim manually for cleanup.
    pub email: &'a BStr,
    /// The timestamp at which the signature was performed,
    /// potentially malformed due to lenient parsing.
    ///
    /// Use [`SignatureRef::time()`] to decode.
    pub time: &'a str,
}
