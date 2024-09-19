// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u64)]
pub enum SupportedHashes {
    Sha2_256 = 0x12,
    Blake2b256 = 0xb220,
    Blake2b512 = 0xb240,
    Keccak256 = 0x1b,
    Ripemd160 = 0x1053,
}
