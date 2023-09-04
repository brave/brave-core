// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct ChainID(u64);

impl From<u64> for ChainID {
    fn from(src: u64) -> Self {
        Self(src)
    }
}

impl From<ChainID> for u64 {
    fn from(src: ChainID) -> Self {
        src.0
    }
}
