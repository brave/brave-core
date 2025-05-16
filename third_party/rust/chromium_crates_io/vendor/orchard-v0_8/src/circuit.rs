//! The Orchard Action circuit implementation.

use core::fmt;

use group::{Curve, GroupEncoding};
use halo2_proofs::{
    circuit::{floor_planner, Layouter, Value},
    plonk::{
        self, Advice, BatchVerifier, Column, Constraints, Expression, Instance as InstanceColumn,
        Selector, SingleVerifier,
    },
    poly::Rotation,
    transcript::{Blake2bRead, Blake2bWrite},
};
use memuse::DynamicUsage;
use pasta_curves::{arithmetic::CurveAffine, pallas, vesta};
use rand::RngCore;

use self::{
    commit_ivk::{CommitIvkChip, CommitIvkConfig},
    gadget::{
        add_chip::{AddChip, AddConfig},
        assign_free_advice,
    },
    note_commit::{NoteCommitChip, NoteCommitConfig},
};
use crate::{
    builder::SpendInfo,
    constants::{
        OrchardCommitDomains, OrchardFixedBases, OrchardFixedBasesFull, OrchardHashDomains,
        MERKLE_DEPTH_ORCHARD,
    },
    keys::{
        CommitIvkRandomness, DiversifiedTransmissionKey, NullifierDerivingKey, SpendValidatingKey,
    },
    note::{
        commitment::{NoteCommitTrapdoor, NoteCommitment},
        nullifier::Nullifier,
        ExtractedNoteCommitment, Note, Rho,
    },
    primitives::redpallas::{SpendAuth, VerificationKey},
    spec::NonIdentityPallasPoint,
    tree::{Anchor, MerkleHashOrchard},
    value::{NoteValue, ValueCommitTrapdoor, ValueCommitment},
};
use halo2_gadgets::{
    ecc::{
        chip::{EccChip, EccConfig},
        FixedPoint, NonIdentityPoint, Point, ScalarFixed, ScalarFixedShort, ScalarVar,
    },
    poseidon::{primitives as poseidon, Pow5Chip as PoseidonChip, Pow5Config as PoseidonConfig},
    sinsemilla::{
        chip::{SinsemillaChip, SinsemillaConfig},
        merkle::{
            chip::{MerkleChip, MerkleConfig},
            MerklePath,
        },
    },
    utilities::lookup_range_check::LookupRangeCheckConfig,
};

mod commit_ivk;
pub mod gadget;
mod note_commit;

/// Size of the Orchard circuit.
const K: u32 = 11;

// Absolute offsets for public inputs.
const ANCHOR: usize = 0;
const CV_NET_X: usize = 1;
const CV_NET_Y: usize = 2;
const NF_OLD: usize = 3;
const RK_X: usize = 4;
const RK_Y: usize = 5;
const CMX: usize = 6;
const ENABLE_SPEND: usize = 7;
const ENABLE_OUTPUT: usize = 8;

/// Configuration needed to use the Orchard Action circuit.
#[derive(Clone, Debug)]
pub struct Config {
    primary: Column<InstanceColumn>,
    q_orchard: Selector,
    advices: [Column<Advice>; 10],
    add_config: AddConfig,
    ecc_config: EccConfig<OrchardFixedBases>,
    poseidon_config: PoseidonConfig<pallas::Base, 3, 2>,
    merkle_config_1: MerkleConfig<OrchardHashDomains, OrchardCommitDomains, OrchardFixedBases>,
    merkle_config_2: MerkleConfig<OrchardHashDomains, OrchardCommitDomains, OrchardFixedBases>,
    sinsemilla_config_1:
        SinsemillaConfig<OrchardHashDomains, OrchardCommitDomains, OrchardFixedBases>,
    sinsemilla_config_2:
        SinsemillaConfig<OrchardHashDomains, OrchardCommitDomains, OrchardFixedBases>,
    commit_ivk_config: CommitIvkConfig,
    old_note_commit_config: NoteCommitConfig,
    new_note_commit_config: NoteCommitConfig,
}

/// The Orchard Action circuit.
#[derive(Clone, Debug, Default)]
pub struct Circuit {
    pub(crate) path: Value<[MerkleHashOrchard; MERKLE_DEPTH_ORCHARD]>,
    pub(crate) pos: Value<u32>,
    pub(crate) g_d_old: Value<NonIdentityPallasPoint>,
    pub(crate) pk_d_old: Value<DiversifiedTransmissionKey>,
    pub(crate) v_old: Value<NoteValue>,
    pub(crate) rho_old: Value<Rho>,
    pub(crate) psi_old: Value<pallas::Base>,
    pub(crate) rcm_old: Value<NoteCommitTrapdoor>,
    pub(crate) cm_old: Value<NoteCommitment>,
    pub(crate) alpha: Value<pallas::Scalar>,
    pub(crate) ak: Value<SpendValidatingKey>,
    pub(crate) nk: Value<NullifierDerivingKey>,
    pub(crate) rivk: Value<CommitIvkRandomness>,
    pub(crate) g_d_new: Value<NonIdentityPallasPoint>,
    pub(crate) pk_d_new: Value<DiversifiedTransmissionKey>,
    pub(crate) v_new: Value<NoteValue>,
    pub(crate) psi_new: Value<pallas::Base>,
    pub(crate) rcm_new: Value<NoteCommitTrapdoor>,
    pub(crate) rcv: Value<ValueCommitTrapdoor>,
}

impl Circuit {
    /// This constructor is public to enable creation of custom builders.
    /// If you are not creating a custom builder, use [`Builder`] to compose
    /// and authorize a transaction.
    ///
    /// Constructs a `Circuit` from the following components:
    /// - `spend`: [`SpendInfo`] of the note spent in scope of the action
    /// - `output_note`: a note created in scope of the action
    /// - `alpha`: a scalar used for randomization of the action spend validating key
    /// - `rcv`: trapdoor for the action value commitment
    ///
    /// Returns `None` if the `rho` of the `output_note` is not equal
    /// to the nullifier of the spent note.
    ///
    /// [`SpendInfo`]: crate::builder::SpendInfo
    /// [`Builder`]: crate::builder::Builder
    pub fn from_action_context(
        spend: SpendInfo,
        output_note: Note,
        alpha: pallas::Scalar,
        rcv: ValueCommitTrapdoor,
    ) -> Option<Circuit> {
        (Rho::from_nf_old(spend.note.nullifier(&spend.fvk)) == output_note.rho())
            .then(|| Self::from_action_context_unchecked(spend, output_note, alpha, rcv))
    }

    pub(crate) fn from_action_context_unchecked(
        spend: SpendInfo,
        output_note: Note,
        alpha: pallas::Scalar,
        rcv: ValueCommitTrapdoor,
    ) -> Circuit {
        let sender_address = spend.note.recipient();
        let rho_old = spend.note.rho();
        let psi_old = spend.note.rseed().psi(&rho_old);
        let rcm_old = spend.note.rseed().rcm(&rho_old);

        let rho_new = output_note.rho();
        let psi_new = output_note.rseed().psi(&rho_new);
        let rcm_new = output_note.rseed().rcm(&rho_new);

        Circuit {
            path: Value::known(spend.merkle_path.auth_path()),
            pos: Value::known(spend.merkle_path.position()),
            g_d_old: Value::known(sender_address.g_d()),
            pk_d_old: Value::known(*sender_address.pk_d()),
            v_old: Value::known(spend.note.value()),
            rho_old: Value::known(rho_old),
            psi_old: Value::known(psi_old),
            rcm_old: Value::known(rcm_old),
            cm_old: Value::known(spend.note.commitment()),
            alpha: Value::known(alpha),
            ak: Value::known(spend.fvk.clone().into()),
            nk: Value::known(*spend.fvk.nk()),
            rivk: Value::known(spend.fvk.rivk(spend.scope)),
            g_d_new: Value::known(output_note.recipient().g_d()),
            pk_d_new: Value::known(*output_note.recipient().pk_d()),
            v_new: Value::known(output_note.value()),
            psi_new: Value::known(psi_new),
            rcm_new: Value::known(rcm_new),
            rcv: Value::known(rcv),
        }
    }
}

impl plonk::Circuit<pallas::Base> for Circuit {
    type Config = Config;
    type FloorPlanner = floor_planner::V1;

    fn without_witnesses(&self) -> Self {
        Self::default()
    }

    fn configure(meta: &mut plonk::ConstraintSystem<pallas::Base>) -> Self::Config {
        // Advice columns used in the Orchard circuit.
        let advices = [
            meta.advice_column(),
            meta.advice_column(),
            meta.advice_column(),
            meta.advice_column(),
            meta.advice_column(),
            meta.advice_column(),
            meta.advice_column(),
            meta.advice_column(),
            meta.advice_column(),
            meta.advice_column(),
        ];

        // Constrain v_old - v_new = magnitude * sign    (https://p.z.cash/ZKS:action-cv-net-integrity?partial).
        // Either v_old = 0, or calculated root = anchor (https://p.z.cash/ZKS:action-merkle-path-validity?partial).
        // Constrain v_old = 0 or enable_spends = 1      (https://p.z.cash/ZKS:action-enable-spend).
        // Constrain v_new = 0 or enable_outputs = 1     (https://p.z.cash/ZKS:action-enable-output).
        let q_orchard = meta.selector();
        meta.create_gate("Orchard circuit checks", |meta| {
            let q_orchard = meta.query_selector(q_orchard);
            let v_old = meta.query_advice(advices[0], Rotation::cur());
            let v_new = meta.query_advice(advices[1], Rotation::cur());
            let magnitude = meta.query_advice(advices[2], Rotation::cur());
            let sign = meta.query_advice(advices[3], Rotation::cur());

            let root = meta.query_advice(advices[4], Rotation::cur());
            let anchor = meta.query_advice(advices[5], Rotation::cur());

            let enable_spends = meta.query_advice(advices[6], Rotation::cur());
            let enable_outputs = meta.query_advice(advices[7], Rotation::cur());

            let one = Expression::Constant(pallas::Base::one());

            Constraints::with_selector(
                q_orchard,
                [
                    (
                        "v_old - v_new = magnitude * sign",
                        v_old.clone() - v_new.clone() - magnitude * sign,
                    ),
                    (
                        "Either v_old = 0, or root = anchor",
                        v_old.clone() * (root - anchor),
                    ),
                    (
                        "v_old = 0 or enable_spends = 1",
                        v_old * (one.clone() - enable_spends),
                    ),
                    (
                        "v_new = 0 or enable_outputs = 1",
                        v_new * (one - enable_outputs),
                    ),
                ],
            )
        });

        // Addition of two field elements.
        let add_config = AddChip::configure(meta, advices[7], advices[8], advices[6]);

        // Fixed columns for the Sinsemilla generator lookup table
        let table_idx = meta.lookup_table_column();
        let lookup = (
            table_idx,
            meta.lookup_table_column(),
            meta.lookup_table_column(),
        );

        // Instance column used for public inputs
        let primary = meta.instance_column();
        meta.enable_equality(primary);

        // Permutation over all advice columns.
        for advice in advices.iter() {
            meta.enable_equality(*advice);
        }

        // Poseidon requires four advice columns, while ECC incomplete addition requires
        // six, so we could choose to configure them in parallel. However, we only use a
        // single Poseidon invocation, and we have the rows to accommodate it serially.
        // Instead, we reduce the proof size by sharing fixed columns between the ECC and
        // Poseidon chips.
        let lagrange_coeffs = [
            meta.fixed_column(),
            meta.fixed_column(),
            meta.fixed_column(),
            meta.fixed_column(),
            meta.fixed_column(),
            meta.fixed_column(),
            meta.fixed_column(),
            meta.fixed_column(),
        ];
        let rc_a = lagrange_coeffs[2..5].try_into().unwrap();
        let rc_b = lagrange_coeffs[5..8].try_into().unwrap();

        // Also use the first Lagrange coefficient column for loading global constants.
        // It's free real estate :)
        meta.enable_constant(lagrange_coeffs[0]);

        // We have a lot of free space in the right-most advice columns; use one of them
        // for all of our range checks.
        let range_check = LookupRangeCheckConfig::configure(meta, advices[9], table_idx);

        // Configuration for curve point operations.
        // This uses 10 advice columns and spans the whole circuit.
        let ecc_config =
            EccChip::<OrchardFixedBases>::configure(meta, advices, lagrange_coeffs, range_check);

        // Configuration for the Poseidon hash.
        let poseidon_config = PoseidonChip::configure::<poseidon::P128Pow5T3>(
            meta,
            // We place the state columns after the partial_sbox column so that the
            // pad-and-add region can be laid out more efficiently.
            advices[6..9].try_into().unwrap(),
            advices[5],
            rc_a,
            rc_b,
        );

        // Configuration for a Sinsemilla hash instantiation and a
        // Merkle hash instantiation using this Sinsemilla instance.
        // Since the Sinsemilla config uses only 5 advice columns,
        // we can fit two instances side-by-side.
        let (sinsemilla_config_1, merkle_config_1) = {
            let sinsemilla_config_1 = SinsemillaChip::configure(
                meta,
                advices[..5].try_into().unwrap(),
                advices[6],
                lagrange_coeffs[0],
                lookup,
                range_check,
            );
            let merkle_config_1 = MerkleChip::configure(meta, sinsemilla_config_1.clone());

            (sinsemilla_config_1, merkle_config_1)
        };

        // Configuration for a Sinsemilla hash instantiation and a
        // Merkle hash instantiation using this Sinsemilla instance.
        // Since the Sinsemilla config uses only 5 advice columns,
        // we can fit two instances side-by-side.
        let (sinsemilla_config_2, merkle_config_2) = {
            let sinsemilla_config_2 = SinsemillaChip::configure(
                meta,
                advices[5..].try_into().unwrap(),
                advices[7],
                lagrange_coeffs[1],
                lookup,
                range_check,
            );
            let merkle_config_2 = MerkleChip::configure(meta, sinsemilla_config_2.clone());

            (sinsemilla_config_2, merkle_config_2)
        };

        // Configuration to handle decomposition and canonicity checking
        // for CommitIvk.
        let commit_ivk_config = CommitIvkChip::configure(meta, advices);

        // Configuration to handle decomposition and canonicity checking
        // for NoteCommit_old.
        let old_note_commit_config =
            NoteCommitChip::configure(meta, advices, sinsemilla_config_1.clone());

        // Configuration to handle decomposition and canonicity checking
        // for NoteCommit_new.
        let new_note_commit_config =
            NoteCommitChip::configure(meta, advices, sinsemilla_config_2.clone());

        Config {
            primary,
            q_orchard,
            advices,
            add_config,
            ecc_config,
            poseidon_config,
            merkle_config_1,
            merkle_config_2,
            sinsemilla_config_1,
            sinsemilla_config_2,
            commit_ivk_config,
            old_note_commit_config,
            new_note_commit_config,
        }
    }

    #[allow(non_snake_case)]
    fn synthesize(
        &self,
        config: Self::Config,
        mut layouter: impl Layouter<pallas::Base>,
    ) -> Result<(), plonk::Error> {
        // Load the Sinsemilla generator lookup table used by the whole circuit.
        SinsemillaChip::load(config.sinsemilla_config_1.clone(), &mut layouter)?;

        // Construct the ECC chip.
        let ecc_chip = config.ecc_chip();

        // Witness private inputs that are used across multiple checks.
        let (psi_old, rho_old, cm_old, g_d_old, ak_P, nk, v_old, v_new) = {
            // Witness psi_old
            let psi_old = assign_free_advice(
                layouter.namespace(|| "witness psi_old"),
                config.advices[0],
                self.psi_old,
            )?;

            // Witness rho_old
            let rho_old = assign_free_advice(
                layouter.namespace(|| "witness rho_old"),
                config.advices[0],
                self.rho_old.map(|rho| rho.into_inner()),
            )?;

            // Witness cm_old
            let cm_old = Point::new(
                ecc_chip.clone(),
                layouter.namespace(|| "cm_old"),
                self.cm_old.as_ref().map(|cm| cm.inner().to_affine()),
            )?;

            // Witness g_d_old
            let g_d_old = NonIdentityPoint::new(
                ecc_chip.clone(),
                layouter.namespace(|| "gd_old"),
                self.g_d_old.as_ref().map(|gd| gd.to_affine()),
            )?;

            // Witness ak_P.
            let ak_P: Value<pallas::Point> = self.ak.as_ref().map(|ak| ak.into());
            let ak_P = NonIdentityPoint::new(
                ecc_chip.clone(),
                layouter.namespace(|| "witness ak_P"),
                ak_P.map(|ak_P| ak_P.to_affine()),
            )?;

            // Witness nk.
            let nk = assign_free_advice(
                layouter.namespace(|| "witness nk"),
                config.advices[0],
                self.nk.map(|nk| nk.inner()),
            )?;

            // Witness v_old.
            let v_old = assign_free_advice(
                layouter.namespace(|| "witness v_old"),
                config.advices[0],
                self.v_old,
            )?;

            // Witness v_new.
            let v_new = assign_free_advice(
                layouter.namespace(|| "witness v_new"),
                config.advices[0],
                self.v_new,
            )?;

            (psi_old, rho_old, cm_old, g_d_old, ak_P, nk, v_old, v_new)
        };

        // Merkle path validity check (https://p.z.cash/ZKS:action-merkle-path-validity?partial).
        let root = {
            let path = self
                .path
                .map(|typed_path| typed_path.map(|node| node.inner()));
            let merkle_inputs = MerklePath::construct(
                [config.merkle_chip_1(), config.merkle_chip_2()],
                OrchardHashDomains::MerkleCrh,
                self.pos,
                path,
            );
            let leaf = cm_old.extract_p().inner().clone();
            merkle_inputs.calculate_root(layouter.namespace(|| "Merkle path"), leaf)?
        };

        // Value commitment integrity (https://p.z.cash/ZKS:action-cv-net-integrity?partial).
        let v_net_magnitude_sign = {
            // Witness the magnitude and sign of v_net = v_old - v_new
            let v_net_magnitude_sign = {
                let v_net = self.v_old - self.v_new;
                let magnitude_sign = v_net.map(|v_net| {
                    let (magnitude, sign) = v_net.magnitude_sign();

                    (
                        // magnitude is guaranteed to be an unsigned 64-bit value.
                        // Therefore, we can move it into the base field.
                        pallas::Base::from(magnitude),
                        match sign {
                            crate::value::Sign::Positive => pallas::Base::one(),
                            crate::value::Sign::Negative => -pallas::Base::one(),
                        },
                    )
                });

                let magnitude = assign_free_advice(
                    layouter.namespace(|| "v_net magnitude"),
                    config.advices[9],
                    magnitude_sign.map(|m_s| m_s.0),
                )?;
                let sign = assign_free_advice(
                    layouter.namespace(|| "v_net sign"),
                    config.advices[9],
                    magnitude_sign.map(|m_s| m_s.1),
                )?;
                (magnitude, sign)
            };

            let v_net = ScalarFixedShort::new(
                ecc_chip.clone(),
                layouter.namespace(|| "v_net"),
                v_net_magnitude_sign.clone(),
            )?;
            let rcv = ScalarFixed::new(
                ecc_chip.clone(),
                layouter.namespace(|| "rcv"),
                self.rcv.as_ref().map(|rcv| rcv.inner()),
            )?;

            let cv_net = gadget::value_commit_orchard(
                layouter.namespace(|| "cv_net = ValueCommit^Orchard_rcv(v_net)"),
                ecc_chip.clone(),
                v_net,
                rcv,
            )?;

            // Constrain cv_net to equal public input
            layouter.constrain_instance(cv_net.inner().x().cell(), config.primary, CV_NET_X)?;
            layouter.constrain_instance(cv_net.inner().y().cell(), config.primary, CV_NET_Y)?;

            // Return the magnitude and sign so we can use them in the Orchard gate.
            v_net_magnitude_sign
        };

        // Nullifier integrity (https://p.z.cash/ZKS:action-nullifier-integrity).
        let nf_old = {
            let nf_old = gadget::derive_nullifier(
                layouter.namespace(|| "nf_old = DeriveNullifier_nk(rho_old, psi_old, cm_old)"),
                config.poseidon_chip(),
                config.add_chip(),
                ecc_chip.clone(),
                rho_old.clone(),
                &psi_old,
                &cm_old,
                nk.clone(),
            )?;

            // Constrain nf_old to equal public input
            layouter.constrain_instance(nf_old.inner().cell(), config.primary, NF_OLD)?;

            nf_old
        };

        // Spend authority (https://p.z.cash/ZKS:action-spend-authority)
        {
            let alpha =
                ScalarFixed::new(ecc_chip.clone(), layouter.namespace(|| "alpha"), self.alpha)?;

            // alpha_commitment = [alpha] SpendAuthG
            let (alpha_commitment, _) = {
                let spend_auth_g = OrchardFixedBasesFull::SpendAuthG;
                let spend_auth_g = FixedPoint::from_inner(ecc_chip.clone(), spend_auth_g);
                spend_auth_g.mul(layouter.namespace(|| "[alpha] SpendAuthG"), alpha)?
            };

            // [alpha] SpendAuthG + ak_P
            let rk = alpha_commitment.add(layouter.namespace(|| "rk"), &ak_P)?;

            // Constrain rk to equal public input
            layouter.constrain_instance(rk.inner().x().cell(), config.primary, RK_X)?;
            layouter.constrain_instance(rk.inner().y().cell(), config.primary, RK_Y)?;
        }

        // Diversified address integrity (https://p.z.cash/ZKS:action-addr-integrity?partial).
        let pk_d_old = {
            let ivk = {
                let ak = ak_P.extract_p().inner().clone();
                let rivk = ScalarFixed::new(
                    ecc_chip.clone(),
                    layouter.namespace(|| "rivk"),
                    self.rivk.map(|rivk| rivk.inner()),
                )?;

                gadget::commit_ivk(
                    config.sinsemilla_chip_1(),
                    ecc_chip.clone(),
                    config.commit_ivk_chip(),
                    layouter.namespace(|| "CommitIvk"),
                    ak,
                    nk,
                    rivk,
                )?
            };
            let ivk =
                ScalarVar::from_base(ecc_chip.clone(), layouter.namespace(|| "ivk"), ivk.inner())?;

            // [ivk] g_d_old
            // The scalar value is passed through and discarded.
            let (derived_pk_d_old, _ivk) =
                g_d_old.mul(layouter.namespace(|| "[ivk] g_d_old"), ivk)?;

            // Constrain derived pk_d_old to equal witnessed pk_d_old
            //
            // This equality constraint is technically superfluous, because the assigned
            // value of `derived_pk_d_old` is an equivalent witness. But it's nice to see
            // an explicit connection between circuit-synthesized values, and explicit
            // prover witnesses. We could get the best of both worlds with a write-on-copy
            // abstraction (https://github.com/zcash/halo2/issues/334).
            let pk_d_old = NonIdentityPoint::new(
                ecc_chip.clone(),
                layouter.namespace(|| "witness pk_d_old"),
                self.pk_d_old.map(|pk_d_old| pk_d_old.inner().to_affine()),
            )?;
            derived_pk_d_old
                .constrain_equal(layouter.namespace(|| "pk_d_old equality"), &pk_d_old)?;

            pk_d_old
        };

        // Old note commitment integrity (https://p.z.cash/ZKS:action-cm-old-integrity?partial).
        {
            let rcm_old = ScalarFixed::new(
                ecc_chip.clone(),
                layouter.namespace(|| "rcm_old"),
                self.rcm_old.as_ref().map(|rcm_old| rcm_old.inner()),
            )?;

            // g★_d || pk★_d || i2lebsp_{64}(v) || i2lebsp_{255}(rho) || i2lebsp_{255}(psi)
            let derived_cm_old = gadget::note_commit(
                layouter.namespace(|| {
                    "g★_d || pk★_d || i2lebsp_{64}(v) || i2lebsp_{255}(rho) || i2lebsp_{255}(psi)"
                }),
                config.sinsemilla_chip_1(),
                config.ecc_chip(),
                config.note_commit_chip_old(),
                g_d_old.inner(),
                pk_d_old.inner(),
                v_old.clone(),
                rho_old,
                psi_old,
                rcm_old,
            )?;

            // Constrain derived cm_old to equal witnessed cm_old
            derived_cm_old.constrain_equal(layouter.namespace(|| "cm_old equality"), &cm_old)?;
        }

        // New note commitment integrity (https://p.z.cash/ZKS:action-cmx-new-integrity?partial).
        {
            // Witness g_d_new
            let g_d_new = {
                let g_d_new = self.g_d_new.map(|g_d_new| g_d_new.to_affine());
                NonIdentityPoint::new(
                    ecc_chip.clone(),
                    layouter.namespace(|| "witness g_d_new_star"),
                    g_d_new,
                )?
            };

            // Witness pk_d_new
            let pk_d_new = {
                let pk_d_new = self.pk_d_new.map(|pk_d_new| pk_d_new.inner().to_affine());
                NonIdentityPoint::new(
                    ecc_chip.clone(),
                    layouter.namespace(|| "witness pk_d_new"),
                    pk_d_new,
                )?
            };

            // ρ^new = nf^old
            let rho_new = nf_old.inner().clone();

            // Witness psi_new
            let psi_new = assign_free_advice(
                layouter.namespace(|| "witness psi_new"),
                config.advices[0],
                self.psi_new,
            )?;

            let rcm_new = ScalarFixed::new(
                ecc_chip,
                layouter.namespace(|| "rcm_new"),
                self.rcm_new.as_ref().map(|rcm_new| rcm_new.inner()),
            )?;

            // g★_d || pk★_d || i2lebsp_{64}(v) || i2lebsp_{255}(rho) || i2lebsp_{255}(psi)
            let cm_new = gadget::note_commit(
                layouter.namespace(|| {
                    "g★_d || pk★_d || i2lebsp_{64}(v) || i2lebsp_{255}(rho) || i2lebsp_{255}(psi)"
                }),
                config.sinsemilla_chip_2(),
                config.ecc_chip(),
                config.note_commit_chip_new(),
                g_d_new.inner(),
                pk_d_new.inner(),
                v_new.clone(),
                rho_new,
                psi_new,
                rcm_new,
            )?;

            let cmx = cm_new.extract_p();

            // Constrain cmx to equal public input
            layouter.constrain_instance(cmx.inner().cell(), config.primary, CMX)?;
        }

        // Constrain the remaining Orchard circuit checks.
        layouter.assign_region(
            || "Orchard circuit checks",
            |mut region| {
                v_old.copy_advice(|| "v_old", &mut region, config.advices[0], 0)?;
                v_new.copy_advice(|| "v_new", &mut region, config.advices[1], 0)?;
                v_net_magnitude_sign.0.copy_advice(
                    || "v_net magnitude",
                    &mut region,
                    config.advices[2],
                    0,
                )?;
                v_net_magnitude_sign.1.copy_advice(
                    || "v_net sign",
                    &mut region,
                    config.advices[3],
                    0,
                )?;

                root.copy_advice(|| "calculated root", &mut region, config.advices[4], 0)?;
                region.assign_advice_from_instance(
                    || "pub input anchor",
                    config.primary,
                    ANCHOR,
                    config.advices[5],
                    0,
                )?;

                region.assign_advice_from_instance(
                    || "enable spends",
                    config.primary,
                    ENABLE_SPEND,
                    config.advices[6],
                    0,
                )?;

                region.assign_advice_from_instance(
                    || "enable outputs",
                    config.primary,
                    ENABLE_OUTPUT,
                    config.advices[7],
                    0,
                )?;

                config.q_orchard.enable(&mut region, 0)
            },
        )?;

        Ok(())
    }
}

/// The verifying key for the Orchard Action circuit.
#[derive(Debug)]
pub struct VerifyingKey {
    pub(crate) params: halo2_proofs::poly::commitment::Params<vesta::Affine>,
    pub(crate) vk: plonk::VerifyingKey<vesta::Affine>,
}

impl VerifyingKey {
    /// Builds the verifying key.
    pub fn build() -> Self {
        let params = halo2_proofs::poly::commitment::Params::new(K);
        let circuit: Circuit = Default::default();

        let vk = plonk::keygen_vk(&params, &circuit).unwrap();

        VerifyingKey { params, vk }
    }
}

/// The proving key for the Orchard Action circuit.
#[derive(Debug)]
pub struct ProvingKey {
    params: halo2_proofs::poly::commitment::Params<vesta::Affine>,
    pk: plonk::ProvingKey<vesta::Affine>,
}

impl ProvingKey {
    /// Builds the proving key.
    pub fn build() -> Self {
        let params = halo2_proofs::poly::commitment::Params::new(K);
        let circuit: Circuit = Default::default();

        let vk = plonk::keygen_vk(&params, &circuit).unwrap();
        let pk = plonk::keygen_pk(&params, vk, &circuit).unwrap();

        ProvingKey { params, pk }
    }
}

/// Public inputs to the Orchard Action circuit.
#[derive(Clone, Debug)]
pub struct Instance {
    pub(crate) anchor: Anchor,
    pub(crate) cv_net: ValueCommitment,
    pub(crate) nf_old: Nullifier,
    pub(crate) rk: VerificationKey<SpendAuth>,
    pub(crate) cmx: ExtractedNoteCommitment,
    pub(crate) enable_spend: bool,
    pub(crate) enable_output: bool,
}

impl Instance {
    /// Constructs an [`Instance`] from its constituent parts.
    ///
    /// This API can be used in combination with [`Proof::verify`] to build verification
    /// pipelines for many proofs, where you don't want to pass around the full bundle.
    /// Use [`Bundle::verify_proof`] instead if you have the full bundle.
    ///
    /// [`Bundle::verify_proof`]: crate::Bundle::verify_proof
    pub fn from_parts(
        anchor: Anchor,
        cv_net: ValueCommitment,
        nf_old: Nullifier,
        rk: VerificationKey<SpendAuth>,
        cmx: ExtractedNoteCommitment,
        enable_spend: bool,
        enable_output: bool,
    ) -> Self {
        Instance {
            anchor,
            cv_net,
            nf_old,
            rk,
            cmx,
            enable_spend,
            enable_output,
        }
    }

    fn to_halo2_instance(&self) -> [[vesta::Scalar; 9]; 1] {
        let mut instance = [vesta::Scalar::zero(); 9];

        instance[ANCHOR] = self.anchor.inner();
        instance[CV_NET_X] = self.cv_net.x();
        instance[CV_NET_Y] = self.cv_net.y();
        instance[NF_OLD] = self.nf_old.0;

        let rk = pallas::Point::from_bytes(&self.rk.clone().into())
            .unwrap()
            .to_affine()
            .coordinates()
            .unwrap();

        instance[RK_X] = *rk.x();
        instance[RK_Y] = *rk.y();
        instance[CMX] = self.cmx.inner();
        instance[ENABLE_SPEND] = vesta::Scalar::from(u64::from(self.enable_spend));
        instance[ENABLE_OUTPUT] = vesta::Scalar::from(u64::from(self.enable_output));

        [instance]
    }
}

/// A proof of the validity of an Orchard [`Bundle`].
///
/// [`Bundle`]: crate::bundle::Bundle
#[derive(Clone)]
pub struct Proof(Vec<u8>);

impl fmt::Debug for Proof {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if f.alternate() {
            f.debug_tuple("Proof").field(&self.0).finish()
        } else {
            // By default, only show the proof length, not its contents.
            f.debug_tuple("Proof")
                .field(&format_args!("{} bytes", self.0.len()))
                .finish()
        }
    }
}

impl AsRef<[u8]> for Proof {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

impl DynamicUsage for Proof {
    fn dynamic_usage(&self) -> usize {
        self.0.dynamic_usage()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        self.0.dynamic_usage_bounds()
    }
}

impl Proof {
    /// Creates a proof for the given circuits and instances.
    pub fn create(
        pk: &ProvingKey,
        circuits: &[Circuit],
        instances: &[Instance],
        mut rng: impl RngCore,
    ) -> Result<Self, plonk::Error> {
        let instances: Vec<_> = instances.iter().map(|i| i.to_halo2_instance()).collect();
        let instances: Vec<Vec<_>> = instances
            .iter()
            .map(|i| i.iter().map(|c| &c[..]).collect())
            .collect();
        let instances: Vec<_> = instances.iter().map(|i| &i[..]).collect();

        let mut transcript = Blake2bWrite::<_, vesta::Affine, _>::init(vec![]);
        plonk::create_proof(
            &pk.params,
            &pk.pk,
            circuits,
            &instances,
            &mut rng,
            &mut transcript,
        )?;
        Ok(Proof(transcript.finalize()))
    }

    /// Verifies this proof with the given instances.
    pub fn verify(&self, vk: &VerifyingKey, instances: &[Instance]) -> Result<(), plonk::Error> {
        let instances: Vec<_> = instances.iter().map(|i| i.to_halo2_instance()).collect();
        let instances: Vec<Vec<_>> = instances
            .iter()
            .map(|i| i.iter().map(|c| &c[..]).collect())
            .collect();
        let instances: Vec<_> = instances.iter().map(|i| &i[..]).collect();

        let strategy = SingleVerifier::new(&vk.params);
        let mut transcript = Blake2bRead::init(&self.0[..]);
        plonk::verify_proof(&vk.params, &vk.vk, strategy, &instances, &mut transcript)
    }

    /// Adds this proof to the given batch for verification with the given instances.
    ///
    /// Use this API if you want more control over how proof batches are processed. If you
    /// just want to batch-validate Orchard bundles, use [`bundle::BatchValidator`].
    ///
    /// [`bundle::BatchValidator`]: crate::bundle::BatchValidator
    pub fn add_to_batch(&self, batch: &mut BatchVerifier<vesta::Affine>, instances: Vec<Instance>) {
        let instances = instances
            .iter()
            .map(|i| {
                i.to_halo2_instance()
                    .into_iter()
                    .map(|c| c.into_iter().collect())
                    .collect()
            })
            .collect();

        batch.add_proof(instances, self.0.clone());
    }

    /// Constructs a new Proof value.
    pub fn new(bytes: Vec<u8>) -> Self {
        Proof(bytes)
    }
}

#[cfg(test)]
mod tests {
    use core::iter;

    use ff::Field;
    use halo2_proofs::{circuit::Value, dev::MockProver};
    use pasta_curves::pallas;
    use rand::{rngs::OsRng, RngCore};

    use super::{Circuit, Instance, Proof, ProvingKey, VerifyingKey, K};
    use crate::{
        keys::SpendValidatingKey,
        note::{Note, Rho},
        tree::MerklePath,
        value::{ValueCommitTrapdoor, ValueCommitment},
    };

    fn generate_circuit_instance<R: RngCore>(mut rng: R) -> (Circuit, Instance) {
        let (_, fvk, spent_note) = Note::dummy(&mut rng, None);

        let sender_address = spent_note.recipient();
        let nk = *fvk.nk();
        let rivk = fvk.rivk(fvk.scope_for_address(&spent_note.recipient()).unwrap());
        let nf_old = spent_note.nullifier(&fvk);
        let rho = Rho::from_nf_old(nf_old);
        let ak: SpendValidatingKey = fvk.into();
        let alpha = pallas::Scalar::random(&mut rng);
        let rk = ak.randomize(&alpha);

        let (_, _, output_note) = Note::dummy(&mut rng, Some(rho));
        let cmx = output_note.commitment().into();

        let value = spent_note.value() - output_note.value();
        let rcv = ValueCommitTrapdoor::random(&mut rng);
        let cv_net = ValueCommitment::derive(value, rcv.clone());

        let path = MerklePath::dummy(&mut rng);
        let anchor = path.root(spent_note.commitment().into());

        (
            Circuit {
                path: Value::known(path.auth_path()),
                pos: Value::known(path.position()),
                g_d_old: Value::known(sender_address.g_d()),
                pk_d_old: Value::known(*sender_address.pk_d()),
                v_old: Value::known(spent_note.value()),
                rho_old: Value::known(spent_note.rho()),
                psi_old: Value::known(spent_note.rseed().psi(&spent_note.rho())),
                rcm_old: Value::known(spent_note.rseed().rcm(&spent_note.rho())),
                cm_old: Value::known(spent_note.commitment()),
                alpha: Value::known(alpha),
                ak: Value::known(ak),
                nk: Value::known(nk),
                rivk: Value::known(rivk),
                g_d_new: Value::known(output_note.recipient().g_d()),
                pk_d_new: Value::known(*output_note.recipient().pk_d()),
                v_new: Value::known(output_note.value()),
                psi_new: Value::known(output_note.rseed().psi(&output_note.rho())),
                rcm_new: Value::known(output_note.rseed().rcm(&output_note.rho())),
                rcv: Value::known(rcv),
            },
            Instance {
                anchor,
                cv_net,
                nf_old,
                rk,
                cmx,
                enable_spend: true,
                enable_output: true,
            },
        )
    }

    // TODO: recast as a proptest
    #[test]
    fn round_trip() {
        let mut rng = OsRng;

        let (circuits, instances): (Vec<_>, Vec<_>) = iter::once(())
            .map(|()| generate_circuit_instance(&mut rng))
            .unzip();

        let vk = VerifyingKey::build();

        // Test that the pinned verification key (representing the circuit)
        // is as expected.
        {
            // panic!("{:#?}", vk.vk.pinned());
            assert_eq!(
                format!("{:#?}\n", vk.vk.pinned()),
                include_str!("circuit_description").replace("\r\n", "\n")
            );
        }

        // Test that the proof size is as expected.
        let expected_proof_size = {
            let circuit_cost =
                halo2_proofs::dev::CircuitCost::<pasta_curves::vesta::Point, _>::measure(
                    K,
                    &circuits[0],
                );
            assert_eq!(usize::from(circuit_cost.proof_size(1)), 4992);
            assert_eq!(usize::from(circuit_cost.proof_size(2)), 7264);
            usize::from(circuit_cost.proof_size(instances.len()))
        };

        for (circuit, instance) in circuits.iter().zip(instances.iter()) {
            assert_eq!(
                MockProver::run(
                    K,
                    circuit,
                    instance
                        .to_halo2_instance()
                        .iter()
                        .map(|p| p.to_vec())
                        .collect()
                )
                .unwrap()
                .verify(),
                Ok(())
            );
        }

        let pk = ProvingKey::build();
        let proof = Proof::create(&pk, &circuits, &instances, &mut rng).unwrap();
        assert!(proof.verify(&vk, &instances).is_ok());
        assert_eq!(proof.0.len(), expected_proof_size);
    }

    #[test]
    fn serialized_proof_test_case() {
        use std::io::{Read, Write};

        let vk = VerifyingKey::build();

        fn write_test_case<W: Write>(
            mut w: W,
            instance: &Instance,
            proof: &Proof,
        ) -> std::io::Result<()> {
            w.write_all(&instance.anchor.to_bytes())?;
            w.write_all(&instance.cv_net.to_bytes())?;
            w.write_all(&instance.nf_old.to_bytes())?;
            w.write_all(&<[u8; 32]>::from(instance.rk.clone()))?;
            w.write_all(&instance.cmx.to_bytes())?;
            w.write_all(&[
                u8::from(instance.enable_spend),
                u8::from(instance.enable_output),
            ])?;

            w.write_all(proof.as_ref())?;
            Ok(())
        }

        fn read_test_case<R: Read>(mut r: R) -> std::io::Result<(Instance, Proof)> {
            let read_32_bytes = |r: &mut R| {
                let mut ret = [0u8; 32];
                r.read_exact(&mut ret).unwrap();
                ret
            };
            let read_bool = |r: &mut R| {
                let mut byte = [0u8; 1];
                r.read_exact(&mut byte).unwrap();
                match byte {
                    [0] => false,
                    [1] => true,
                    _ => panic!("Unexpected non-boolean byte"),
                }
            };

            let anchor = crate::Anchor::from_bytes(read_32_bytes(&mut r)).unwrap();
            let cv_net = ValueCommitment::from_bytes(&read_32_bytes(&mut r)).unwrap();
            let nf_old = crate::note::Nullifier::from_bytes(&read_32_bytes(&mut r)).unwrap();
            let rk = read_32_bytes(&mut r).try_into().unwrap();
            let cmx =
                crate::note::ExtractedNoteCommitment::from_bytes(&read_32_bytes(&mut r)).unwrap();
            let enable_spend = read_bool(&mut r);
            let enable_output = read_bool(&mut r);
            let instance =
                Instance::from_parts(anchor, cv_net, nf_old, rk, cmx, enable_spend, enable_output);

            let mut proof_bytes = vec![];
            r.read_to_end(&mut proof_bytes)?;
            let proof = Proof::new(proof_bytes);

            Ok((instance, proof))
        }

        if std::env::var_os("ORCHARD_CIRCUIT_TEST_GENERATE_NEW_PROOF").is_some() {
            let create_proof = || -> std::io::Result<()> {
                let mut rng = OsRng;

                let (circuit, instance) = generate_circuit_instance(OsRng);
                let instances = &[instance.clone()];

                let pk = ProvingKey::build();
                let proof = Proof::create(&pk, &[circuit], instances, &mut rng).unwrap();
                assert!(proof.verify(&vk, instances).is_ok());

                let file = std::fs::File::create("circuit_proof_test_case.bin")?;
                write_test_case(file, &instance, &proof)
            };
            create_proof().expect("should be able to write new proof");
        }

        // Parse the hardcoded proof test case.
        let (instance, proof) = {
            let test_case_bytes = include_bytes!("circuit_proof_test_case.bin");
            read_test_case(&test_case_bytes[..]).expect("proof must be valid")
        };
        assert_eq!(proof.0.len(), 4992);

        assert!(proof.verify(&vk, &[instance]).is_ok());
    }

    #[cfg(feature = "dev-graph")]
    #[test]
    fn print_action_circuit() {
        use plotters::prelude::*;

        let root = BitMapBackend::new("action-circuit-layout.png", (1024, 768)).into_drawing_area();
        root.fill(&WHITE).unwrap();
        let root = root
            .titled("Orchard Action Circuit", ("sans-serif", 60))
            .unwrap();

        let circuit = Circuit {
            path: Value::unknown(),
            pos: Value::unknown(),
            g_d_old: Value::unknown(),
            pk_d_old: Value::unknown(),
            v_old: Value::unknown(),
            rho_old: Value::unknown(),
            psi_old: Value::unknown(),
            rcm_old: Value::unknown(),
            cm_old: Value::unknown(),
            alpha: Value::unknown(),
            ak: Value::unknown(),
            nk: Value::unknown(),
            rivk: Value::unknown(),
            g_d_new: Value::unknown(),
            pk_d_new: Value::unknown(),
            v_new: Value::unknown(),
            psi_new: Value::unknown(),
            rcm_new: Value::unknown(),
            rcv: Value::unknown(),
        };
        halo2_proofs::dev::CircuitLayout::default()
            .show_labels(false)
            .view_height(0..(1 << 11))
            .render(K, &circuit, &root)
            .unwrap();
    }
}
