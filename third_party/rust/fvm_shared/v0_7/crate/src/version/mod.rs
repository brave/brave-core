// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use std::fmt::Display;

use fvm_ipld_encoding::repr::Serialize_repr;

/// Specifies the network version
#[derive(Debug, Eq, PartialEq, Clone, Copy, Ord, PartialOrd, Serialize_repr)]
#[repr(u32)]
#[non_exhaustive]
pub enum NetworkVersion {
    /// genesis (specs-actors v0.9.3)
    V0 = 0,
    /// breeze (specs-actors v0.9.7)
    V1,
    /// smoke (specs-actors v0.9.8)
    V2,
    /// ignition (specs-actors v0.9.11)
    V3,
    /// actors v2 (specs-actors v2.0.x)
    V4,
    /// tape (increases max prove commit size by 10x)
    V5,
    /// kumquat (specs-actors v2.2.0)
    V6,
    /// calico (specs-actors v2.3.2)
    V7,
    /// persian (post-2.3.2 behaviour transition)
    V8,
    /// orange
    V9,
    /// actors v3 (specs-actors v3.0.x)
    V10,
    /// norwegian (specs-actor v3.1.x)
    V11,
    /// actors v3 (specs-actor v4.0.x)
    V12,
    /// reserved
    V13,
    /// actors v6
    V14,
    /// actors v7
    V15,
    /// actors v8
    V16,
}

impl Display for NetworkVersion {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", *self as u32)
    }
}

impl TryFrom<u32> for NetworkVersion {
    type Error = u32;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        use NetworkVersion::*;
        match value {
            0 => Ok(V0),
            1 => Ok(V1),
            2 => Ok(V2),
            3 => Ok(V3),
            4 => Ok(V4),
            5 => Ok(V5),
            6 => Ok(V6),
            7 => Ok(V7),
            8 => Ok(V8),
            9 => Ok(V9),
            10 => Ok(V10),
            11 => Ok(V11),
            12 => Ok(V12),
            13 => Ok(V13),
            14 => Ok(V14),
            15 => Ok(V15),
            16 => Ok(V16),
            _ => Err(value),
        }
    }
}
