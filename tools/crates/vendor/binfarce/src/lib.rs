#![forbid(unsafe_code)]

//! Extremely minimal parser for ELF/PE/Mach-o/ar.
//!
//! This crate is used mostly for sharing code between `cargo-bloat` and `auditable-extract` crates.
//! It implements just enough features for those tools to work.
//! If you're looking for a fully-featured parser, see [`goblin`](https://crates.io/crates/goblin).

// Style lints on which pre-existing code disagrees with Clippy
#![allow(clippy::single_match)]
#![allow(clippy::while_let_loop)]
#![allow(clippy::single_char_pattern)]
#![allow(clippy::many_single_char_names)]

// I find this more readable
#![allow(clippy::skip_while_next)]

pub mod ar;
pub mod demangle;
pub mod elf32;
pub mod elf64;
pub mod macho;
pub mod pe;
mod parser;
mod error;

pub use crate::error::ParseError;
pub use crate::parser::UnexpectedEof;

#[derive(Clone, Copy, PartialEq, Debug)]
pub enum ByteOrder {
    LittleEndian,
    BigEndian,
}

pub enum Format {
    Elf32 {byte_order: ByteOrder},
    Elf64 {byte_order: ByteOrder},
    Macho,
    PE,
    Unknown,
}

pub fn detect_format(data: &[u8]) -> Format {
    if data.len() < 8 {return Format::Unknown};
    let macho_signatures = [
        b"\xCA\xFE\xBA\xBE", // multi-architecture macOS
        b"\xFE\xED\xFA\xCE", // 32-bit macOS
        b"\xFE\xED\xFA\xCF", // 64-bit macOS
        b"\xCE\xFA\xED\xFE", // and now the same in reverse order
        b"\xCF\xFA\xED\xFE", // because they could
    ];
    if data.starts_with(b"\x7FELF") {
        let byte_order = match data[5] {
            1 => ByteOrder::LittleEndian,
            2 => ByteOrder::BigEndian,
            _ => return Format::Unknown,
        };
        return match data[4] {
            1 => Format::Elf32{byte_order},
            2 => Format::Elf64{byte_order},
            _ => Format::Unknown,
        };
    } else if data.starts_with(b"MZ") {
        return Format::PE;
    } else {
        for signature in &macho_signatures {
            if data.starts_with(*signature) {
                return Format::Macho;
            }
        }
    }
    Format::Unknown
}
