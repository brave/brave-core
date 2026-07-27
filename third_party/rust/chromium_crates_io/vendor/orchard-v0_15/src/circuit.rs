//! The Orchard Action circuit implementation.

use alloc::vec::Vec;

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
    bundle::Flags,
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
        CircuitVersion, FixedPoint, NonIdentityPoint, Point, ScalarFixed, ScalarFixedShort,
        ScalarVar,
    },
    poseidon::{primitives as poseidon, Pow5Chip as PoseidonChip, Pow5Config as PoseidonConfig},
    sinsemilla::{
        chip::{SinsemillaChip, SinsemillaConfig},
        merkle::{
            chip::{MerkleChip, MerkleConfig},
            MerklePath,
        },
    },
    utilities::lookup_range_check::{LookupRangeCheck, LookupRangeCheckConfig},
};

#[cfg(not(feature = "unstable-voting-circuits"))]
mod commit_ivk;
#[cfg(feature = "unstable-voting-circuits")]
pub mod commit_ivk;
pub mod gadget;
#[cfg(not(feature = "unstable-voting-circuits"))]
mod note_commit;
#[cfg(feature = "unstable-voting-circuits")]
pub mod note_commit;

pub use crate::Proof;

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
const DISABLE_CROSS_ADDRESS: usize = 9;

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

/// Selects which version of the Orchard Action circuit to build.
///
/// [`FixedPostNu6_2`] and [`InsecurePreNu6_2`] produce different verifying keys: the fixed
/// circuit anchors the variable-base scalar-multiplication base (see `halo2_gadgets`), while
/// the pre-NU6.2 one does not. [`PostNu6_3`] extends the fixed circuit by enforcing the
/// same-address check, i.e. `(g_d^old, pk_d^old) = (g_d^new, pk_d^new)`, when the
/// boolean `disableCrossAddress` public input is set.
///
/// This is a runtime value rather than a type parameter: it is carried in [`Circuit`] and
/// chosen when building a [`ProvingKey`] or [`VerifyingKey`], so the circuit version can be
/// threaded dynamically (e.g. across an FFI boundary).
///
/// Please note that the public exposure of APIs using `InsecurePreNu6_2` is intentional,
/// and is strictly necessary for verifying the block chain from NU5 activation and for
/// creating proofs needed by tests that operate at past epochs. These APIs cannot be
/// used accidentally without passing an `OrchardCircuitVersion` that is clearly labelled
/// "insecure". This is not a security vulnerability.
///
/// [`FixedPostNu6_2`]: OrchardCircuitVersion::FixedPostNu6_2
/// [`InsecurePreNu6_2`]: OrchardCircuitVersion::InsecurePreNu6_2
/// [`PostNu6_3`]: OrchardCircuitVersion::PostNu6_3
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum OrchardCircuitVersion {
    /// The insecure pre-NU6.2 circuit, in which the variable-base scalar-multiplication base
    /// is not anchored to the real base. For reconstructing the historical (NU5..NU6.2)
    /// verifying key only — never for proving or current verification.
    InsecurePreNu6_2,
    /// The fixed circuit, active from NU6.2 onward. Used for all current network proving and
    /// verification.
    FixedPostNu6_2,
    /// The post-NU 6.3 circuit. This uses the fixed circuit with additional constraints
    /// enforcing the `disableCrossAddress` public input.
    PostNu6_3,
}

impl OrchardCircuitVersion {
    /// Whether this circuit version enforces the `disableCrossAddress` public input.
    ///
    /// Statements with `disableCrossAddress = 1` can be proven and verified only with
    /// keys for a circuit version that constrains the flag. [`PostNu6_3`] constrains it;
    /// older circuit versions leave it unconstrained, so they cannot enforce — and must
    /// not be asked to attest to — the restriction.
    ///
    /// [`PostNu6_3`]: OrchardCircuitVersion::PostNu6_3
    pub fn supports_cross_address_restriction(self) -> bool {
        match self {
            OrchardCircuitVersion::InsecurePreNu6_2 | OrchardCircuitVersion::FixedPostNu6_2 => {
                false
            }
            OrchardCircuitVersion::PostNu6_3 => true,
        }
    }

    /// The corresponding `halo2_gadgets` variable-base scalar-mul circuit version.
    fn halo2_version(self) -> CircuitVersion {
        match self {
            OrchardCircuitVersion::InsecurePreNu6_2 => CircuitVersion::InsecureUnanchoredBase,
            OrchardCircuitVersion::FixedPostNu6_2 | OrchardCircuitVersion::PostNu6_3 => {
                CircuitVersion::AnchoredBase
            }
        }
    }
}

/// The Orchard Action circuit.
///
/// The `circuit_version` field selects which circuit to build. Callers must choose it
/// explicitly instead of relying on a default.
#[derive(Clone, Debug)]
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
    pub(crate) circuit_version: OrchardCircuitVersion,
}

impl Circuit {
    /// Returns an empty circuit with all private witnesses unknown.
    ///
    /// This is used for circuit shape-dependent operations, such as generating keys
    /// or rendering the circuit layout, where witness values are not required but the
    /// selected circuit version still determines the configured constraints.
    fn empty(circuit_version: OrchardCircuitVersion) -> Self {
        Circuit {
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
            circuit_version,
        }
    }

    /// This constructor is public to enable creation of custom builders.
    /// If you are not creating a custom builder, use [`Builder`] to compose
    /// and authorize a transaction.
    ///
    /// Constructs a `Circuit` for the given `circuit_version` from the following components:
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
        circuit_version: OrchardCircuitVersion,
    ) -> Option<Circuit> {
        (Rho::from_nf_old(spend.note.nullifier(&spend.fvk)) == output_note.rho()).then(|| {
            Self::from_action_context_unchecked(spend, output_note, alpha, rcv, circuit_version)
        })
    }

    pub(crate) fn from_action_context_unchecked(
        spend: SpendInfo,
        output_note: Note,
        alpha: pallas::Scalar,
        rcv: ValueCommitTrapdoor,
        circuit_version: OrchardCircuitVersion,
    ) -> Circuit {
        let sender_address = spend.note.recipient();
        let rho_old = spend.note.rho();
        let psi_old = spend.note.psi();
        let rcm_old = spend.note.rcm();

        let psi_new = output_note.psi();
        let rcm_new = output_note.rcm();

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
            circuit_version,
        }
    }
}

impl Config {
    /// Configures the Orchard Action constraint system shared by every circuit version.
    fn configure(meta: &mut plonk::ConstraintSystem<pallas::Base>) -> Self {
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
        // Constrain v_old = 0 or enable_spend = 1       (https://p.z.cash/ZKS:action-enable-spend).
        // Constrain v_new = 0 or enable_output = 1      (https://p.z.cash/ZKS:action-enable-output).
        //
        // This gate is also reused for the same-address check; see
        // [`Circuit::synthesize_cross_address_checks`].
        let q_orchard = meta.selector();
        meta.create_gate("Orchard circuit checks", |meta| {
            let q_orchard = meta.query_selector(q_orchard);
            let v_old = meta.query_advice(advices[0], Rotation::cur());
            let v_new = meta.query_advice(advices[1], Rotation::cur());
            let magnitude = meta.query_advice(advices[2], Rotation::cur());
            let sign = meta.query_advice(advices[3], Rotation::cur());

            let root = meta.query_advice(advices[4], Rotation::cur());
            let anchor = meta.query_advice(advices[5], Rotation::cur());

            let enable_spend = meta.query_advice(advices[6], Rotation::cur());
            let enable_output = meta.query_advice(advices[7], Rotation::cur());

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
                        "v_old = 0 or enable_spend = 1",
                        v_old * (one.clone() - enable_spend),
                    ),
                    (
                        "v_new = 0 or enable_output = 1",
                        v_new * (one - enable_output),
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
                false,
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
                false,
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
}

/// Cells carrying the addresses of an action's spent and newly created notes, returned
/// from the shared synthesis logic so that circuit versions can impose additional
/// constraints on them.
struct AddressPoints {
    g_d_old: NonIdentityPoint<pallas::Affine, EccChip<OrchardFixedBases>>,
    pk_d_old: NonIdentityPoint<pallas::Affine, EccChip<OrchardFixedBases>>,
    g_d_new: NonIdentityPoint<pallas::Affine, EccChip<OrchardFixedBases>>,
    pk_d_new: NonIdentityPoint<pallas::Affine, EccChip<OrchardFixedBases>>,
}

impl Circuit {
    /// Synthesizes the Orchard Action checks common to every circuit version,
    /// parameterized by `self.circuit_version`, returning the cells carrying the old
    /// and new note addresses so that circuit versions can impose additional
    /// constraints on them.
    #[allow(non_snake_case)]
    fn synthesize_base(
        &self,
        config: &Config,
        layouter: &mut impl Layouter<pallas::Base>,
    ) -> Result<AddressPoints, plonk::Error> {
        // Load the Sinsemilla generator lookup table used by the whole circuit.
        SinsemillaChip::load(config.sinsemilla_config_1.clone(), layouter)?;

        // Construct the ECC chip.
        let ecc_chip = config.ecc_chip(self.circuit_version.halo2_version());

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
                config.ecc_chip(self.circuit_version.halo2_version()),
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

        // Witness g_d_new, used in the new note commitment integrity check below and,
        // for the post-NU 6.3 circuit, in the cross-address checks.
        let g_d_new = {
            let g_d_new = self.g_d_new.map(|g_d_new| g_d_new.to_affine());
            NonIdentityPoint::new(
                ecc_chip.clone(),
                layouter.namespace(|| "witness g_d_new_star"),
                g_d_new,
            )?
        };

        // Witness pk_d_new, used in the new note commitment integrity check below and,
        // for the post-NU 6.3 circuit, in the cross-address checks.
        let pk_d_new = {
            let pk_d_new = self.pk_d_new.map(|pk_d_new| pk_d_new.inner().to_affine());
            NonIdentityPoint::new(
                ecc_chip.clone(),
                layouter.namespace(|| "witness pk_d_new"),
                pk_d_new,
            )?
        };

        // New note commitment integrity (https://p.z.cash/ZKS:action-cmx-new-integrity?partial).
        {
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
                config.ecc_chip(self.circuit_version.halo2_version()),
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
                    || "enable spend",
                    config.primary,
                    ENABLE_SPEND,
                    config.advices[6],
                    0,
                )?;

                region.assign_advice_from_instance(
                    || "enable output",
                    config.primary,
                    ENABLE_OUTPUT,
                    config.advices[7],
                    0,
                )?;

                config.q_orchard.enable(&mut region, 0)
            },
        )?;

        Ok(AddressPoints {
            g_d_old,
            pk_d_old,
            g_d_new,
            pk_d_new,
        })
    }

    /// Enforces the post-NU 6.3 cross-address restriction for one action: when
    /// `disableCrossAddress` is nonzero, the spent note and output note must be
    /// addressed to the same expanded receiver, meaning equal `(g_d, pk_d)`.
    ///
    /// This reuses the existing "Orchard circuit checks" gate instead of adding a
    /// new gate. The gate already has a product constraint,
    /// `v_old * (root - anchor) = 0`, with exactly the shape needed for
    /// `disableCrossAddress * (old_coord - new_coord) = 0`.
    ///
    /// The post-NU 6.3 circuit enables that gate on four extra rows, one per affine coordinate of
    /// `(g_d, pk_d)`, with each row wired as:
    ///
    /// ```text
    /// v_old          <- disableCrossAddress
    /// v_new          <- 0 (constant)
    /// magnitude      <- disableCrossAddress
    /// sign           <- 1 (constant)
    /// root           <- old coordinate
    /// anchor         <- new coordinate
    /// enable_spend   <- 1 (constant)
    /// enable_output  <- 1 (constant)
    /// ```
    ///
    /// With this layout, the gate constraints become:
    ///
    /// ```text
    /// v_old - v_new = magnitude * sign  ->  disableCrossAddress - 0 = disableCrossAddress * 1
    /// v_old * (root - anchor) = 0       ->  disableCrossAddress * (old_coord - new_coord) = 0
    /// v_old * (1 - enable_spend) = 0    ->  disableCrossAddress * (1 - 1) = 0
    /// v_new * (1 - enable_output) = 0   ->  0 * (1 - 1) = 0
    /// ```
    ///
    /// The second line is the actual cross-address check. Any nonzero
    /// `disableCrossAddress` value forces each old coordinate to equal the
    /// corresponding new coordinate. The public API encodes `disableCrossAddress`
    /// as 0 or 1, but this algebra does not rely on a boolean constraint.
    ///
    /// The two otherwise-unused advice columns are also filled with copies of
    /// `disableCrossAddress` so these rows occupy every advice column; that prevents
    /// the floor planner from overlapping another selector-enabled region with the
    /// check rows.
    fn synthesize_cross_address_checks(
        config: &Config,
        layouter: &mut impl Layouter<pallas::Base>,
        addrs: &AddressPoints,
    ) -> Result<(), plonk::Error> {
        let AddressPoints {
            g_d_old,
            pk_d_old,
            g_d_new,
            pk_d_new,
        } = addrs;

        layouter.assign_region(
            || "post-NU 6.3 cross-address checks",
            |mut region| {
                let coordinate_checks = [
                    ("g_d x", g_d_old.inner().x(), g_d_new.inner().x()),
                    ("g_d y", g_d_old.inner().y(), g_d_new.inner().y()),
                    ("pk_d x", pk_d_old.inner().x(), pk_d_new.inner().x()),
                    ("pk_d y", pk_d_old.inner().y(), pk_d_new.inner().y()),
                ];

                for (offset, (label, old_coord, new_coord)) in
                    coordinate_checks.into_iter().enumerate()
                {
                    // Copy disableCrossAddress from the public input at
                    // primary[DISABLE_CROSS_ADDRESS] into advices[0] for this
                    // coordinate-check row.
                    let cross_address_disabled = region.assign_advice_from_instance(
                        || "disableCrossAddress",
                        config.primary,
                        DISABLE_CROSS_ADDRESS,
                        config.advices[0],
                        offset,
                    )?;

                    // Fill the v_new, magnitude, and sign cells so the reused
                    // value-balance constraint reads:
                    // disableCrossAddress - 0 = disableCrossAddress * 1.
                    region.assign_advice_from_constant(
                        || "zero",
                        config.advices[1],
                        offset,
                        pallas::Base::zero(),
                    )?;
                    cross_address_disabled.copy_advice(
                        || "disableCrossAddress magnitude",
                        &mut region,
                        config.advices[2],
                        offset,
                    )?;
                    region.assign_advice_from_constant(
                        || "positive sign",
                        config.advices[3],
                        offset,
                        pallas::Base::one(),
                    )?;

                    // Copy the old coordinate into the gate's root cell and the
                    // new coordinate into its anchor cell for the equality check.
                    old_coord.copy_advice(
                        || format!("old {label}"),
                        &mut region,
                        config.advices[4],
                        offset,
                    )?;
                    new_coord.copy_advice(
                        || format!("new {label}"),
                        &mut region,
                        config.advices[5],
                        offset,
                    )?;

                    // Set both enable flags to one so the unrelated enable checks
                    // in q_orchard are neutralized on these rows.
                    region.assign_advice_from_constant(
                        || "one (neutralize enable_spend check)",
                        config.advices[6],
                        offset,
                        pallas::Base::one(),
                    )?;
                    region.assign_advice_from_constant(
                        || "one (neutralize enable_output check)",
                        config.advices[7],
                        offset,
                        pallas::Base::one(),
                    )?;

                    // Occupy the otherwise-unused rightmost advice columns so the
                    // floor planner cannot lay out another region (and enable its
                    // gate) on these rows.
                    cross_address_disabled.copy_advice(
                        || "disableCrossAddress padding",
                        &mut region,
                        config.advices[8],
                        offset,
                    )?;
                    cross_address_disabled.copy_advice(
                        || "disableCrossAddress padding",
                        &mut region,
                        config.advices[9],
                        offset,
                    )?;

                    config.q_orchard.enable(&mut region, offset)?;
                }

                Ok(())
            },
        )
    }
}

impl plonk::Circuit<pallas::Base> for Circuit {
    type Config = Config;
    type FloorPlanner = floor_planner::V1;

    fn without_witnesses(&self) -> Self {
        Self::empty(self.circuit_version)
    }

    fn configure(meta: &mut plonk::ConstraintSystem<pallas::Base>) -> Self::Config {
        Config::configure(meta)
    }

    fn synthesize(
        &self,
        config: Self::Config,
        mut layouter: impl Layouter<pallas::Base>,
    ) -> Result<(), plonk::Error> {
        let addrs = self.synthesize_base(&config, &mut layouter)?;

        if self.circuit_version.supports_cross_address_restriction() {
            Self::synthesize_cross_address_checks(&config, &mut layouter, &addrs)?;
        }

        Ok(())
    }
}

/// The verifying key for the Orchard Action circuit.
///
/// Build with [`VerifyingKey::build`] for an explicit circuit version.
#[derive(Debug)]
pub struct VerifyingKey {
    pub(crate) params: halo2_proofs::poly::commitment::Params<vesta::Affine>,
    pub(crate) vk: plonk::VerifyingKey<vesta::Affine>,
    circuit_version: OrchardCircuitVersion,
}

impl VerifyingKey {
    /// Builds the verifying key for the given circuit version.
    ///
    /// See [`OrchardCircuitVersion`] for which version to use.
    pub fn build(circuit_version: OrchardCircuitVersion) -> Self {
        let params = halo2_proofs::poly::commitment::Params::new(K);
        let circuit = Circuit::empty(circuit_version);

        let vk = plonk::keygen_vk(&params, &circuit).unwrap();

        VerifyingKey {
            params,
            vk,
            circuit_version,
        }
    }

    /// The circuit version this verifying key was built for.
    pub fn circuit_version(&self) -> OrchardCircuitVersion {
        self.circuit_version
    }

    /// Returns whether this verifying key supports the cross-address restriction.
    pub fn supports_cross_address_restriction(&self) -> bool {
        self.circuit_version.supports_cross_address_restriction()
    }
}

/// The proving key for the Orchard Action circuit.
///
/// Build with [`ProvingKey::build`] for an explicit circuit version.
/// The resulting proofs verify only under a compatible [`VerifyingKey`].
#[derive(Debug)]
pub struct ProvingKey {
    params: halo2_proofs::poly::commitment::Params<vesta::Affine>,
    pk: plonk::ProvingKey<vesta::Affine>,
    circuit_version: OrchardCircuitVersion,
}

impl ProvingKey {
    /// Builds the proving key for the given circuit version.
    ///
    /// See [`OrchardCircuitVersion`] for which version to use.
    pub fn build(circuit_version: OrchardCircuitVersion) -> Self {
        let params = halo2_proofs::poly::commitment::Params::new(K);
        let circuit = Circuit::empty(circuit_version);

        let vk = plonk::keygen_vk(&params, &circuit).unwrap();
        let pk = plonk::keygen_pk(&params, vk, &circuit).unwrap();

        ProvingKey {
            params,
            pk,
            circuit_version,
        }
    }

    /// The circuit version this proving key produces proofs for.
    pub fn circuit_version(&self) -> OrchardCircuitVersion {
        self.circuit_version
    }

    /// Returns whether this proving key supports the cross-address restriction.
    pub fn supports_cross_address_restriction(&self) -> bool {
        self.circuit_version.supports_cross_address_restriction()
    }
}

/// Public inputs to the Orchard Action circuit.
///
/// # Invariants
///
/// Every `Instance` has a non-identity `rk`.
#[derive(Clone, Debug)]
pub struct Instance {
    anchor: Anchor,
    cv_net: ValueCommitment,
    nf_old: Nullifier,
    rk: VerificationKey<SpendAuth>,
    cmx: ExtractedNoteCommitment,
    enable_spend: bool,
    enable_output: bool,
    cross_address_disabled: bool,
}

impl Instance {
    /// Constructs an [`Instance`] from its constituent parts.
    ///
    /// This API can be used in combination with [`Proof::verify`] to build verification
    /// pipelines for many proofs, where you don't want to pass around the full bundle.
    /// Use [`Bundle::verify_proof`] instead if you have the full bundle.
    ///
    /// The provided [`Flags`] are encoded into the spend/output enable public inputs and
    /// the `disableCrossAddress` public input, which is set to the negation of
    /// [`Flags::cross_address_enabled`]. If cross-address transfers are disabled,
    /// callers must use a proving or verifying key whose circuit version supports the
    /// cross-address restriction; [`Proof::create`], [`Proof::verify`], and
    /// [`crate::bundle::BatchValidator`] enforce this.
    ///
    /// Returns `None` if `rk` is the identity [`pasta_curves::pallas::Point`].
    /// zcashd v6.12.1 and Zebra 4.3.1 both added a consensus rule rejecting
    /// transactions whose Orchard actions have an identity `rk`; the Zcash
    /// protocol specification will be updated to match, and this crate
    /// aligns with that rule.
    ///
    /// See:
    /// - <https://zodl.com/zcashd-zebra-april-2026-disclosure/>
    /// - <https://zfnd.org/zebra-4-3-1-critical-security-fixes-dockerized-mining-and-ci-hardening/>
    ///
    /// [`Bundle::verify_proof`]: crate::Bundle::verify_proof
    pub fn from_parts(
        anchor: Anchor,
        cv_net: ValueCommitment,
        nf_old: Nullifier,
        rk: VerificationKey<SpendAuth>,
        cmx: ExtractedNoteCommitment,
        flags: Flags,
    ) -> Option<Self> {
        (!rk.is_identity()).then_some(Instance {
            anchor,
            cv_net,
            nf_old,
            rk,
            cmx,
            enable_spend: flags.spends_enabled(),
            enable_output: flags.outputs_enabled(),
            cross_address_disabled: !flags.cross_address_enabled(),
        })
    }

    /// Returns the Merkle tree anchor of this instance.
    pub(crate) fn anchor(&self) -> &Anchor {
        &self.anchor
    }

    /// Returns the commitment to the net value of this instance.
    pub(crate) fn cv_net(&self) -> &ValueCommitment {
        &self.cv_net
    }

    /// Returns the nullifier of the note being spent by this instance.
    pub(crate) fn nf_old(&self) -> &Nullifier {
        &self.nf_old
    }

    /// Returns the randomized verification key of this instance.
    pub(crate) fn rk(&self) -> &VerificationKey<SpendAuth> {
        &self.rk
    }

    /// Returns the commitment to the new note being created by this instance.
    pub(crate) fn cmx(&self) -> &ExtractedNoteCommitment {
        &self.cmx
    }

    /// Returns whether the spend is enabled for this instance.
    pub(crate) fn enable_spend(&self) -> bool {
        self.enable_spend
    }

    /// Returns whether the output is enabled for this instance.
    pub(crate) fn enable_output(&self) -> bool {
        self.enable_output
    }

    /// Returns whether cross-address transfers are disabled for this instance.
    pub(crate) fn cross_address_disabled(&self) -> bool {
        self.cross_address_disabled
    }

    fn to_halo2_instance(&self) -> [[vesta::Scalar; 10]; 1] {
        let mut instance = [vesta::Scalar::zero(); 10];

        instance[ANCHOR] = self.anchor.inner();
        instance[CV_NET_X] = self.cv_net.x();
        instance[CV_NET_Y] = self.cv_net.y();
        instance[NF_OLD] = self.nf_old.inner();

        let rk = pallas::Point::from_bytes(&self.rk.clone().into())
            .expect("the cached byte encoding of a VerificationKey<_> is canonical")
            .to_affine()
            .coordinates()
            .expect("rk is non-identity by construction");

        instance[RK_X] = *rk.x();
        instance[RK_Y] = *rk.y();
        instance[CMX] = self.cmx.inner();
        instance[ENABLE_SPEND] = vesta::Scalar::from(u64::from(self.enable_spend));
        instance[ENABLE_OUTPUT] = vesta::Scalar::from(u64::from(self.enable_output));
        // Instance columns are zero-padded over the evaluation domain, so for statements
        // where this flag is false, this encoding is commitment-identical to the historical
        // nine-row encoding. Pre-NU 6.3 circuits leave this row unconstrained, which is why
        // restricted statements must never reach those keys (see `Proof::create` and
        // `Proof::verify`).
        instance[DISABLE_CROSS_ADDRESS] =
            vesta::Scalar::from(u64::from(self.cross_address_disabled));

        [instance]
    }
}

impl Proof {
    /// Creates a proof for the given circuits and instances.
    ///
    /// The resulting proof verifies only under a compatible [`VerifyingKey`] (see
    /// [`OrchardCircuitVersion`]).
    ///
    /// Returns [`plonk::Error::Synthesis`] if any circuit's version does not match `pk`'s
    /// version, since `pk` could not produce a valid proof for it.
    ///
    /// Returns [`plonk::Error::InvalidInstances`] if any instance has
    /// `disableCrossAddress = 1` and `pk` is not an
    /// [`OrchardCircuitVersion::PostNu6_3`] proving key.
    ///
    /// All instances of a bundle carry the same `disableCrossAddress` value; that uniformity
    /// is the bundle layer's invariant, and is not checked here.
    pub fn create(
        pk: &ProvingKey,
        circuits: &[Circuit],
        instances: &[Instance],
        mut rng: impl RngCore,
    ) -> Result<Self, plonk::Error> {
        if circuits
            .iter()
            .any(|c| c.circuit_version != pk.circuit_version)
        {
            return Err(plonk::Error::Synthesis);
        }
        if instances.iter().any(Instance::cross_address_disabled)
            && !pk.supports_cross_address_restriction()
        {
            return Err(plonk::Error::InvalidInstances);
        }

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
    ///
    /// # Errors
    ///
    /// Returns [`plonk::Error::InvalidInstances`] if any instance has
    /// `disableCrossAddress = 1` and `vk` is not an
    /// [`OrchardCircuitVersion::PostNu6_3`] verifying key.
    ///
    /// Also returns an error if proof verification fails.
    pub fn verify(&self, vk: &VerifyingKey, instances: &[Instance]) -> Result<(), plonk::Error> {
        if instances.iter().any(Instance::cross_address_disabled)
            && !vk.supports_cross_address_restriction()
        {
            return Err(plonk::Error::InvalidInstances);
        }

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
    /// Internal to [`BatchValidator`], which is the only public batch path. A raw batch
    /// does not know which [`VerifyingKey`] it will be finalized with, so it cannot enforce
    /// that instances disabling cross-address transfers are only finalized with a key whose
    /// circuit version constrains the `disableCrossAddress` public input (see
    /// [`OrchardCircuitVersion::supports_cross_address_restriction`]). [`BatchValidator`]
    /// binds its key at construction and rejects such bundles in [`add_bundle`] before they
    /// reach this method; exposing this directly would let a caller sidestep that check by
    /// finalizing the batch against an unsupported key.
    ///
    /// [`BatchValidator`]: crate::bundle::BatchValidator
    /// [`add_bundle`]: crate::bundle::BatchValidator::add_bundle
    pub(crate) fn add_to_batch(
        &self,
        batch: &mut BatchVerifier<vesta::Affine>,
        instances: Vec<Instance>,
    ) {
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
}

#[cfg(test)]
mod tests {
    use alloc::vec::Vec;
    use core::iter;

    use ff::Field;
    use halo2_proofs::{circuit::Value, dev::MockProver};
    use pasta_curves::{pallas, vesta};
    use rand::{rngs::OsRng, RngCore};

    use super::{Circuit, Instance, OrchardCircuitVersion, Proof, ProvingKey, VerifyingKey, K};
    use crate::{
        bundle::{BundleVersion, Flags},
        keys::SpendValidatingKey,
        note::{Note, NoteVersion, Rho},
        tree::MerklePath,
        value::{ValueCommitTrapdoor, ValueCommitment},
    };

    /// Generates a circuit and instance whose output note is addressed to an expanded
    /// receiver distinct from the spent note's.
    fn generate_circuit_instance<R: RngCore>(
        rng: R,
        circuit_version: OrchardCircuitVersion,
    ) -> (Circuit, Instance) {
        generate_circuit_instance_inner(rng, circuit_version, false)
    }

    /// Generates a circuit and instance whose output note is addressed to the spent
    /// note's expanded receiver, as the cross-address restriction requires.
    fn generate_self_transfer_circuit_instance<R: RngCore>(
        rng: R,
        circuit_version: OrchardCircuitVersion,
    ) -> (Circuit, Instance) {
        generate_circuit_instance_inner(rng, circuit_version, true)
    }

    fn generate_circuit_instance_inner<R: RngCore>(
        mut rng: R,
        circuit_version: OrchardCircuitVersion,
        output_matches_spend: bool,
    ) -> (Circuit, Instance) {
        // Note Version does not matter for this
        let note_version = NoteVersion::V2;
        let (_, fvk, spent_note) = Note::dummy(&mut rng, None, note_version);

        let sender_address = spent_note.recipient();
        let nk = *fvk.nk();
        let rivk = fvk.rivk(fvk.scope_for_address(&spent_note.recipient()).unwrap());
        let nf_old = spent_note.nullifier(&fvk);
        let rho = Rho::from_nf_old(nf_old);
        let ak: SpendValidatingKey = fvk.into();
        let alpha = pallas::Scalar::random(&mut rng);
        let rk = ak.randomize(&alpha);

        let output_note = if output_matches_spend {
            Note::new(
                sender_address,
                spent_note.value(),
                rho,
                note_version,
                &mut rng,
            )
        } else {
            loop {
                let (_, _, output_note) = Note::dummy(&mut rng, Some(rho), note_version);
                if !sender_address.same_expanded_receiver(&output_note.recipient()) {
                    break output_note;
                }
            }
        };
        let cmx = output_note.commitment().into();

        let value = spent_note.value() - output_note.value();
        let rcv = ValueCommitTrapdoor::random(&mut rng);
        let cv_net = ValueCommitment::derive(value, rcv.clone());

        let path = MerklePath::dummy(&mut rng);
        let anchor = path.root(spent_note.commitment().into());

        (
            Circuit {
                circuit_version,
                path: Value::known(path.auth_path()),
                pos: Value::known(path.position()),
                g_d_old: Value::known(sender_address.g_d()),
                pk_d_old: Value::known(*sender_address.pk_d()),
                v_old: Value::known(spent_note.value()),
                rho_old: Value::known(spent_note.rho()),
                psi_old: Value::known(spent_note.psi()),
                rcm_old: Value::known(spent_note.rcm()),
                cm_old: Value::known(spent_note.commitment()),
                alpha: Value::known(alpha),
                ak: Value::known(ak),
                nk: Value::known(nk),
                rivk: Value::known(rivk),
                g_d_new: Value::known(output_note.recipient().g_d()),
                pk_d_new: Value::known(*output_note.recipient().pk_d()),
                v_new: Value::known(output_note.value()),
                psi_new: Value::known(output_note.psi()),
                rcm_new: Value::known(output_note.rcm()),
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
                cross_address_disabled: false,
            },
        )
    }

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    enum ProofFixtureEncoding {
        LegacyTwoFlags,
        PostNu6_3ThreeFlags,
    }

    fn write_test_case<W: std::io::Write>(
        mut w: W,
        instance: &Instance,
        proof: &Proof,
        encoding: ProofFixtureEncoding,
    ) -> std::io::Result<()> {
        w.write_all(&instance.anchor().to_bytes())?;
        w.write_all(&instance.cv_net().to_bytes())?;
        w.write_all(&instance.nf_old().to_bytes())?;
        w.write_all(&<[u8; 32]>::from(instance.rk()))?;
        w.write_all(&instance.cmx().to_bytes())?;
        match encoding {
            ProofFixtureEncoding::LegacyTwoFlags => {
                w.write_all(&[
                    u8::from(instance.enable_spend()),
                    u8::from(instance.enable_output()),
                ])?;
            }
            ProofFixtureEncoding::PostNu6_3ThreeFlags => {
                w.write_all(&[
                    u8::from(instance.enable_spend()),
                    u8::from(instance.enable_output()),
                    u8::from(instance.cross_address_disabled()),
                ])?;
            }
        }
        w.write_all(proof.as_ref())?;
        Ok(())
    }

    fn read_test_case<R: std::io::Read>(
        mut r: R,
        encoding: ProofFixtureEncoding,
    ) -> std::io::Result<(Instance, Proof)> {
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
        let cmx = crate::note::ExtractedNoteCommitment::from_bytes(&read_32_bytes(&mut r)).unwrap();
        let enable_spend = read_bool(&mut r);
        let enable_output = read_bool(&mut r);
        let (cross_address_bit, bundle_version) = match encoding {
            ProofFixtureEncoding::LegacyTwoFlags => (0, BundleVersion::orchard_v2()),
            ProofFixtureEncoding::PostNu6_3ThreeFlags => {
                // The fixture stores the instance-level *disable* bit; the NU6.3 flag
                // byte carries the *enable* bit, so invert when reconstructing.
                //
                // The circuit is pool-agnostic; decode under Ironwood, the pool whose flag
                // byte can represent `enableCrossAddress` either way (Orchard post-NU6.3
                // rejects bit 2).
                let cross_address_disabled = read_bool(&mut r);
                (
                    u8::from(!cross_address_disabled) << 2,
                    BundleVersion::ironwood_v3(),
                )
            }
        };
        let flags = Flags::from_byte(
            u8::from(enable_spend) | (u8::from(enable_output) << 1) | cross_address_bit,
            bundle_version,
        )
        .expect("test vectors use canonical flag encodings");

        let instance = Instance::from_parts(anchor, cv_net, nf_old, rk, cmx, flags)
            .expect("test vectors were generated with non-identity rk");

        let mut proof_bytes = vec![];
        r.read_to_end(&mut proof_bytes)?;
        let proof = Proof::new(proof_bytes);

        Ok((instance, proof))
    }

    #[test]
    fn halo2_instance_includes_cross_address_disabled_flag() {
        let (_, mut instance) =
            generate_circuit_instance(OsRng, OrchardCircuitVersion::FixedPostNu6_2);

        let halo2_instance = instance.to_halo2_instance();
        assert_eq!(halo2_instance[0].len(), 10);
        assert_eq!(
            halo2_instance[0][super::DISABLE_CROSS_ADDRESS],
            vesta::Scalar::zero()
        );

        instance.cross_address_disabled = true;
        assert_eq!(
            instance.to_halo2_instance()[0][super::DISABLE_CROSS_ADDRESS],
            vesta::Scalar::one()
        );
    }

    #[test]
    fn cross_address_support_matches_circuit_version() {
        assert!(!OrchardCircuitVersion::InsecurePreNu6_2.supports_cross_address_restriction());
        assert!(!OrchardCircuitVersion::FixedPostNu6_2.supports_cross_address_restriction());
        assert!(OrchardCircuitVersion::PostNu6_3.supports_cross_address_restriction());
    }

    #[test]
    fn post_nu6_3_cross_address_restriction_is_conditional() {
        let mock_verify = |circuit: &Circuit, instance: &Instance| {
            MockProver::run(
                K,
                circuit,
                instance
                    .to_halo2_instance()
                    .iter()
                    .map(|p| p.to_vec())
                    .collect(),
            )
            .unwrap()
            .verify()
        };

        // An unrestricted cross-address statement is satisfiable...
        let (circuit, mut instance) =
            generate_circuit_instance(OsRng, OrchardCircuitVersion::PostNu6_3);
        assert_eq!(mock_verify(&circuit, &instance), Ok(()));

        // ...but setting `disableCrossAddress` makes it unsatisfiable...
        instance.cross_address_disabled = true;
        assert!(mock_verify(&circuit, &instance).is_err());

        // ...while a restricted self-transfer statement is satisfiable.
        let (circuit, mut instance) =
            generate_self_transfer_circuit_instance(OsRng, OrchardCircuitVersion::PostNu6_3);
        instance.cross_address_disabled = true;
        assert_eq!(mock_verify(&circuit, &instance), Ok(()));
    }

    #[test]
    fn post_nu6_3_restricted_statement_proves_and_verifies() {
        let mut rng = OsRng;
        let (circuit, mut instance) =
            generate_self_transfer_circuit_instance(&mut rng, OrchardCircuitVersion::PostNu6_3);
        instance.cross_address_disabled = true;

        let pk = ProvingKey::build(OrchardCircuitVersion::PostNu6_3);
        let vk = VerifyingKey::build(OrchardCircuitVersion::PostNu6_3);

        let proof = Proof::create(
            &pk,
            core::slice::from_ref(&circuit),
            core::slice::from_ref(&instance),
            &mut rng,
        )
        .unwrap();
        assert!(proof.verify(&vk, core::slice::from_ref(&instance)).is_ok());
    }

    // FixedPostNu6_2 leaves instance row 9 (`disableCrossAddress`) unconstrained, so a
    // freshly created proof can satisfy a restricted statement at the raw halo2 level
    // without enforcing anything about addresses. This test documents that hazard and
    // pins the API checks that close it.
    #[test]
    fn restricted_statement_requires_supporting_key() {
        use halo2_proofs::transcript::{Blake2bRead, Blake2bWrite};

        let mut rng = OsRng;
        let (circuit, mut instance) =
            generate_circuit_instance(&mut rng, OrchardCircuitVersion::FixedPostNu6_2);
        instance.cross_address_disabled = true;

        let pk = ProvingKey::build(OrchardCircuitVersion::FixedPostNu6_2);
        let vk = VerifyingKey::build(OrchardCircuitVersion::FixedPostNu6_2);

        let raw_instances = instance.to_halo2_instance();
        let raw_instances: Vec<_> = raw_instances.iter().map(|i| &i[..]).collect();
        let raw_instances = [&raw_instances[..]];

        let mut transcript = Blake2bWrite::<_, vesta::Affine, _>::init(vec![]);
        super::plonk::create_proof(
            &pk.params,
            &pk.pk,
            core::slice::from_ref(&circuit),
            &raw_instances,
            &mut rng,
            &mut transcript,
        )
        .unwrap();
        let proof_bytes = transcript.finalize();

        let strategy = super::SingleVerifier::new(&vk.params);
        let mut transcript = Blake2bRead::init(&proof_bytes[..]);
        assert!(super::plonk::verify_proof(
            &vk.params,
            &vk.vk,
            strategy,
            &raw_instances,
            &mut transcript,
        )
        .is_ok());

        assert!(matches!(
            Proof::create(
                &pk,
                core::slice::from_ref(&circuit),
                core::slice::from_ref(&instance),
                &mut rng,
            ),
            Err(super::plonk::Error::InvalidInstances),
        ));

        let proof = Proof::new(proof_bytes);
        assert!(matches!(
            proof.verify(&vk, core::slice::from_ref(&instance)),
            Err(super::plonk::Error::InvalidInstances),
        ));
    }

    // Set ORCHARD_CIRCUIT_TEST_GENERATE_NEW_PROOF to regenerate the pinned circuit description
    // for this version.
    fn pinned_circuit_description(
        circuit_version: OrchardCircuitVersion,
        path: &str,
        expected: &str,
    ) -> VerifyingKey {
        let vk = VerifyingKey::build(circuit_version);

        if std::env::var_os("ORCHARD_CIRCUIT_TEST_GENERATE_NEW_PROOF").is_some() {
            std::fs::write(path, format!("{:#?}\n", vk.vk.pinned()))
                .expect("should be able to write new circuit description");
        } else {
            assert_eq!(
                format!("{:#?}\n", vk.vk.pinned()),
                expected.replace("\r\n", "\n")
            );
        }

        vk
    }

    // TODO: recast as a proptest
    fn round_trip_for_version(circuit_version: OrchardCircuitVersion, vk: &VerifyingKey) {
        let mut rng = OsRng;

        let (circuits, instances): (Vec<_>, Vec<_>) = iter::once(())
            .map(|()| generate_circuit_instance(&mut rng, circuit_version))
            .unzip();

        // Test that the proof size is as expected.
        let expected_proof_size = {
            let circuit_cost =
                halo2_proofs::dev::CircuitCost::<pasta_curves::vesta::Point, _>::measure(
                    K,
                    &circuits[0],
                );
            // These sizes are identical for every circuit version: the post-NU 6.3 circuit reuses the
            // existing Orchard checks gate on spare rows and adds no columns or
            // commitments, leaving the proof shape unchanged.
            assert_eq!(usize::from(circuit_cost.proof_size(1)), 4992);
            assert_eq!(usize::from(circuit_cost.proof_size(2)), 7264);
            // The constants in `Proof::expected_proof_size` must track the circuit's actual
            // proof size; this guards them against drift if the circuit ever changes.
            assert_eq!(Proof::expected_proof_size(1), 4992);
            assert_eq!(Proof::expected_proof_size(2), 7264);
            assert_eq!(
                Proof::expected_proof_size(instances.len()),
                usize::from(circuit_cost.proof_size(instances.len())),
            );
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

        let pk = ProvingKey::build(circuit_version);
        let proof = Proof::create(&pk, &circuits, &instances, &mut rng).unwrap();
        assert!(proof.verify(vk, &instances).is_ok());
        assert_eq!(proof.0.len(), expected_proof_size);
    }

    #[test]
    fn round_trip_fixed() {
        let vk = pinned_circuit_description(
            OrchardCircuitVersion::FixedPostNu6_2,
            "src/circuit_data/circuit_description_fixed",
            include_str!("circuit_data/circuit_description_fixed"),
        );
        round_trip_for_version(OrchardCircuitVersion::FixedPostNu6_2, &vk);
    }

    #[test]
    fn round_trip_post_nu6_3() {
        let vk = pinned_circuit_description(
            OrchardCircuitVersion::PostNu6_3,
            "src/circuit_data/circuit_description_post_nu6_3",
            include_str!("circuit_data/circuit_description_post_nu6_3"),
        );
        round_trip_for_version(OrchardCircuitVersion::PostNu6_3, &vk);
    }

    // Proves with the proving key for `proving_version` and checks that the proof verifies under
    // the verifying key for the same version, but not under a version with a different verifying
    // key.
    fn proof_is_rejected_by_other_circuit_version(
        proving_version: OrchardCircuitVersion,
        other_version: OrchardCircuitVersion,
    ) {
        let mut rng = OsRng;

        let (circuit, instance) = generate_circuit_instance(&mut rng, proving_version);
        let instances = core::slice::from_ref(&instance);

        let pk = ProvingKey::build(proving_version);
        let proof = Proof::create(&pk, &[circuit], instances, &mut rng).unwrap();

        // Verifies under the matching version's verifying key.
        let vk_matching = VerifyingKey::build(proving_version);
        assert!(proof.verify(&vk_matching, instances).is_ok());

        // Does not verify under the other version's verifying key.
        let vk_other = VerifyingKey::build(other_version);
        assert!(proof.verify(&vk_other, instances).is_err());
    }

    #[test]
    fn proof_verifies_against_matching_circuit_version() {
        // Insecure proofs are rejected by the anchored circuit versions, and anchored proofs are
        // rejected by the insecure verifying key.
        proof_is_rejected_by_other_circuit_version(
            OrchardCircuitVersion::FixedPostNu6_2,
            OrchardCircuitVersion::InsecurePreNu6_2,
        );
        proof_is_rejected_by_other_circuit_version(
            OrchardCircuitVersion::PostNu6_3,
            OrchardCircuitVersion::InsecurePreNu6_2,
        );
        proof_is_rejected_by_other_circuit_version(
            OrchardCircuitVersion::InsecurePreNu6_2,
            OrchardCircuitVersion::FixedPostNu6_2,
        );
        proof_is_rejected_by_other_circuit_version(
            OrchardCircuitVersion::InsecurePreNu6_2,
            OrchardCircuitVersion::PostNu6_3,
        );
    }

    #[test]
    fn fixed_and_post_nu6_3_have_distinct_verifying_keys() {
        proof_is_rejected_by_other_circuit_version(
            OrchardCircuitVersion::FixedPostNu6_2,
            OrchardCircuitVersion::PostNu6_3,
        );
        proof_is_rejected_by_other_circuit_version(
            OrchardCircuitVersion::PostNu6_3,
            OrchardCircuitVersion::FixedPostNu6_2,
        );
    }

    // Proving a circuit with a proving key for a different circuit version is a misuse: the
    // proving key and circuits must agree (see `Proof::create`). Confirm `create` rejects it
    // with `plonk::Error::Synthesis` rather than emitting an unverifiable proof.
    #[test]
    fn create_rejects_mismatched_proving_key_version() {
        let mut rng = OsRng;

        for (circuit_version, pk_version) in [
            (
                OrchardCircuitVersion::InsecurePreNu6_2,
                OrchardCircuitVersion::FixedPostNu6_2,
            ),
            (
                OrchardCircuitVersion::FixedPostNu6_2,
                OrchardCircuitVersion::PostNu6_3,
            ),
            (
                OrchardCircuitVersion::PostNu6_3,
                OrchardCircuitVersion::FixedPostNu6_2,
            ),
        ] {
            let (circuit, instance) = generate_circuit_instance(&mut rng, circuit_version);
            let instances = core::slice::from_ref(&instance);

            let mismatched_pk = ProvingKey::build(pk_version);

            assert!(matches!(
                Proof::create(&mismatched_pk, &[circuit], instances, &mut rng),
                Err(super::plonk::Error::Synthesis),
            ));
        }
    }

    fn serialized_proof_test_case_for_version(
        circuit_version: OrchardCircuitVersion,
        proof_path: &str,
        test_case_bytes: &[u8],
        encoding: ProofFixtureEncoding,
        expected_proof_size: usize,
        restricted: bool,
    ) {
        let vk = VerifyingKey::build(circuit_version);
        // Set ORCHARD_CIRCUIT_TEST_GENERATE_NEW_PROOF to regenerate this serialized proof
        // fixture. The non-regeneration path embeds and verifies the checked-in fixture.
        if std::env::var_os("ORCHARD_CIRCUIT_TEST_GENERATE_NEW_PROOF").is_some() {
            let create_proof = || -> std::io::Result<()> {
                let mut rng = OsRng;

                let (circuit, mut instance) = if restricted {
                    generate_self_transfer_circuit_instance(&mut rng, circuit_version)
                } else {
                    generate_circuit_instance(&mut rng, circuit_version)
                };
                instance.cross_address_disabled = restricted;
                let instances = core::slice::from_ref(&instance);

                let pk = ProvingKey::build(circuit_version);
                let proof = Proof::create(&pk, &[circuit], instances, &mut rng).unwrap();
                assert!(proof.verify(&vk, instances).is_ok());

                let file = std::fs::File::create(proof_path)?;
                write_test_case(file, &instance, &proof, encoding)
            };
            create_proof().expect("should be able to write new proof");
            // Regeneration only writes the fixture; the non-generate run below embeds and
            // verifies it.
            return;
        }

        // Parse the hardcoded proof test case.
        let (instance, proof) =
            read_test_case(test_case_bytes, encoding).expect("proof must be valid");
        assert_eq!(instance.cross_address_disabled(), restricted);
        assert_eq!(proof.0.len(), expected_proof_size);

        assert!(proof.verify(&vk, &[instance]).is_ok());
    }

    #[test]
    fn serialized_fixed_proof_test_case() {
        serialized_proof_test_case_for_version(
            OrchardCircuitVersion::FixedPostNu6_2,
            "src/circuit_data/circuit_proof_test_case_fixed.bin",
            include_bytes!("circuit_data/circuit_proof_test_case_fixed.bin"),
            ProofFixtureEncoding::LegacyTwoFlags,
            4992,
            false,
        );
    }

    #[test]
    fn serialized_post_nu6_3_proof_test_case() {
        serialized_proof_test_case_for_version(
            OrchardCircuitVersion::PostNu6_3,
            "src/circuit_data/circuit_proof_test_case_post_nu6_3.bin",
            include_bytes!("circuit_data/circuit_proof_test_case_post_nu6_3.bin"),
            ProofFixtureEncoding::PostNu6_3ThreeFlags,
            4992,
            false,
        );
    }

    #[test]
    fn serialized_post_nu6_3_restricted_proof_test_case() {
        serialized_proof_test_case_for_version(
            OrchardCircuitVersion::PostNu6_3,
            "src/circuit_data/circuit_proof_test_case_post_nu6_3_restricted.bin",
            include_bytes!("circuit_data/circuit_proof_test_case_post_nu6_3_restricted.bin"),
            ProofFixtureEncoding::PostNu6_3ThreeFlags,
            4992,
            true,
        );
    }

    // The deployed (NU5..NU6.2) verifying key and a pre-fix proof. `InsecurePreNu6_2`
    // reconstructs the historical circuit, so this checks that the deployed verifying key is
    // reproduced exactly and that the old proof still verifies under it — the guarantee that
    // lets a node sync from before the fix. These fixtures are frozen as the canonical
    // pre-NU6.2 verifying key and a sample proof, so they are never regenerated.
    #[test]
    fn insecure_against_stored_circuit() {
        let vk = VerifyingKey::build(OrchardCircuitVersion::InsecurePreNu6_2);
        assert_eq!(
            format!("{:#?}\n", vk.vk.pinned()),
            include_str!("circuit_data/circuit_description_insecure").replace("\r\n", "\n")
        );

        let (instance, proof) = {
            let test_case_bytes =
                include_bytes!("circuit_data/circuit_proof_test_case_insecure.bin");
            read_test_case(&test_case_bytes[..], ProofFixtureEncoding::LegacyTwoFlags)
                .expect("proof must be valid")
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

        let circuit = Circuit::empty(OrchardCircuitVersion::FixedPostNu6_2);
        halo2_proofs::dev::CircuitLayout::default()
            .show_labels(false)
            .view_height(0..(1 << 11))
            .render(K, &circuit, &root)
            .unwrap();
    }

    mod from_parts_rk_identity {
        use ff::{Field as _, PrimeField as _};
        use pasta_curves::pallas;

        use super::super::Instance;
        use crate::{
            bundle::Flags,
            note::{ExtractedNoteCommitment, Nullifier},
            primitives::redpallas::{self, SpendAuth},
            tree::Anchor,
            value::{ValueCommitTrapdoor, ValueCommitment, ValueSum},
        };

        /// Non-rk fields for `Instance`. Distinct non-zero patterns avoid
        /// accidental overlap with sentinel values. See the analogous helper
        /// in `src/action.rs` for notes on which of these fields have
        /// consensus-level validity checks elsewhere in the pipeline.
        fn dummy_other_fields() -> (Anchor, ValueCommitment, Nullifier, ExtractedNoteCommitment) {
            let anchor = Anchor::from_bytes([6u8; 32]).unwrap();
            let cv_net =
                ValueCommitment::derive(ValueSum::from_raw(42), ValueCommitTrapdoor::zero());
            let nf_old = Nullifier::from_bytes(&[1u8; 32]).unwrap();
            let cmx = ExtractedNoteCommitment::from_bytes(&[2u8; 32]).unwrap();
            (anchor, cv_net, nf_old, cmx)
        }

        fn identity_rk() -> redpallas::VerificationKey<SpendAuth> {
            redpallas::VerificationKey::<SpendAuth>::try_from([0u8; 32])
                .expect("plain redpallas accepts the identity encoding")
        }

        fn non_identity_rk() -> redpallas::VerificationKey<SpendAuth> {
            let ask_bytes: [u8; 32] = pallas::Scalar::ONE.to_repr();
            let ask = redpallas::SigningKey::<SpendAuth>::try_from(ask_bytes)
                .expect("1 is a valid scalar");
            (&ask).into()
        }

        #[test]
        fn rejects_identity_rk() {
            let (anchor, cv_net, nf_old, cmx) = dummy_other_fields();
            let result =
                Instance::from_parts(anchor, cv_net, nf_old, identity_rk(), cmx, Flags::ENABLED);
            assert!(result.is_none());
        }

        #[test]
        fn accepts_non_identity_rk() {
            let (anchor, cv_net, nf_old, cmx) = dummy_other_fields();
            let rk = non_identity_rk();
            let instance =
                Instance::from_parts(anchor, cv_net, nf_old, rk.clone(), cmx, Flags::ENABLED)
                    .expect("non-identity rk must be accepted");
            assert_eq!(instance.rk(), &rk);
        }
    }
}
