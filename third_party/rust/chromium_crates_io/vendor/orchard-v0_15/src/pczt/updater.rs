use core::fmt;

use alloc::string::String;
use alloc::vec::Vec;

use super::{Action, Bundle, Zip32Derivation};

impl Bundle {
    /// Updates the bundle with information provided in the given closure.
    pub fn update_with<F>(&mut self, f: F) -> Result<(), UpdaterError>
    where
        F: FnOnce(Updater<'_>) -> Result<(), UpdaterError>,
    {
        f(Updater(self))
    }
}

/// An updater for an Orchard PCZT bundle.
#[derive(Debug)]
pub struct Updater<'a>(&'a mut Bundle);

impl Updater<'_> {
    /// Provides read access to the bundle being updated.
    pub fn bundle(&self) -> &Bundle {
        self.0
    }

    /// Updates the action at the given index with information provided in the given
    /// closure.
    pub fn update_action_with<F>(&mut self, index: usize, f: F) -> Result<(), UpdaterError>
    where
        F: FnOnce(ActionUpdater<'_>) -> Result<(), UpdaterError>,
    {
        f(ActionUpdater(
            self.0
                .actions
                .get_mut(index)
                .ok_or(UpdaterError::InvalidIndex)?,
        ))
    }
}

/// An updater for an Orchard PCZT action.
#[derive(Debug)]
pub struct ActionUpdater<'a>(&'a mut Action);

impl ActionUpdater<'_> {
    /// Sets the ZIP 32 derivation path for the spent note's signing key.
    pub fn set_spend_zip32_derivation(&mut self, derivation: Zip32Derivation) {
        self.0.spend.zip32_derivation = Some(derivation);
    }

    /// Stores the given spend-specific proprietary value at the given key.
    pub fn set_spend_proprietary(&mut self, key: String, value: Vec<u8>) {
        self.0.spend.proprietary.insert(key, value);
    }

    /// Sets the ZIP 32 derivation path for the new note's signing key.
    pub fn set_output_zip32_derivation(&mut self, derivation: Zip32Derivation) {
        self.0.output.zip32_derivation = Some(derivation);
    }

    /// Sets the user-facing address that the new note is being sent to.
    pub fn set_output_user_address(&mut self, user_address: String) {
        self.0.output.user_address = Some(user_address);
    }

    /// Stores the given output-specific proprietary value at the given key.
    pub fn set_output_proprietary(&mut self, key: String, value: Vec<u8>) {
        self.0.output.proprietary.insert(key, value);
    }
}

/// Errors that can occur while updating an Orchard bundle in a PCZT.
#[derive(Debug)]
#[non_exhaustive]
pub enum UpdaterError {
    /// An out-of-bounds index was provided when looking up an action.
    InvalidIndex,
}

impl fmt::Display for UpdaterError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            UpdaterError::InvalidIndex => write!(f, "Action index is out-of-bounds"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for UpdaterError {}
