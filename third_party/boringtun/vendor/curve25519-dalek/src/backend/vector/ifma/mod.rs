// -*- mode: rust; -*-
//
// This file is part of curve25519-dalek.
// Copyright (c) 2018-2019 Henry de Valence
// See LICENSE for licensing information.
//
// Authors:
// - Henry de Valence <hdevalence@hdevalence.ca>

#![doc = include_str!("../../../../docs/ifma-notes.md")]

#[allow(missing_docs)]
pub mod field;

#[allow(missing_docs)]
pub mod edwards;

pub mod constants;

pub(crate) use self::edwards::{CachedPoint, ExtendedPoint};
