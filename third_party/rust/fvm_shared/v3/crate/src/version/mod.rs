// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use std::fmt::Display;

use serde::{Deserialize, Serialize};

/// Specifies the network version
#[derive(Debug, Eq, PartialEq, Clone, Copy, Ord, PartialOrd, Serialize, Deserialize)]
#[repr(transparent)]
#[serde(transparent)]
pub struct NetworkVersion(u32);

impl NetworkVersion {
    /// genesis (specs-actors v0.9.3)
    pub const V0: Self = Self(0);
    /// breeze (specs-actors v0.9.7)
    pub const V1: Self = Self(1);
    /// smoke (specs-actors v0.9.8)
    pub const V2: Self = Self(2);
    /// ignition (specs-actors v0.9.11)
    pub const V3: Self = Self(3);
    /// actors v2 (specs-actors v2.0.x)
    pub const V4: Self = Self(4);
    /// tape (increases max prove commit size by 10x)
    pub const V5: Self = Self(5);
    /// kumquat (specs-actors v2.2.0)
    pub const V6: Self = Self(6);
    /// calico (specs-actors v2.3.2)
    pub const V7: Self = Self(7);
    /// persian (post-2.3.2 behaviour transition)
    pub const V8: Self = Self(8);
    /// orange
    pub const V9: Self = Self(9);
    /// trust (specs-actors v3.0.x)
    pub const V10: Self = Self(10);
    /// norwegian (specs-actors v3.1.x)
    pub const V11: Self = Self(11);
    /// turbo (specs-actors v4.0.x)
    pub const V12: Self = Self(12);
    /// HyperDrive
    pub const V13: Self = Self(13);
    /// Chocolate v6
    pub const V14: Self = Self(14);
    /// OhSnap v7
    pub const V15: Self = Self(15);
    /// Skyr (builtin-actors v8)
    pub const V16: Self = Self(16);
    /// Shark (builtin-actors v9)
    pub const V17: Self = Self(17);
    /// Hygge (builtin-actors v10)
    pub const V18: Self = Self(18);
    /// Lightning (builtin-actors v11)
    pub const V19: Self = Self(19);
    /// Thunder (builtin-actors v11)
    pub const V20: Self = Self(20);

    pub const MAX: Self = Self(u32::MAX);

    /// Construct a new arbitrary network version.
    pub const fn new(v: u32) -> Self {
        Self(v)
    }
}

impl Display for NetworkVersion {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl From<u32> for NetworkVersion {
    fn from(v: u32) -> Self {
        Self(v)
    }
}

impl From<NetworkVersion> for u32 {
    fn from(v: NetworkVersion) -> Self {
        v.0
    }
}
