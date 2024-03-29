// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// https://github.com/zcash/librustzcash/blob/zcash_primitives-0.15.0/zcash_primitives/src/transaction/components/orchard.rs

use std::io::{self, Write};
use orchard::{
    bundle::{Authorization, Authorized},
    note::{ExtractedNoteCommitment, Nullifier, TransmittedNoteCiphertext},
    primitives::redpallas::{self, SpendAuth},
    value::ValueCommitment,
    Action,
};
use byteorder::{WriteBytesExt};
use crate::librustzcash::encoding::{Vector, Array, CompactSize};
use crate::librustzcash::amount::Amount;

pub fn write_value_commitment<W: Write>(mut writer: W, cv: &ValueCommitment) -> io::Result<()> {
    writer.write_all(&cv.to_bytes())
}

pub fn write_nullifier<W: Write>(mut writer: W, nf: &Nullifier) -> io::Result<()> {
    writer.write_all(&nf.to_bytes())
}

pub fn write_verification_key<W: Write>(
    mut writer: W,
    rk: &redpallas::VerificationKey<SpendAuth>,
) -> io::Result<()> {
    writer.write_all(&<[u8; 32]>::from(rk))
}

pub fn write_cmx<W: Write>(mut writer: W, cmx: &ExtractedNoteCommitment) -> io::Result<()> {
    writer.write_all(&cmx.to_bytes())
}

pub fn write_note_ciphertext<W: Write>(
    mut writer: W,
    nc: &TransmittedNoteCiphertext,
) -> io::Result<()> {
    writer.write_all(&nc.epk_bytes)?;
    writer.write_all(&nc.enc_ciphertext)?;
    writer.write_all(&nc.out_ciphertext)
}


pub fn write_action_without_auth<W: Write>(
    mut writer: W,
    act: &Action<<Authorized as Authorization>::SpendAuth>,
) -> io::Result<()> {
    write_value_commitment(&mut writer, act.cv_net())?;
    write_nullifier(&mut writer, act.nullifier())?;
    write_verification_key(&mut writer, act.rk())?;
    write_cmx(&mut writer, act.cmx())?;
    write_note_ciphertext(&mut writer, act.encrypted_note())?;
    Ok(())
}

/// Writes an [`orchard::Bundle`] in the v5 transaction format.
pub fn write_v5_bundle<W: Write>(
    bundle: Option<&orchard::Bundle<Authorized, Amount>>,
    mut writer: W,
) -> io::Result<()> {
    if let Some(bundle) = &bundle {
        Vector::write_nonempty(&mut writer, bundle.actions(), |w, a| {
            write_action_without_auth(w, a)
        })?;

        writer.write_all(&[bundle.flags().to_byte()])?;
        writer.write_all(&bundle.value_balance().to_i64_le_bytes())?;
        writer.write_all(&bundle.anchor().to_bytes())?;
        Vector::write(
            &mut writer,
            bundle.authorization().proof().as_ref(),
            |w, b| w.write_u8(*b),
        )?;
        Array::write(
            &mut writer,
            bundle.actions().iter().map(|a| a.authorization()),
            |w, auth| w.write_all(&<[u8; 64]>::from(*auth)),
        )?;
        writer.write_all(&<[u8; 64]>::from(
            bundle.authorization().binding_signature(),
        ))?;
    } else {
        CompactSize::write(&mut writer, 0)?;
    }

    Ok(())
}

