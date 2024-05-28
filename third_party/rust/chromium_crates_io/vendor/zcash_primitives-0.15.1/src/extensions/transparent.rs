//! Core traits and structs for Transparent Zcash Extensions.

use std::fmt;

use crate::transaction::components::{
    tze::{self, TzeOut},
    Amount,
};

/// A typesafe wrapper for witness payloads
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct AuthData(pub Vec<u8>);

/// Binary parsing capability for TZE preconditions & witnesses.
///
/// Serialization formats interpreted by implementations of this trait become consensus-critical
/// upon activation of of the extension that uses them.
pub trait FromPayload: Sized {
    type Error;

    /// Parses an extension-specific witness or precondition from a mode and payload.
    fn from_payload(mode: u32, payload: &[u8]) -> Result<Self, Self::Error>;
}

/// Binary serialization capability for TZE preconditions & witnesses.
///
/// Serialization formats used by implementations of this trait become consensus-critical upon
/// activation of of the extension that uses them.
pub trait ToPayload {
    /// Returns a serialized payload and its corresponding mode.
    fn to_payload(&self) -> (u32, Vec<u8>);
}

/// A condition that can be used to encumber transparent funds.
///
/// This struct is an intermediate representation between the serialized binary format which is
/// used inside of a transaction, and extension-specific types. The payload field of this struct is
/// treated as opaque to all but the extension corresponding to the encapsulated `extension_id`
/// value.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Precondition {
    pub extension_id: u32,
    pub mode: u32,
    pub payload: Vec<u8>,
}

impl Precondition {
    /// Produce the intermediate format for an extension-specific precondition
    /// type.
    pub fn from<P: ToPayload>(extension_id: u32, value: &P) -> Precondition {
        let (mode, payload) = value.to_payload();
        Precondition {
            extension_id,
            mode,
            payload,
        }
    }

    /// Attempt to parse an extension-specific precondition value from the
    /// intermediate representation.
    pub fn try_to<P: FromPayload>(&self) -> Result<P, P::Error> {
        P::from_payload(self.mode, &self.payload)
    }
}

/// Data that satisfies the precondition for prior encumbered funds, enabling them to be spent.
///
/// This struct is an intermediate representation between the serialized binary format which is
/// used inside of a transaction, and extension-specific types. The payload field of this struct is
/// treated as opaque to all but the extension corresponding to the encapsulated `extension_id`
/// value.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Witness<T> {
    pub extension_id: u32,
    pub mode: u32,
    pub payload: T,
}

impl<T> Witness<T> {
    pub fn map_payload<U, F: FnOnce(T) -> U>(self, f: F) -> Witness<U> {
        Witness {
            extension_id: self.extension_id,
            mode: self.mode,
            payload: f(self.payload),
        }
    }
}

impl Witness<AuthData> {
    /// Produce the intermediate format for an extension-specific witness
    /// type.
    pub fn from<P: ToPayload>(extension_id: u32, value: &P) -> Witness<AuthData> {
        let (mode, payload) = value.to_payload();
        Witness {
            extension_id,
            mode,
            payload: AuthData(payload),
        }
    }

    /// Attempt to parse an extension-specific witness value from the
    /// intermediate representation.
    pub fn try_to<P: FromPayload>(&self) -> Result<P, P::Error> {
        P::from_payload(self.mode, &self.payload.0)
    }
}

#[derive(Debug, PartialEq, Eq)]
pub enum Error<E> {
    InvalidExtensionId(u32),
    ProgramError(E),
}

impl<E: fmt::Display> fmt::Display for Error<E> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::InvalidExtensionId(extension_id) => {
                write!(f, "Unrecognized program type id {}", extension_id)
            }

            Error::ProgramError(err) => write!(f, "Program error: {}", err),
        }
    }
}

/// This is the primary trait which must be implemented by an extension type for that type to be
/// eligible for inclusion in Zcash consensus rules.
pub trait Extension<C> {
    /// Extension-specific precondition type. The extension will need to implement
    /// [`FromPayload<Error = Self::Error>`] for this type in order for their extension to be
    /// eligible for integration into consensus rules.
    type Precondition;

    /// Extension-specific witness type. The extension will need to implement [`FromPayload<Error =
    /// Self::Error>`] for this type in order for their extension to be eligible for integration
    /// into consensus rules.
    type Witness;

    /// Extension-specific error type. This should encompass both parsing and verification errors.
    type Error;

    /// This is the primary method that an extension must implement. Implementations should return
    /// [`Ok(())`] if verification of the witness succeeds against the supplied precondition, and
    /// an error in any other case.
    fn verify_inner(
        &self,
        precondition: &Self::Precondition,
        witness: &Self::Witness,
        context: &C,
    ) -> Result<(), Self::Error>;

    /// This is a convenience method intended for use by consensus nodes at the integration point
    /// to provide easy interoperation with the opaque, cross-extension `Precondition` and
    /// `Witness` types.
    fn verify(
        &self,
        precondition: &Precondition,
        witness: &Witness<AuthData>,
        context: &C,
    ) -> Result<(), Self::Error>
    where
        Self::Precondition: FromPayload<Error = Self::Error>,
        Self::Witness: FromPayload<Error = Self::Error>,
    {
        self.verify_inner(
            &Self::Precondition::from_payload(precondition.mode, &precondition.payload)?,
            &Self::Witness::from_payload(witness.mode, &witness.payload.0)?,
            context,
        )
    }
}

/// An interface for transaction builders which support addition of TZE inputs and outputs.
///
/// This extension trait is satisfied by [`transaction::builder::Builder`]. It provides a minimal
/// contract for interacting with the transaction builder, that extension library authors can use
/// to add extension-specific builder traits that may be used to interact with the transaction
/// builder. This may make it simpler for projects that include transaction-builder functionality
/// to integrate with third-party extensions without those extensions being coupled to a particular
/// transaction or builder representation.
///
/// [`transaction::builder::Builder`]: crate::transaction::builder::Builder
pub trait ExtensionTxBuilder<'a> {
    type BuildCtx;
    type BuildError;

    /// Adds a TZE input to the transaction by providing a witness to a precondition identified by a
    /// prior outpoint.
    ///
    /// The `witness_builder` function allows the transaction builder to provide extra contextual
    /// information from the transaction under construction to be used in the production of this
    /// witness (for example, so that the witness may internally make commitments based upon this
    /// information.) For the standard transaction builder, the value provided here is the
    /// transaction under construction.
    fn add_tze_input<WBuilder, W: ToPayload>(
        &mut self,
        extension_id: u32,
        mode: u32,
        prevout: (tze::OutPoint, TzeOut),
        witness_builder: WBuilder,
    ) -> Result<(), Self::BuildError>
    where
        WBuilder: 'a + (FnOnce(&Self::BuildCtx) -> Result<W, Self::BuildError>);

    /// Adds a TZE precondition to the transaction which must be satisfied by a future transaction's
    /// witness in order to spend the specified `amount`.
    fn add_tze_output<Precondition: ToPayload>(
        &mut self,
        extension_id: u32,
        value: Amount,
        guarded_by: &Precondition,
    ) -> Result<(), Self::BuildError>;
}
