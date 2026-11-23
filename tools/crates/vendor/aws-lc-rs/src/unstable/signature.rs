// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

//! The ML-DSA signature APIs have been stabilized; use [`crate::signature`] instead.
//!
//! Everything in this module is a deprecated alias for its stable counterpart in
//! [`crate::signature`]. Aliases are used rather than re-exports so that usage
//! produces deprecation warnings; type identity is unchanged.
//!
//! During stabilization, `PqdsaKeyPair::to_pkcs8` was renamed to
//! [`crate::signature::PqdsaKeyPair::to_pkcs8v1`]. A deprecated `to_pkcs8` alias
//! remains available while the `unstable` feature is enabled; it will be removed
//! along with this module in a future release.

use crate::signature;

/// Deprecated alias for [`crate::signature::PqdsaKeyPair`].
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::PqdsaKeyPair")]
pub type PqdsaKeyPair = signature::PqdsaKeyPair;

/// Deprecated alias for [`crate::signature::PqdsaPrivateKey`].
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::PqdsaPrivateKey")]
pub type PqdsaPrivateKey<'a> = signature::PqdsaPrivateKey<'a>;

/// Deprecated alias for [`crate::signature::PqdsaPublicKey`].
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::PqdsaPublicKey")]
pub type PqdsaPublicKey = signature::PqdsaPublicKey;

/// Deprecated alias for [`crate::signature::PqdsaSigningAlgorithm`].
#[deprecated(
    since = "1.18.0",
    note = "use aws_lc_rs::signature::PqdsaSigningAlgorithm"
)]
pub type PqdsaSigningAlgorithm = signature::PqdsaSigningAlgorithm;

/// Deprecated alias for [`crate::signature::PqdsaVerificationAlgorithm`].
#[deprecated(
    since = "1.18.0",
    note = "use aws_lc_rs::signature::PqdsaVerificationAlgorithm"
)]
pub type PqdsaVerificationAlgorithm = signature::PqdsaVerificationAlgorithm;

/// Verification of ML-DSA-44 signatures.
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_44")]
pub const ML_DSA_44: signature::PqdsaVerificationAlgorithm = signature::ML_DSA_44;

/// Verification of ML-DSA-65 signatures.
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_65")]
pub const ML_DSA_65: signature::PqdsaVerificationAlgorithm = signature::ML_DSA_65;

/// Verification of ML-DSA-87 signatures.
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_87")]
pub const ML_DSA_87: signature::PqdsaVerificationAlgorithm = signature::ML_DSA_87;

/// Signing using ML-DSA-44.
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_44_SIGNING")]
pub const ML_DSA_44_SIGNING: signature::PqdsaSigningAlgorithm = signature::ML_DSA_44_SIGNING;

/// Signing using ML-DSA-65.
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_65_SIGNING")]
pub const ML_DSA_65_SIGNING: signature::PqdsaSigningAlgorithm = signature::ML_DSA_65_SIGNING;

/// Signing using ML-DSA-87.
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_87_SIGNING")]
pub const ML_DSA_87_SIGNING: signature::PqdsaSigningAlgorithm = signature::ML_DSA_87_SIGNING;

/// Verification of MLDSA-44 signatures
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_44")]
pub const MLDSA_44: signature::PqdsaVerificationAlgorithm = signature::ML_DSA_44;

/// Verification of MLDSA-65 signatures
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_65")]
pub const MLDSA_65: signature::PqdsaVerificationAlgorithm = signature::ML_DSA_65;

/// Verification of MLDSA-87 signatures
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_87")]
pub const MLDSA_87: signature::PqdsaVerificationAlgorithm = signature::ML_DSA_87;

/// Sign using MLDSA-44
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_44_SIGNING")]
pub const MLDSA_44_SIGNING: signature::PqdsaSigningAlgorithm = signature::ML_DSA_44_SIGNING;

/// Sign using MLDSA-65
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_65_SIGNING")]
pub const MLDSA_65_SIGNING: signature::PqdsaSigningAlgorithm = signature::ML_DSA_65_SIGNING;

/// Sign using MLDSA-87
#[deprecated(since = "1.18.0", note = "use aws_lc_rs::signature::ML_DSA_87_SIGNING")]
pub const MLDSA_87_SIGNING: signature::PqdsaSigningAlgorithm = signature::ML_DSA_87_SIGNING;
