// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
use bitflags::bitflags;
use fvm_ipld_encoding::strict_bytes;
use serde::{Deserialize, Serialize};
use serde_tuple::*;

use crate::ActorID;

/// Event with extra information stamped by the FVM. This is the structure that gets committed
/// on-chain via the receipt.
#[derive(Serialize_tuple, Deserialize_tuple, PartialEq, Eq, Clone, Debug)]
pub struct StampedEvent {
    /// Carries the ID of the actor that emitted this event.
    pub emitter: ActorID,
    /// The event as emitted by the actor.
    pub event: ActorEvent,
}

impl StampedEvent {
    pub fn new(emitter: ActorID, event: ActorEvent) -> Self {
        Self { emitter, event }
    }
}

/// An event as originally emitted by the actor.
#[derive(Serialize_tuple, Deserialize_tuple, PartialEq, Eq, Clone, Debug)]
#[serde(transparent)]
pub struct ActorEvent {
    pub entries: Vec<Entry>,
}

impl From<Vec<Entry>> for ActorEvent {
    fn from(entries: Vec<Entry>) -> Self {
        Self { entries }
    }
}

bitflags! {
    /// Flags associated with an Event entry.
    #[derive(Deserialize, Serialize, Copy, Clone, Eq, PartialEq, Debug)]
    #[repr(transparent)] // we pass this type through a syscall
    #[serde(transparent)]
    pub struct Flags: u64 {
        const FLAG_INDEXED_KEY      = 0b00000001;
        const FLAG_INDEXED_VALUE    = 0b00000010;
        const FLAG_INDEXED_ALL      = Self::FLAG_INDEXED_KEY.bits() | Self::FLAG_INDEXED_VALUE.bits();
    }
}

/// A key value entry inside an Event.
#[derive(Serialize_tuple, Deserialize_tuple, PartialEq, Eq, Clone, Debug)]
pub struct Entry {
    /// A bitmap conveying metadata or hints about this entry.
    pub flags: Flags,
    /// The key of this event.
    pub key: String,
    /// The value's codec. Must be IPLD_RAW (0x55) for now according to FIP-0049.
    pub codec: u64,
    /// The event's value.
    #[serde(with = "strict_bytes")]
    pub value: Vec<u8>,
}
