// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#![cfg(debug_assertions)]

use crate::fips::{assert_fips_status_indicator, FipsServiceStatus};
use crate::rsa::{KeyPair, KeySize, PrivateDecryptingKey};

macro_rules! generate_key {
    ($name:ident, $type:ident, $size:expr) => {
        #[test]
        fn $name() {
            // Key generation should set the approved indicator
            let _ =
                assert_fips_status_indicator!($type::generate($size), FipsServiceStatus::Approved)
                    .expect("key generated");
        }
    };
}

generate_key!(rsa2048_signing_generate_key, KeyPair, KeySize::Rsa2048);
// Key generation for large RSA keys is very slow
#[cfg(not(disable_slow_tests))]
generate_key!(rsa3072_signing_generate_key, KeyPair, KeySize::Rsa3072);
// Key generation for large RSA keys is very slow
#[cfg(not(disable_slow_tests))]
generate_key!(rsa4096_signing_generate_key, KeyPair, KeySize::Rsa4096);
// Key generation for large RSA keys is very slow
#[cfg(not(disable_slow_tests))]
generate_key!(rsa8192_signing_generate_key, KeyPair, KeySize::Rsa8192);

generate_key!(
    rsa2048_encryption_generate_key,
    PrivateDecryptingKey,
    KeySize::Rsa2048
);

// Key generation for large RSA keys is very slow
#[cfg(not(disable_slow_tests))]
generate_key!(
    rsa3072_encryption_generate_key,
    PrivateDecryptingKey,
    KeySize::Rsa3072
);

// Key generation for large RSA keys is very slow
#[cfg(not(disable_slow_tests))]
generate_key!(
    rsa4096_encryption_signing_generate_key,
    PrivateDecryptingKey,
    KeySize::Rsa4096
);

// Key generation for large RSA keys is very slow
#[cfg(not(disable_slow_tests))]
generate_key!(
    rsa8192_encryption_generate_key,
    PrivateDecryptingKey,
    KeySize::Rsa8192
);
