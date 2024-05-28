use std::io::Write;

use blake2b_simd::{Hash as Blake2bHash, Params, State};
use zcash_encoding::Array;

use crate::transaction::{
    components::transparent::{self, TxOut},
    sighash::{
        SignableInput, TransparentAuthorizingContext, SIGHASH_ANYONECANPAY, SIGHASH_MASK,
        SIGHASH_NONE, SIGHASH_SINGLE,
    },
    txid::{
        hash_transparent_txid_data, to_hash, transparent_outputs_hash, transparent_prevout_hash,
        transparent_sequence_hash, ZCASH_TRANSPARENT_HASH_PERSONALIZATION,
    },
    Authorization, TransactionData, TransparentDigests, TxDigests,
};

#[cfg(zcash_unstable = "zfuture")]
use {
    crate::transaction::{components::tze, TzeDigests},
    byteorder::WriteBytesExt,
    zcash_encoding::{CompactSize, Vector},
};

const ZCASH_TRANSPARENT_INPUT_HASH_PERSONALIZATION: &[u8; 16] = b"Zcash___TxInHash";
const ZCASH_TRANSPARENT_AMOUNTS_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxTrAmountsHash";
const ZCASH_TRANSPARENT_SCRIPTS_HASH_PERSONALIZATION: &[u8; 16] = b"ZTxTrScriptsHash";

#[cfg(zcash_unstable = "zfuture")]
const ZCASH_TZE_INPUT_HASH_PERSONALIZATION: &[u8; 16] = b"Zcash__TzeInHash";

fn hasher(personal: &[u8; 16]) -> State {
    Params::new().hash_length(32).personal(personal).to_state()
}

/// Implements [ZIP 244 section S.2](https://zips.z.cash/zip-0244#s-2-transparent-sig-digest).
fn transparent_sig_digest<A: TransparentAuthorizingContext>(
    tx_data: Option<(&transparent::Bundle<A>, &TransparentDigests<Blake2bHash>)>,
    input: &SignableInput<'_>,
) -> Blake2bHash {
    match tx_data {
        // No transparent inputs or outputs.
        None => hash_transparent_txid_data(None),
        // No transparent inputs, or coinbase.
        Some((bundle, txid_digests)) if bundle.is_coinbase() || bundle.vin.is_empty() => {
            hash_transparent_txid_data(Some(txid_digests))
        }
        // Some transparent inputs, and not coinbase.
        Some((bundle, txid_digests)) => {
            let hash_type = input.hash_type();
            let flag_anyonecanpay = hash_type & SIGHASH_ANYONECANPAY != 0;
            let flag_single = hash_type & SIGHASH_MASK == SIGHASH_SINGLE;
            let flag_none = hash_type & SIGHASH_MASK == SIGHASH_NONE;

            let prevouts_digest = if flag_anyonecanpay {
                transparent_prevout_hash::<A>(&[])
            } else {
                txid_digests.prevouts_digest
            };

            let amounts_digest = {
                let mut h = hasher(ZCASH_TRANSPARENT_AMOUNTS_HASH_PERSONALIZATION);
                if !flag_anyonecanpay {
                    Array::write(&mut h, bundle.authorization.input_amounts(), |w, amount| {
                        w.write_all(&amount.to_i64_le_bytes())
                    })
                    .unwrap();
                }
                h.finalize()
            };

            let scripts_digest = {
                let mut h = hasher(ZCASH_TRANSPARENT_SCRIPTS_HASH_PERSONALIZATION);
                if !flag_anyonecanpay {
                    Array::write(
                        &mut h,
                        bundle.authorization.input_scriptpubkeys(),
                        |w, script| script.write(w),
                    )
                    .unwrap();
                }
                h.finalize()
            };

            let sequence_digest = if flag_anyonecanpay {
                transparent_sequence_hash::<A>(&[])
            } else {
                txid_digests.sequence_digest
            };

            let outputs_digest = if let SignableInput::Transparent { index, .. } = input {
                if flag_single {
                    if *index < bundle.vout.len() {
                        transparent_outputs_hash(&[&bundle.vout[*index]])
                    } else {
                        transparent_outputs_hash::<TxOut>(&[])
                    }
                } else if flag_none {
                    transparent_outputs_hash::<TxOut>(&[])
                } else {
                    txid_digests.outputs_digest
                }
            } else {
                txid_digests.outputs_digest
            };

            //S.2g.i:   prevout      (field encoding)
            //S.2g.ii:  value        (8-byte signed little-endian)
            //S.2g.iii: scriptPubKey (field encoding)
            //S.2g.iv:  nSequence    (4-byte unsigned little-endian)
            let mut ch = hasher(ZCASH_TRANSPARENT_INPUT_HASH_PERSONALIZATION);
            if let SignableInput::Transparent {
                index,
                script_pubkey,
                value,
                ..
            } = input
            {
                let txin = &bundle.vin[*index];
                txin.prevout.write(&mut ch).unwrap();
                ch.write_all(&value.to_i64_le_bytes()).unwrap();
                script_pubkey.write(&mut ch).unwrap();
                ch.write_all(&txin.sequence.to_le_bytes()).unwrap();
            }
            let txin_sig_digest = ch.finalize();

            let mut h = hasher(ZCASH_TRANSPARENT_HASH_PERSONALIZATION);
            h.write_all(&[hash_type]).unwrap();
            h.write_all(prevouts_digest.as_bytes()).unwrap();
            h.write_all(amounts_digest.as_bytes()).unwrap();
            h.write_all(scripts_digest.as_bytes()).unwrap();
            h.write_all(sequence_digest.as_bytes()).unwrap();
            h.write_all(outputs_digest.as_bytes()).unwrap();
            h.write_all(txin_sig_digest.as_bytes()).unwrap();
            h.finalize()
        }
    }
}

#[cfg(zcash_unstable = "zfuture")]
fn tze_input_sigdigests<A: tze::Authorization>(
    bundle: &tze::Bundle<A>,
    input: &SignableInput<'_>,
    txid_digests: &TzeDigests<Blake2bHash>,
) -> TzeDigests<Blake2bHash> {
    let mut ch = hasher(ZCASH_TZE_INPUT_HASH_PERSONALIZATION);
    if let SignableInput::Tze {
        index,
        precondition,
        value,
    } = input
    {
        let tzein = &bundle.vin[*index];
        tzein.prevout.write(&mut ch).unwrap();
        CompactSize::write(&mut ch, precondition.extension_id.try_into().unwrap()).unwrap();
        CompactSize::write(&mut ch, precondition.mode.try_into().unwrap()).unwrap();
        Vector::write(&mut ch, &precondition.payload, |w, e| w.write_u8(*e)).unwrap();
        ch.write_all(&value.to_i64_le_bytes()).unwrap();
    }
    let per_input_digest = ch.finalize();

    TzeDigests {
        inputs_digest: txid_digests.inputs_digest,
        outputs_digest: txid_digests.outputs_digest,
        per_input_digest: Some(per_input_digest),
    }
}

/// Implements the [Signature Digest section of ZIP 244](https://zips.z.cash/zip-0244#signature-digest)
pub fn v5_signature_hash<
    TA: TransparentAuthorizingContext,
    A: Authorization<TransparentAuth = TA>,
>(
    tx: &TransactionData<A>,
    signable_input: &SignableInput<'_>,
    txid_parts: &TxDigests<Blake2bHash>,
) -> Blake2bHash {
    // The caller must provide the transparent digests if and only if the transaction has a
    // transparent component.
    assert_eq!(
        tx.transparent_bundle.is_some(),
        txid_parts.transparent_digests.is_some()
    );

    to_hash(
        tx.version,
        tx.consensus_branch_id,
        txid_parts.header_digest,
        transparent_sig_digest(
            tx.transparent_bundle
                .as_ref()
                .zip(txid_parts.transparent_digests.as_ref()),
            signable_input,
        ),
        txid_parts.sapling_digest,
        txid_parts.orchard_digest,
        #[cfg(zcash_unstable = "zfuture")]
        tx.tze_bundle
            .as_ref()
            .zip(txid_parts.tze_digests.as_ref())
            .map(|(bundle, tze_digests)| tze_input_sigdigests(bundle, signable_input, tze_digests))
            .as_ref(),
    )
}
