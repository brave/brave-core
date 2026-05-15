// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use crate::Error;
use parity_scale_codec::{Compact, Decode};

pub(crate) fn decode_scale<T: Decode>(input: &mut &[u8]) -> Result<T, Error> {
    T::decode(input).map_err(|_| Error::InvalidScale)
}

pub(crate) fn decode_vec_len(input: &mut &[u8]) -> Result<usize, Error> {
    let len: Compact<u32> = decode_scale(input)?;
    usize::try_from(len.0).map_err(|_| Error::InvalidLength)
}

pub(crate) fn decode_type_id(input: &mut &[u8]) -> Result<u32, Error> {
    decode_scale::<Compact<u32>>(input).map(|v| v.0)
}

pub(crate) fn decode_vec<T>(
    input: &mut &[u8],
    mut parse_elem: impl FnMut(&mut &[u8]) -> Result<T, Error>,
) -> Result<Vec<T>, Error> {
    let len = decode_vec_len(input)?;
    let mut out = Vec::with_capacity(len);
    for _ in 0..len {
        out.push(parse_elem(input)?);
    }
    Ok(out)
}

pub(crate) fn decode_option<T>(
    input: &mut &[u8],
    parse_some: impl FnOnce(&mut &[u8]) -> Result<T, Error>,
) -> Result<Option<T>, Error> {
    match decode_scale::<u8>(input)? {
        0 => Ok(None),
        1 => Ok(Some(parse_some(input)?)),
        _ => Err(Error::InvalidMetadata),
    }
}
