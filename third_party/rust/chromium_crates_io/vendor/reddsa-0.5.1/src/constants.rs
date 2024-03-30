// -*- mode: rust; -*-
//
// This file is part of reddsa.
// Copyright (c) 2019-2021 Zcash Foundation
// See LICENSE for licensing information.
//
// Authors:
// - Henry de Valence <hdevalence@hdevalence.ca>

/// The byte-encoding of the basepoint for `SpendAuthSig`.
// Extracted ad-hoc from librustzcash
// XXX add tests for this value.
pub const SPENDAUTHSIG_BASEPOINT_BYTES: [u8; 32] = [
    48, 181, 242, 170, 173, 50, 86, 48, 188, 221, 219, 206, 77, 103, 101, 109, 5, 253, 28, 194,
    208, 55, 187, 83, 117, 182, 233, 109, 158, 1, 161, 215,
];

/// The byte-encoding of the basepoint for `BindingSig`.
// Extracted ad-hoc from librustzcash
// XXX add tests for this value.
pub const BINDINGSIG_BASEPOINT_BYTES: [u8; 32] = [
    139, 106, 11, 56, 185, 250, 174, 60, 59, 128, 59, 71, 176, 241, 70, 173, 80, 171, 34, 30, 110,
    42, 251, 230, 219, 222, 69, 203, 169, 211, 129, 237,
];
