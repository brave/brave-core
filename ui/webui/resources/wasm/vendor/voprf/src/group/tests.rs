// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Includes a series of tests for the group implementations

use crate::{Error, Group, Result};

// Test that the deserialization of a group element should throw an error if the
// identity element can be deserialized properly

#[test]
fn test_group_properties() -> Result<()> {
    use p256::NistP256;
    use p384::NistP384;
    use p521::NistP521;

    #[cfg(feature = "ristretto255")]
    {
        use crate::Ristretto255;

        test_identity_element_error::<Ristretto255>()?;
        test_zero_scalar_error::<Ristretto255>()?;
    }

    test_identity_element_error::<NistP256>()?;
    test_zero_scalar_error::<NistP256>()?;

    test_identity_element_error::<NistP384>()?;
    test_zero_scalar_error::<NistP384>()?;

    test_identity_element_error::<NistP521>()?;
    test_zero_scalar_error::<NistP521>()?;

    Ok(())
}

// Checks that the identity element cannot be deserialized
fn test_identity_element_error<G: Group>() -> Result<()> {
    let identity = G::identity_elem();
    let result = G::deserialize_elem(&G::serialize_elem(identity));
    assert!(matches!(result, Err(Error::Deserialization)));

    Ok(())
}

// Checks that the zero scalar cannot be deserialized
fn test_zero_scalar_error<G: Group>() -> Result<()> {
    let zero_scalar = G::zero_scalar();
    let result = G::deserialize_scalar(&G::serialize_scalar(zero_scalar));
    assert!(matches!(result, Err(Error::Deserialization)));

    Ok(())
}
