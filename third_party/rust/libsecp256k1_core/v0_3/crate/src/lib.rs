//! Core libraries for libsecp256k1.

#![allow(
    clippy::cast_ptr_alignment,
    clippy::identity_op,
    clippy::many_single_char_names,
    clippy::needless_range_loop,
    clippy::suspicious_op_assign_impl,
    clippy::too_many_arguments,
    clippy::type_complexity
)]
#![deny(
    unused_import_braces,
    unused_imports,
    unused_comparisons,
    unused_must_use,
    unused_variables,
    non_shorthand_field_patterns,
    unreachable_code,
    unused_parens
)]
#![cfg_attr(not(feature = "std"), no_std)]
extern crate alloc;

#[macro_use]
mod field;
#[macro_use]
mod group;
mod der;
mod ecdh;
mod ecdsa;
mod ecmult;
mod error;
mod scalar;

pub use crate::error::Error;

/// Curve related structs.
pub mod curve {
    pub use crate::{
        field::{Field, FieldStorage},
        group::{Affine, AffineStorage, Jacobian, AFFINE_G, CURVE_B},
        scalar::Scalar,
    };

    pub use crate::ecmult::{ECMultContext, ECMultGenContext};
}

/// Utilities to manipulate the secp256k1 curve parameters.
pub mod util {
    pub const TAG_PUBKEY_EVEN: u8 = 0x02;
    pub const TAG_PUBKEY_ODD: u8 = 0x03;
    pub const TAG_PUBKEY_FULL: u8 = 0x04;
    pub const TAG_PUBKEY_HYBRID_EVEN: u8 = 0x06;
    pub const TAG_PUBKEY_HYBRID_ODD: u8 = 0x07;

    pub const MESSAGE_SIZE: usize = 32;
    pub const SECRET_KEY_SIZE: usize = 32;
    pub const RAW_PUBLIC_KEY_SIZE: usize = 64;
    pub const FULL_PUBLIC_KEY_SIZE: usize = 65;
    pub const COMPRESSED_PUBLIC_KEY_SIZE: usize = 33;
    pub const SIGNATURE_SIZE: usize = 64;
    pub const DER_MAX_SIGNATURE_SIZE: usize = 72;

    pub use crate::{
        ecmult::{
            odd_multiples_table, ECMULT_TABLE_SIZE_A, ECMULT_TABLE_SIZE_G, WINDOW_A, WINDOW_G,
        },
        group::{globalz_set_table_gej, set_table_gej_var, AFFINE_INFINITY, JACOBIAN_INFINITY},
    };

    pub use crate::der::{Decoder, SignatureArray};
}
