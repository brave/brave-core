//! APIs for batch trial decryption.

use alloc::vec::Vec; // module is alloc only

use crate::{
    try_compact_note_decryption_inner, try_note_decryption_inner, BatchDomain, EphemeralKeyBytes,
    ShieldedOutput, COMPACT_NOTE_SIZE, ENC_CIPHERTEXT_SIZE,
};

/// Trial decryption of a batch of notes with a set of recipients.
///
/// This is the batched version of [`crate::try_note_decryption`].
///
/// Returns a vector containing the decrypted result for each output,
/// with the same length and in the same order as the outputs were
/// provided, along with the index in the `ivks` slice associated with
/// the IVK that successfully decrypted the output.
#[allow(clippy::type_complexity)]
pub fn try_note_decryption<D: BatchDomain, Output: ShieldedOutput<D, ENC_CIPHERTEXT_SIZE>>(
    ivks: &[D::IncomingViewingKey],
    outputs: &[(D, Output)],
) -> Vec<Option<((D::Note, D::Recipient, D::Memo), usize)>> {
    batch_note_decryption(ivks, outputs, try_note_decryption_inner)
}

/// Trial decryption of a batch of notes for light clients with a set of recipients.
///
/// This is the batched version of [`crate::try_compact_note_decryption`].
///
/// Returns a vector containing the decrypted result for each output,
/// with the same length and in the same order as the outputs were
/// provided, along with the index in the `ivks` slice associated with
/// the IVK that successfully decrypted the output.
#[allow(clippy::type_complexity)]
pub fn try_compact_note_decryption<D: BatchDomain, Output: ShieldedOutput<D, COMPACT_NOTE_SIZE>>(
    ivks: &[D::IncomingViewingKey],
    outputs: &[(D, Output)],
) -> Vec<Option<((D::Note, D::Recipient), usize)>> {
    batch_note_decryption(ivks, outputs, try_compact_note_decryption_inner)
}

fn batch_note_decryption<D: BatchDomain, Output: ShieldedOutput<D, CS>, F, FR, const CS: usize>(
    ivks: &[D::IncomingViewingKey],
    outputs: &[(D, Output)],
    decrypt_inner: F,
) -> Vec<Option<(FR, usize)>>
where
    F: Fn(&D, &D::IncomingViewingKey, &EphemeralKeyBytes, &Output, &D::SymmetricKey) -> Option<FR>,
{
    if ivks.is_empty() {
        return (0..outputs.len()).map(|_| None).collect();
    };

    // Fetch the ephemeral keys for each output, and batch-parse and prepare them.
    let ephemeral_keys = D::batch_epk(outputs.iter().map(|(_, output)| output.ephemeral_key()));

    // Derive the shared secrets for all combinations of (ivk, output).
    // The scalar multiplications cannot benefit from batching.
    let items = ephemeral_keys.iter().flat_map(|(epk, ephemeral_key)| {
        ivks.iter().map(move |ivk| {
            (
                epk.as_ref().map(|epk| D::ka_agree_dec(ivk, epk)),
                ephemeral_key,
            )
        })
    });

    // Run the batch-KDF to obtain the symmetric keys from the shared secrets.
    let keys = D::batch_kdf(items);

    // Finish the trial decryption!
    keys.chunks(ivks.len())
        .zip(ephemeral_keys.iter().zip(outputs.iter()))
        .map(|(key_chunk, ((_, ephemeral_key), (domain, output)))| {
            key_chunk
                .iter()
                .zip(ivks.iter().enumerate())
                .filter_map(|(key, (i, ivk))| {
                    key.as_ref()
                        .and_then(|key| decrypt_inner(domain, ivk, ephemeral_key, output, key))
                        .map(|out| (out, i))
                })
                .next()
        })
        .collect::<Vec<Option<_>>>()
}
