/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

//! CXX FFI for scrubbing Facebook FBMD tracking tokens from IPTC Special
//! Instructions metadata.
//!
//! See: https://www.hackerfactor.com/blog/index.php?/archives/726-Facebook-Tracking.html

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = image_metadata_stripper)]
mod ffi {
    extern "Rust" {
        /// Scrubs FBMD tracking payloads from IPTC Special Instructions in
        /// |data|.
        ///
        /// Returns the (possibly modified) image bytes. If |data| is not a
        /// recognized image format, returns an empty vector. If no IPTC
        /// Special Instructions / FBMD payload is present, returns a copy of
        /// the original bytes.
        fn remove_iptc_metadata(data: &[u8]) -> Vec<u8>;
    }
}

/// IPTC IIM record/dataset for Special Instructions (aka Instructions).
const IPTC_RECORD_APPLICATION: u8 = 2;
const IPTC_DATASET_SPECIAL_INSTRUCTIONS: u8 = 40;

/// Photoshop Image Resource ID for IPTC-NAA payload.
const PHOTOSHOP_IPTC_RESOURCE_ID: u16 = 0x0404;

const FBMD_PREFIX: &[u8] = b"FBMD";
/// ASCII-hex: 1 tag byte + 2-byte length after "FBMD".
const FBMD_HEADER_HEX_LEN: usize = 6;
/// Each FBMD chunk is 4 bytes → 8 ASCII hex characters.
const FBMD_CHUNK_HEX_LEN: usize = 8;

pub fn remove_iptc_metadata(data: &[u8]) -> Vec<u8> {
    if !is_supported_image(data) {
        return Vec::new();
    }
    let mut out = data.to_vec();
    scrub_fbmd_tracking(&mut out);
    out
}

/// Magic-byte check for formats we may receive from downloads (JPEG/PNG).
/// FBMD scrubbing currently only mutates JPEG; PNG is accepted and returned
/// unchanged when no FBMD is present.
fn is_supported_image(data: &[u8]) -> bool {
    is_jpeg(data) || is_png(data)
}

fn is_jpeg(data: &[u8]) -> bool {
    data.starts_with(&[0xff, 0xd8])
}

fn is_png(data: &[u8]) -> bool {
    data.starts_with(b"\x89PNG\r\n\x1a\n")
}

/// Locates IPTC Special Instructions and zeroes FBMD tracking chunks in place.
fn scrub_fbmd_tracking(data: &mut [u8]) {
    if is_jpeg(data) {
        scrub_fbmd_in_jpeg(data);
    }
}

fn scrub_fbmd_in_jpeg(data: &mut [u8]) {
    let mut i = 2usize; // skip SOI
    while i + 4 <= data.len() {
        if data[i] != 0xff {
            break;
        }
        // Skip fill bytes.
        while i < data.len() && data[i] == 0xff {
            i += 1;
        }
        if i >= data.len() {
            break;
        }
        let marker = data[i];
        i += 1;

        // Standalone markers without a length field.
        if marker == 0x01 || (0xd0..=0xd9).contains(&marker) {
            continue;
        }
        // SOS: compressed image data follows; no more APPn segments.
        if marker == 0xda {
            break;
        }
        if i + 2 > data.len() {
            break;
        }
        let seg_len = u16::from_be_bytes([data[i], data[i + 1]]) as usize;
        if seg_len < 2 || i + seg_len > data.len() {
            break;
        }
        let seg_start = i + 2;
        let seg_end = i + seg_len;

        // APP13 — Photoshop IRB, may contain IPTC-NAA (resource 0x0404).
        if marker == 0xed {
            scrub_fbmd_in_photoshop_irb(&mut data[seg_start..seg_end]);
        }

        i = seg_end;
    }
}

fn scrub_fbmd_in_photoshop_irb(data: &mut [u8]) {
    const PS_SIGNATURE: &[u8] = b"Photoshop 3.0\0";
    if !data.starts_with(PS_SIGNATURE) {
        return;
    }
    let mut i = PS_SIGNATURE.len();
    while i + 12 <= data.len() {
        if &data[i..i + 4] != b"8BIM" {
            break;
        }
        i += 4;
        if i + 2 > data.len() {
            break;
        }
        let resource_id = u16::from_be_bytes([data[i], data[i + 1]]);
        i += 2;

        // Pascal name string, padded to even size including the length byte.
        if i >= data.len() {
            break;
        }
        let name_len = data[i] as usize;
        let name_total = name_len + 1;
        let name_padded = name_total + (name_total % 2);
        if i + name_padded + 4 > data.len() {
            break;
        }
        i += name_padded;

        let resource_size =
            u32::from_be_bytes([data[i], data[i + 1], data[i + 2], data[i + 3]]) as usize;
        i += 4;
        if i + resource_size > data.len() {
            break;
        }
        if resource_id == PHOTOSHOP_IPTC_RESOURCE_ID {
            scrub_fbmd_in_iptc_iim(&mut data[i..i + resource_size]);
        }
        i += resource_size;
        // Resource data is padded to an even size.
        if resource_size % 2 == 1 {
            i += 1;
        }
    }
}

fn scrub_fbmd_in_iptc_iim(data: &mut [u8]) {
    let mut i = 0usize;
    while i + 5 <= data.len() {
        if data[i] != 0x1c {
            // Be tolerant of leading padding / unknown bytes.
            i += 1;
            continue;
        }
        let record = data[i + 1];
        let dataset = data[i + 2];
        let Some((value_offset, value_len)) = parse_iptc_value_size(data, i + 3) else {
            break;
        };
        if value_offset + value_len > data.len() {
            break;
        }
        if record == IPTC_RECORD_APPLICATION && dataset == IPTC_DATASET_SPECIAL_INSTRUCTIONS {
            scrub_fbmd_in_special_instructions(&mut data[value_offset..value_offset + value_len]);
        }
        i = value_offset + value_len;
    }
}

/// Parses the IPTC size field at |size_offset|.
///
/// Returns `(value_offset, value_len)`.
fn parse_iptc_value_size(data: &[u8], size_offset: usize) -> Option<(usize, usize)> {
    if size_offset + 2 > data.len() {
        return None;
    }
    let size_field = u16::from_be_bytes([data[size_offset], data[size_offset + 1]]);
    if size_field & 0x8000 == 0 {
        return Some((size_offset + 2, size_field as usize));
    }
    let ext_len = (size_field & 0x7fff) as usize;
    if ext_len == 0 || size_offset + 2 + ext_len > data.len() {
        return None;
    }
    let mut value_len = 0usize;
    for b in &data[size_offset + 2..size_offset + 2 + ext_len] {
        value_len = (value_len << 8) | (*b as usize);
    }
    Some((size_offset + 2 + ext_len, value_len))
}

/// Scrubs FBMD chunk payloads inside an IPTC Special Instructions field.
///
/// Facebook stores Special Instructions as an ASCII-hex encoding of:
/// `FBMD` + 1-byte tag + 2-byte BE length + (length + 1) × 4-byte chunks.
fn scrub_fbmd_in_special_instructions(field: &mut [u8]) {
    // No Special Instructions / no FBMD → leave unchanged.
    let Some(fbmd_at) = find_fbmd_prefix(field) else {
        return;
    };
    scrub_fbmd_ascii_hex(&mut field[fbmd_at..]);
}

fn find_fbmd_prefix(data: &[u8]) -> Option<usize> {
    data.windows(FBMD_PREFIX.len()).position(|window| window == FBMD_PREFIX)
}

fn scrub_fbmd_ascii_hex(fbmd: &mut [u8]) {
    // FBMD + tag(2 hex) + length(4 hex)
    if fbmd.len() < FBMD_PREFIX.len() + FBMD_HEADER_HEX_LEN {
        return;
    }
    if !fbmd.starts_with(FBMD_PREFIX) {
        return;
    }

    let header = &fbmd[FBMD_PREFIX.len()..FBMD_PREFIX.len() + FBMD_HEADER_HEX_LEN];
    // header[0..2] = tag byte (ignored); header[2..6] = length.
    let Ok(chunk_count_field) = std::str::from_utf8(&header[2..6]) else {
        return;
    };
    let Ok(length) = u16::from_str_radix(chunk_count_field, 16) else {
        return;
    };

    // Payload is (length + 1) chunks of 4 bytes each.
    let Some(chunk_count) = (length as usize).checked_add(1) else {
        return;
    };
    let Some(chunks_hex_len) = chunk_count.checked_mul(FBMD_CHUNK_HEX_LEN) else {
        return;
    };
    let chunks_start = FBMD_PREFIX.len() + FBMD_HEADER_HEX_LEN;
    let Some(chunks_end) = chunks_start.checked_add(chunks_hex_len) else {
        return;
    };
    if chunks_end > fbmd.len() {
        return;
    }
    if !fbmd[chunks_start..chunks_end].iter().all(|b| b.is_ascii_hexdigit()) {
        return;
    }

    // Nuke tracking chunks with zeroed 4-byte values (ASCII "00000000").
    for b in &mut fbmd[chunks_start..chunks_end] {
        *b = b'0';
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn scrubs_fbmd_chunks_from_special_instructions() {
        // From https://www.hackerfactor.com/blog/index.php?/archives/726-Facebook-Tracking.html
        let original = b"FBMD01000ac60300004a1d00002d4b000067580000c9650000d5fc000054350100953a0100d3420100e84b01005f8f0100";
        let mut field = original.to_vec();
        scrub_fbmd_in_special_instructions(&mut field);
        assert_eq!(
            std::str::from_utf8(&field).unwrap(),
            "FBMD01000a0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        );
    }

    #[test]
    fn leaves_field_without_fbmd_unchanged() {
        let mut field = b"no tracking here".to_vec();
        let before = field.clone();
        scrub_fbmd_in_special_instructions(&mut field);
        assert_eq!(field, before);
    }

    #[test]
    fn scrubs_fbmd_inside_jpeg_app13_iptc() {
        let instructions = b"FBMD23000969010000b1590000cb7700000a8600000c07010046820100b8c0010052590200e5c902006e440300";
        let iptc = {
            let mut v = vec![0x1c, IPTC_RECORD_APPLICATION, IPTC_DATASET_SPECIAL_INSTRUCTIONS];
            let len = instructions.len() as u16;
            v.extend_from_slice(&len.to_be_bytes());
            v.extend_from_slice(instructions);
            v
        };

        // Build a minimal JPEG: SOI + APP13(Photoshop IRB with IPTC) + EOI.
        let mut app13_payload = b"Photoshop 3.0\0".to_vec();
        app13_payload.extend_from_slice(b"8BIM");
        app13_payload.extend_from_slice(&PHOTOSHOP_IPTC_RESOURCE_ID.to_be_bytes());
        app13_payload.push(0); // empty Pascal name
                               // name total length 1 is odd → already even? name_total=1, pad to 2.
        app13_payload.push(0); // pad
        let iptc_len = iptc.len() as u32;
        app13_payload.extend_from_slice(&iptc_len.to_be_bytes());
        app13_payload.extend_from_slice(&iptc);
        if iptc.len() % 2 == 1 {
            app13_payload.push(0);
        }

        let mut jpeg = vec![0xff, 0xd8]; // SOI
        jpeg.extend_from_slice(&[0xff, 0xed]); // APP13
        let seg_len = (app13_payload.len() + 2) as u16;
        jpeg.extend_from_slice(&seg_len.to_be_bytes());
        jpeg.extend_from_slice(&app13_payload);
        jpeg.extend_from_slice(&[0xff, 0xd9]); // EOI

        let out = remove_iptc_metadata(&jpeg);
        assert!(!out.is_empty());
        let out_str = String::from_utf8_lossy(&out);
        assert!(out_str.contains("FBMD230009"));
        assert!(out_str.contains("FBMD23000900000000000000000000000000000000000000000000000000000000000000000000000000000000"));
        assert!(!out_str.contains("69010000"));
    }
}
