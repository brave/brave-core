//! Structs representing the components within Zcash transactions.

use std::io::{self, Read, Write};

use super::{amount::Amount, GROTH_PROOF_SIZE};

// π_A + π_A' + π_B + π_B' + π_C + π_C' + π_K + π_H
const PHGR_PROOF_SIZE: usize = 33 + 33 + 65 + 33 + 33 + 33 + 33 + 33;

const ZC_NUM_JS_INPUTS: usize = 2;
const ZC_NUM_JS_OUTPUTS: usize = 2;

#[derive(Debug, Clone)]
pub struct Bundle {
    pub joinsplits: Vec<JsDescription>,
    pub joinsplit_pubkey: [u8; 32],
    pub joinsplit_sig: [u8; 64],
}

impl Bundle {
    /// The value balance for the bundle. When this is positive,
    /// its value is added to the transparent value pool; when it
    /// is negative, its value is subtracted from the transparent
    /// value pool.
    pub fn value_balance(&self) -> Option<Amount> {
        self.joinsplits
            .iter()
            .try_fold(Amount::zero(), |total, js| total + js.net_value())
    }
}

#[derive(Clone)]
#[allow(clippy::upper_case_acronyms)]
pub(crate) enum SproutProof {
    Groth([u8; GROTH_PROOF_SIZE]),
    PHGR([u8; PHGR_PROOF_SIZE]),
}

impl std::fmt::Debug for SproutProof {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> Result<(), std::fmt::Error> {
        match self {
            SproutProof::Groth(_) => write!(f, "SproutProof::Groth"),
            SproutProof::PHGR(_) => write!(f, "SproutProof::PHGR"),
        }
    }
}

#[derive(Clone)]
pub struct JsDescription {
    pub(crate) vpub_old: Amount,
    pub(crate) vpub_new: Amount,
    pub(crate) anchor: [u8; 32],
    pub(crate) nullifiers: [[u8; 32]; ZC_NUM_JS_INPUTS],
    pub(crate) commitments: [[u8; 32]; ZC_NUM_JS_OUTPUTS],
    pub(crate) ephemeral_key: [u8; 32],
    pub(crate) random_seed: [u8; 32],
    pub(crate) macs: [[u8; 32]; ZC_NUM_JS_INPUTS],
    pub(crate) proof: SproutProof,
    pub(crate) ciphertexts: [[u8; 601]; ZC_NUM_JS_OUTPUTS],
}

impl std::fmt::Debug for JsDescription {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> Result<(), std::fmt::Error> {
        write!(
            f,
            "JSDescription(
                vpub_old = {:?}, vpub_new = {:?},
                anchor = {:?},
                nullifiers = {:?},
                commitments = {:?},
                ephemeral_key = {:?},
                random_seed = {:?},
                macs = {:?})",
            self.vpub_old,
            self.vpub_new,
            self.anchor,
            self.nullifiers,
            self.commitments,
            self.ephemeral_key,
            self.random_seed,
            self.macs
        )
    }
}

impl JsDescription {
    pub fn read<R: Read>(mut reader: R, use_groth: bool) -> io::Result<Self> {
        // Consensus rule (§4.3): Canonical encoding is enforced here
        let vpub_old = {
            let mut tmp = [0u8; 8];
            reader.read_exact(&mut tmp)?;
            Amount::from_u64_le_bytes(tmp)
        }
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "vpub_old out of range"))?;

        // Consensus rule (§4.3): Canonical encoding is enforced here
        let vpub_new = {
            let mut tmp = [0u8; 8];
            reader.read_exact(&mut tmp)?;
            Amount::from_u64_le_bytes(tmp)
        }
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "vpub_new out of range"))?;

        // Consensus rule (§4.3): One of vpub_old and vpub_new being zero is
        // enforced by CheckTransactionWithoutProofVerification() in zcashd.

        let mut anchor = [0u8; 32];
        reader.read_exact(&mut anchor)?;

        let mut nullifiers = [[0u8; 32]; ZC_NUM_JS_INPUTS];
        nullifiers
            .iter_mut()
            .try_for_each(|nf| reader.read_exact(nf))?;

        let mut commitments = [[0u8; 32]; ZC_NUM_JS_OUTPUTS];
        commitments
            .iter_mut()
            .try_for_each(|cm| reader.read_exact(cm))?;

        // Consensus rule (§4.3): Canonical encoding is enforced by
        // ZCNoteDecryption::decrypt() in zcashd
        let mut ephemeral_key = [0u8; 32];
        reader.read_exact(&mut ephemeral_key)?;

        let mut random_seed = [0u8; 32];
        reader.read_exact(&mut random_seed)?;

        let mut macs = [[0u8; 32]; ZC_NUM_JS_INPUTS];
        macs.iter_mut().try_for_each(|mac| reader.read_exact(mac))?;

        let proof = if use_groth {
            // Consensus rules (§4.3):
            // - Canonical encoding is enforced in librustzcash_sprout_verify()
            // - Proof validity is enforced in librustzcash_sprout_verify()
            let mut proof = [0u8; GROTH_PROOF_SIZE];
            reader.read_exact(&mut proof)?;
            SproutProof::Groth(proof)
        } else {
            // Consensus rules (§4.3):
            // - Canonical encoding is enforced by PHGRProof in zcashd
            // - Proof validity is enforced by JSDescription::Verify() in zcashd
            let mut proof = [0u8; PHGR_PROOF_SIZE];
            reader.read_exact(&mut proof)?;
            SproutProof::PHGR(proof)
        };

        let mut ciphertexts = [[0u8; 601]; ZC_NUM_JS_OUTPUTS];
        ciphertexts
            .iter_mut()
            .try_for_each(|ct| reader.read_exact(ct))?;

        Ok(JsDescription {
            vpub_old,
            vpub_new,
            anchor,
            nullifiers,
            commitments,
            ephemeral_key,
            random_seed,
            macs,
            proof,
            ciphertexts,
        })
    }

    pub fn write<W: Write>(&self, mut writer: W) -> io::Result<()> {
        writer.write_all(&self.vpub_old.to_i64_le_bytes())?;
        writer.write_all(&self.vpub_new.to_i64_le_bytes())?;
        writer.write_all(&self.anchor)?;
        writer.write_all(&self.nullifiers[0])?;
        writer.write_all(&self.nullifiers[1])?;
        writer.write_all(&self.commitments[0])?;
        writer.write_all(&self.commitments[1])?;
        writer.write_all(&self.ephemeral_key)?;
        writer.write_all(&self.random_seed)?;
        writer.write_all(&self.macs[0])?;
        writer.write_all(&self.macs[1])?;

        match &self.proof {
            SproutProof::Groth(p) => writer.write_all(p)?,
            SproutProof::PHGR(p) => writer.write_all(p)?,
        }

        writer.write_all(&self.ciphertexts[0])?;
        writer.write_all(&self.ciphertexts[1])
    }

    /// The net value for the JoinSplit. When this is positive,
    /// its value is added to the transparent value pool; when it
    /// is negative, its value is subtracted from the transparent
    /// value pool.
    pub fn net_value(&self) -> Amount {
        (self.vpub_new - self.vpub_old).expect("difference is in range [-MAX_MONEY..=MAX_MONEY]")
    }
}
