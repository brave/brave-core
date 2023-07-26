extern crate base64;
extern crate challenge_bypass_ristretto;
extern crate core;
extern crate hmac;
extern crate rand;
extern crate sha2;

use core::ptr;
use std::cell::RefCell;
use std::error::Error;
use std::ffi::CString;
use std::os::raw::{c_char, c_int};
use std::{slice, str};

use challenge_bypass_ristretto::errors::InternalError;
use challenge_bypass_ristretto::voprf::{
    BatchDLEQProof, BlindedToken, DLEQProof, PublicKey, SignedToken, SigningKey, Token,
    TokenPreimage, UnblindedToken, VerificationKey, VerificationSignature,
};
use hmac::Hmac;
use rand::rngs::OsRng;
use sha2::Sha512;

type HmacSha512 = Hmac<Sha512>;

#[cfg(not(feature = "cbindgen"))]
thread_local! {
    static LAST_ERROR: RefCell<Option<Box<dyn Error>>> = RefCell::new(None);
}

/// Update the last error that occured.
#[cfg(not(feature = "cbindgen"))]
fn update_last_error<T>(err: T)
where
    T: Into<Box<dyn Error>>,
{
    LAST_ERROR.with(|prev| {
        *prev.borrow_mut() = Some(err.into());
    });
}

/// Clear and return the message associated with the last error.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn last_error_message() -> *mut c_char {
    LAST_ERROR.with(|prev| {
        let mut ret = ptr::null_mut();
        if let Some(ref err) = *prev.borrow_mut() {
            if let Ok(s) = CString::new(err.to_string()) {
                ret = s.into_raw();
            }
        }
        *prev.borrow_mut() = None;
        ret
    })
}

/// Destroy a `*c_char` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn c_char_destroy(s: *mut c_char) {
    if !s.is_null() {
        drop(CString::from_raw(s));
    }
}

macro_rules! impl_base64 {
    ($t:ident, $en:ident, $de:ident) => {
        /// Return base64 encoding as a C string.
        #[no_mangle]
        #[allow(unsafe_op_in_unsafe_fn)]
        pub unsafe extern "C" fn $en(t: *const $t) -> *mut c_char {
            if !t.is_null() {
                let b64 = (&*t).encode_base64();
                return CString::from_vec_unchecked(b64.into()).into_raw();
            }
            update_last_error("Pointer to struct was null");
            return ptr::null_mut();
        }

        /// Decode from base64 C string.
        ///
        /// If something goes wrong, this will return a null pointer. Don't forget to
        /// destroy the returned pointer once you are done with it!
        #[no_mangle]
        #[allow(unsafe_op_in_unsafe_fn)]
        pub unsafe extern "C" fn $de(s: *const u8, s_length: usize) -> *mut $t {
            if !s.is_null() {
                let slice = slice::from_raw_parts(s, s_length);

                match str::from_utf8(slice) {
                    Ok(s_as_str) => match $t::decode_base64(s_as_str) {
                        Ok(t) => return Box::into_raw(Box::new(t)),
                        Err(err) => update_last_error(err),
                    },
                    Err(err) => update_last_error(err),
                }
            } else {
                update_last_error("Supplied string was null");
            }
            return ptr::null_mut();
        }
    };
}

/// Destroy a `TokenPreimage` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn token_preimage_destroy(t: *mut TokenPreimage) {
    if !t.is_null() {
        drop(Box::from_raw(t));
    }
}

impl_base64!(
    TokenPreimage,
    token_preimage_encode_base64,
    token_preimage_decode_base64
);

/// Generate a new `Token`
///
/// # Safety
///
/// Make sure you destroy the token with [`token_destroy()`] once you are
/// done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn token_random() -> *mut Token {
    let mut rng = OsRng;
    let token = Token::random::<Sha512, OsRng>(&mut rng);
    Box::into_raw(Box::new(token))
}

/// Destroy a `Token` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn token_destroy(token: *mut Token) {
    if !token.is_null() {
        drop(Box::from_raw(token));
    }
}

/// Take a reference to a `Token` and blind it, returning a `BlindedToken`
///
/// If something goes wrong, this will return a null pointer. Don't forget to
/// destroy the `BlindedToken` once you are done with it!
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn token_blind(token: *const Token) -> *mut BlindedToken {
    if token.is_null() {
        update_last_error("Pointer to token was null");
        return ptr::null_mut();
    }

    Box::into_raw(Box::new((*token).blind()))
}

impl_base64!(Token, token_encode_base64, token_decode_base64);

/// Destroy a `BlindedToken` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn blinded_token_destroy(token: *mut BlindedToken) {
    if !token.is_null() {
        drop(Box::from_raw(token));
    }
}

impl_base64!(
    BlindedToken,
    blinded_token_encode_base64,
    blinded_token_decode_base64
);

/// Destroy a `SignedToken` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn signed_token_destroy(token: *mut SignedToken) {
    if !token.is_null() {
        drop(Box::from_raw(token));
    }
}

impl_base64!(
    SignedToken,
    signed_token_encode_base64,
    signed_token_decode_base64
);

/// Destroy an `UnblindedToken` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn unblinded_token_destroy(token: *mut UnblindedToken) {
    if !token.is_null() {
        drop(Box::from_raw(token));
    }
}

/// Take a reference to an `UnblindedToken` and use it to derive a `VerificationKey`
/// using Sha512 as the hash function
///
/// If something goes wrong, this will return a null pointer. Don't forget to
/// destroy the `VerificationKey` once you are done with it!
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn unblinded_token_derive_verification_key_sha512(
    token: *const UnblindedToken,
) -> *mut VerificationKey {
    if token.is_null() {
        update_last_error("Pointer to unblinded token was null");
        return ptr::null_mut();
    }
    Box::into_raw(Box::new((*token).derive_verification_key::<Sha512>()))
}

/// Take a reference to an `UnblindedToken` and return the corresponding `TokenPreimage`
///
/// If something goes wrong, this will return a null pointer. Don't forget to
/// destroy the `BlindedToken` once you are done with it!
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn unblinded_token_preimage(
    token: *const UnblindedToken,
) -> *mut TokenPreimage {
    if token.is_null() {
        update_last_error("Pointer to token was null");
        return ptr::null_mut();
    }

    Box::into_raw(Box::new((*token).t))
}

impl_base64!(
    UnblindedToken,
    unblinded_token_encode_base64,
    unblinded_token_decode_base64
);

/// Destroy a `VerificationKey` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn verification_key_destroy(key: *mut VerificationKey) {
    if !key.is_null() {
        drop(Box::from_raw(key));
    }
}

/// Take a reference to a `VerificationKey` and use it to sign a message
/// using Sha512 as the HMAC hash function to obtain a `VerificationSignature`
///
/// If something goes wrong, this will return a null pointer. Don't forget to
/// destroy the `VerificationSignature` once you are done with it!
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn verification_key_sign_sha512(
    key: *const VerificationKey,
    message: *const u8,
    message_length: usize,
) -> *mut VerificationSignature {
    if key.is_null() {
        update_last_error("Pointer to verification key was null");
        return ptr::null_mut();
    }

    let slice = slice::from_raw_parts(message, message_length);

    Box::into_raw(Box::new((*key).sign::<HmacSha512>(slice)))
}

/// Take a reference to a `VerificationKey` and use it to verify an
/// existing `VerificationSignature` using Sha512 as the HMAC hash function
///
/// Returns -1 if an error was encountered, 1 if the signature failed verification and 0 if valid
///
/// NOTE this is named "invalid" instead of "verify" as it returns true (non-zero) when
/// the signature is invalid and false (zero) when valid
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn verification_key_invalid_sha512(
    key: *const VerificationKey,
    sig: *const VerificationSignature,
    message: *const u8,
    message_length: usize,
) -> c_int {
    if key.is_null() || sig.is_null() {
        update_last_error("Pointer to verification key or signature was null");
        return -1;
    }

    let slice = slice::from_raw_parts(message, message_length);

    if (*key).verify::<HmacSha512>(&*sig, slice) {
        0
    } else {
        1
    }
}

/// Destroy a `VerificationSignature` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn verification_signature_destroy(sig: *mut VerificationSignature) {
    if !sig.is_null() {
        drop(Box::from_raw(sig));
    }
}

impl_base64!(
    VerificationSignature,
    verification_signature_encode_base64,
    verification_signature_decode_base64
);

/// Generate a new `SigningKey`
///
/// # Safety
///
/// Make sure you destroy the key with [`signing_key_destroy()`] once you are
/// done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn signing_key_random() -> *mut SigningKey {
    let mut rng = OsRng;
    let key = SigningKey::random(&mut rng);
    Box::into_raw(Box::new(key))
}

/// Destroy a `SigningKey` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn signing_key_destroy(key: *mut SigningKey) {
    if !key.is_null() {
        drop(Box::from_raw(key));
    }
}

/// Take a reference to a `SigningKey` and use it to sign a `BlindedToken`, returning a
/// `SignedToken`
///
/// If something goes wrong, this will return a null pointer. Don't forget to
/// destroy the `SignedToken` once you are done with it!
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn signing_key_sign(
    key: *const SigningKey,
    token: *const BlindedToken,
) -> *mut SignedToken {
    if key.is_null() || token.is_null() {
        update_last_error("Pointer to signing key or token was null");
        return ptr::null_mut();
    }

    match (*key).sign(&*token) {
        Ok(signed_token) => Box::into_raw(Box::new(signed_token)),
        Err(err) => {
            update_last_error(err);
            ptr::null_mut()
        }
    }
}

/// Take a reference to a `SigningKey` and use it to rederive an `UnblindedToken`
///
/// If something goes wrong, this will return a null pointer. Don't forget to
/// destroy the `UnblindedToken` once you are done with it!
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn signing_key_rederive_unblinded_token(
    key: *const SigningKey,
    t: *const TokenPreimage,
) -> *mut UnblindedToken {
    if key.is_null() || t.is_null() {
        update_last_error("Pointer to signing key or token preimage was null");
        return ptr::null_mut();
    }

    Box::into_raw(Box::new((*key).rederive_unblinded_token(&*t)))
}

/// Take a reference to a `SigningKey` and return it's associated `PublicKey`
///
/// If something goes wrong, this will return a null pointer. Don't forget to
/// destroy the `PublicKey` once you are done with it!
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn signing_key_get_public_key(key: *const SigningKey) -> *mut PublicKey {
    if key.is_null() {
        update_last_error("Pointer to signing key was null");
        return ptr::null_mut();
    }

    Box::into_raw(Box::new((*key).public_key))
}

impl_base64!(
    SigningKey,
    signing_key_encode_base64,
    signing_key_decode_base64
);

/// Destroy a `DLEQProof` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn dleq_proof_destroy(p: *mut DLEQProof) {
    if !p.is_null() {
        drop(Box::from_raw(p));
    }
}

impl_base64!(
    DLEQProof,
    dleq_proof_encode_base64,
    dleq_proof_decode_base64
);

/// Create a new DLEQ proof
///
/// If something goes wrong, this will return a null pointer. Don't forget to
/// destroy the `DLEQProof` once you are done with it!
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn dleq_proof_new(
    blinded_token: *const BlindedToken,
    signed_token: *const SignedToken,
    key: *const SigningKey,
) -> *mut DLEQProof {
    if !blinded_token.is_null() && !signed_token.is_null() && !key.is_null() {
        let mut rng = OsRng;
        match DLEQProof::new::<Sha512, OsRng>(&mut rng, &*blinded_token, &*signed_token, &*key) {
            Ok(proof) => return Box::into_raw(Box::new(proof)),
            Err(err) => update_last_error(err),
        }
        return ptr::null_mut();
    }
    update_last_error("Pointer to blinded token, signed token or signing key was null");
    ptr::null_mut()
}

/// Check if a DLEQ proof is invalid
///
/// Returns -1 if an error was encountered, 1 if the proof failed verification and 0 if valid
///
/// NOTE this is named "invalid" instead of "verify" as it returns true (non-zero) when
/// the proof is invalid and false (zero) when valid
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn dleq_proof_invalid(
    proof: *const DLEQProof,
    blinded_token: *const BlindedToken,
    signed_token: *const SignedToken,
    public_key: *const PublicKey,
) -> c_int {
    if !proof.is_null()
        && !blinded_token.is_null()
        && !signed_token.is_null()
        && !public_key.is_null()
    {
        match (*proof).verify::<Sha512>(&*blinded_token, &*signed_token, &*public_key) {
            Ok(_) => return 0,
            Err(err) => {
                if let Some(InternalError::VerifyError) =
                    err.source().unwrap().downcast_ref::<InternalError>()
                {
                    return 1;
                } else {
                    update_last_error(err);
                    return -1;
                }
            }
        }
    }
    update_last_error("Pointer to proof, blinded token, signed token or signing key was null");
    -1
}

/// Destroy a `PublicKey` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn public_key_destroy(k: *mut PublicKey) {
    if !k.is_null() {
        drop(Box::from_raw(k));
    }
}

impl_base64!(
    PublicKey,
    public_key_encode_base64,
    public_key_decode_base64
);

/// Destroy a `BatchDLEQProof` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn batch_dleq_proof_destroy(p: *mut BatchDLEQProof) {
    if !p.is_null() {
        drop(Box::from_raw(p));
    }
}

impl_base64!(
    BatchDLEQProof,
    batch_dleq_proof_encode_base64,
    batch_dleq_proof_decode_base64
);

/// Create a new batch DLEQ proof
///
/// If something goes wrong, this will return a null pointer. Don't forget to
/// destroy the `BatchDLEQProof` once you are done with it!
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn batch_dleq_proof_new(
    blinded_tokens: *const *const BlindedToken,
    signed_tokens: *const *const SignedToken,
    tokens_length: c_int,
    key: *const SigningKey,
) -> *mut BatchDLEQProof {
    if !blinded_tokens.is_null() && !signed_tokens.is_null() && !key.is_null() {
        let mut rng = OsRng;
        let blinded_tokens: &[*const BlindedToken] =
            slice::from_raw_parts(blinded_tokens, tokens_length as usize);
        let blinded_tokens: Vec<BlindedToken> = blinded_tokens.iter().map(|p| **p).collect();
        let signed_tokens: &[*const SignedToken] =
            slice::from_raw_parts(signed_tokens, tokens_length as usize);
        let signed_tokens: Vec<SignedToken> = signed_tokens.iter().map(|p| **p).collect();

        match BatchDLEQProof::new::<Sha512, OsRng>(&mut rng, &blinded_tokens, &signed_tokens, &*key)
        {
            Ok(proof) => return Box::into_raw(Box::new(proof)),
            Err(err) => update_last_error(err),
        }
    }
    update_last_error("Pointer to blinded tokens, signed tokens or signing key was null");
    ptr::null_mut()
}

/// Check if a batch DLEQ proof is invalid
///
/// Returns -1 if an error was encountered, 1 if the proof failed verification and 0 if valid
///
/// NOTE this is named "invalid" instead of "verify" as it returns true (non-zero) when
/// the proof is invalid and false (zero) when valid
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn batch_dleq_proof_invalid(
    proof: *const BatchDLEQProof,
    blinded_tokens: *const *const BlindedToken,
    signed_tokens: *const *const SignedToken,
    tokens_length: c_int,
    public_key: *const PublicKey,
) -> c_int {
    if !proof.is_null()
        && !blinded_tokens.is_null()
        && !signed_tokens.is_null()
        && !public_key.is_null()
    {
        let blinded_tokens: &[*const BlindedToken] =
            slice::from_raw_parts(blinded_tokens, tokens_length as usize);
        let blinded_tokens: Vec<BlindedToken> = blinded_tokens.iter().map(|p| **p).collect();
        let signed_tokens: &[*const SignedToken] =
            slice::from_raw_parts(signed_tokens, tokens_length as usize);
        let signed_tokens: Vec<SignedToken> = signed_tokens.iter().map(|p| **p).collect();

        match (*proof).verify::<Sha512>(&blinded_tokens, &signed_tokens, &*public_key) {
            Ok(_) => return 0,
            Err(err) => {
                if let Some(InternalError::VerifyError) =
                    err.source().unwrap().downcast_ref::<InternalError>()
                {
                    return 1;
                } else {
                    update_last_error(err);
                    return -1;
                }
            }
        }
    }
    update_last_error("Pointer to blinded tokens, signed tokens or signing key was null");
    -1
}

/// Check if a batch DLEQ proof is invalid and unblind each signed token if not
///
/// Returns -1 if an error was encountered, 1 if the proof failed verification and 0 if valid
///
/// NOTE this is named "invalid" instead of "verify" as it returns true (non-zero) when
/// the proof is invalid and false (zero) when valid
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn batch_dleq_proof_invalid_or_unblind(
    proof: *const BatchDLEQProof,
    tokens: *const *const Token,
    blinded_tokens: *const *const BlindedToken,
    signed_tokens: *const *const SignedToken,
    unblinded_tokens: *mut *mut UnblindedToken,
    tokens_length: c_int,
    public_key: *const PublicKey,
) -> c_int {
    if !proof.is_null()
        && !tokens.is_null()
        && !blinded_tokens.is_null()
        && !signed_tokens.is_null()
        && !unblinded_tokens.is_null()
        && !public_key.is_null()
    {
        let tokens: &[*const Token] = slice::from_raw_parts(tokens, tokens_length as usize);
        let tokens = tokens.iter().filter_map(|p| p.as_ref());

        let blinded_tokens: &[*const BlindedToken] =
            slice::from_raw_parts(blinded_tokens, tokens_length as usize);
        let blinded_tokens: Vec<BlindedToken> = blinded_tokens.iter().map(|p| **p).collect();

        let signed_tokens: &[*const SignedToken] =
            slice::from_raw_parts(signed_tokens, tokens_length as usize);
        let signed_tokens: Vec<SignedToken> = signed_tokens.iter().map(|p| **p).collect();

        let unblinded_tokens: &mut [*mut UnblindedToken] =
            slice::from_raw_parts_mut(unblinded_tokens, tokens_length as usize);

        match (*proof).verify_and_unblind::<Sha512, _>(
            tokens,
            &blinded_tokens,
            &signed_tokens,
            &*public_key,
        ) {
            Ok(temp_unblinded_tokens) => {
                let temp_unblinded_tokens: Vec<*mut UnblindedToken> = temp_unblinded_tokens
                    .into_iter()
                    .map(|t| Box::into_raw(Box::new(t)))
                    .collect();
                unblinded_tokens.copy_from_slice(&temp_unblinded_tokens[..]);
                return 0;
            }
            Err(err) => {
                if let Some(InternalError::VerifyError) =
                    err.source().unwrap().downcast_ref::<InternalError>()
                {
                    return 1;
                } else {
                    update_last_error(err);
                    return -1;
                }
            }
        }
    }
    update_last_error("Pointer to tokens, blinded tokens, signed tokens, unblinded tokens, proof or public key was null");
    -1
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_embedded_null() {
        unsafe {
            let c_msg1 = "\0hello";
            let c_msg2 = "";

            let key = signing_key_random();

            let token = token_random();
            let blinded_token = token_blind(token);
            let signed_token = signing_key_sign(key, blinded_token);

            let tokens = vec![std::mem::transmute::<*mut Token, *const Token>(token)];
            let blinded_tokens = vec![blinded_token];
            let signed_tokens = vec![signed_token];
            let mut unblinded_tokens: Vec<*mut UnblindedToken> = Vec::with_capacity(1);

            let proof = batch_dleq_proof_new(
                blinded_tokens.as_ptr() as *const *const BlindedToken,
                signed_tokens.as_ptr() as *const *const SignedToken,
                1,
                key,
            );

            batch_dleq_proof_invalid_or_unblind(
                proof,
                tokens.as_ptr(),
                blinded_tokens.as_ptr() as *const *const BlindedToken,
                signed_tokens.as_ptr() as *const *const SignedToken,
                unblinded_tokens.as_mut_ptr(),
                1,
                signing_key_get_public_key(key),
            );
            unblinded_tokens.set_len(1);

            let v_key = unblinded_token_derive_verification_key_sha512(unblinded_tokens[0]);

            let code = verification_key_sign_sha512(v_key, c_msg1.as_ptr(), c_msg1.len());

            assert_ne!(
                verification_key_invalid_sha512(v_key, code, c_msg2.as_ptr(), c_msg2.len()),
                0,
                "A different message should not validate"
            );

            assert_eq!(
                verification_key_invalid_sha512(v_key, code, c_msg1.as_ptr(), c_msg1.len()),
                0,
                "Embedded nulls in the same message should validate"
            );
        }
    }
}
