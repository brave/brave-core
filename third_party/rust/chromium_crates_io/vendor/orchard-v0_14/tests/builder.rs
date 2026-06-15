#![cfg(feature = "circuit")]

use incrementalmerkletree::{Hashable, Marking, Retention};
use orchard::{
    builder::{Builder, BundleType},
    bundle::{Authorized, Flags},
    circuit::{OrchardCircuitVersion, ProvingKey, VerifyingKey},
    keys::{FullViewingKey, PreparedIncomingViewingKey, Scope, SpendAuthorizingKey, SpendingKey},
    note::ExtractedNoteCommitment,
    note_encryption::OrchardDomain,
    tree::MerkleHashOrchard,
    value::NoteValue,
    Bundle,
};
use rand::rngs::OsRng;
use shardtree::{store::memory::MemoryShardStore, ShardTree};
use zcash_note_encryption::try_note_decryption;

fn verify_bundle(bundle: &Bundle<Authorized, i64>, vk: &VerifyingKey) {
    assert!(matches!(bundle.verify_proof(vk), Ok(())));
    let sighash: [u8; 32] = bundle.commitment().into();
    let bvk = bundle.binding_validating_key();
    for action in bundle.actions() {
        assert_eq!(action.rk().verify(&sighash, action.authorization()), Ok(()));
    }
    assert_eq!(
        bvk.verify(&sighash, bundle.authorization().binding_signature()),
        Ok(())
    );
}

#[test]
fn bundle_chain() {
    let mut rng = OsRng;
    let pk = ProvingKey::build();
    let vk = VerifyingKey::build();

    let sk = SpendingKey::from_bytes([0; 32]).unwrap();
    let fvk = FullViewingKey::from(&sk);
    let recipient = fvk.address_at(0u32, Scope::External);

    // Create a shielding bundle.
    let shielding_bundle: Bundle<_, i64> = {
        // Use the empty tree.
        let anchor = MerkleHashOrchard::empty_root(32.into()).into();

        let mut builder = Builder::new(
            BundleType::Transactional {
                flags: Flags::SPENDS_DISABLED,
                bundle_required: false,
            },
            anchor,
        );
        let note_value = NoteValue::from_raw(5000);
        assert_eq!(
            builder.add_output(None, recipient, note_value, [0u8; 512]),
            Ok(())
        );
        let (unauthorized, bundle_meta) = builder.build(&mut rng).unwrap().unwrap();

        assert_eq!(
            unauthorized
                .decrypt_output_with_key(
                    bundle_meta
                        .output_action_index(0)
                        .expect("Output 0 can be found"),
                    &fvk.to_ivk(Scope::External)
                )
                .map(|(note, _, _)| note.value()),
            Some(note_value)
        );

        let sighash = unauthorized.commitment().into();
        let proven = unauthorized.create_proof(&pk, &mut rng).unwrap();
        proven.apply_signatures(rng, sighash, &[]).unwrap()
    };

    // Verify the shielding bundle.
    verify_bundle(&shielding_bundle, &vk);

    // Create a shielded bundle spending the previous output.
    let shielded_bundle: Bundle<_, i64> = {
        let ivk = PreparedIncomingViewingKey::new(&fvk.to_ivk(Scope::External));
        let (note, _, _) = shielding_bundle
            .actions()
            .iter()
            .find_map(|action| {
                let domain = OrchardDomain::for_action(action);
                try_note_decryption(&domain, &ivk, action)
            })
            .unwrap();

        // Use the tree with a single leaf.
        let cmx: ExtractedNoteCommitment = note.commitment().into();
        let leaf = MerkleHashOrchard::from_cmx(&cmx);
        let mut tree: ShardTree<MemoryShardStore<MerkleHashOrchard, u32>, 32, 16> =
            ShardTree::new(MemoryShardStore::empty(), 100);
        tree.append(
            leaf,
            Retention::Checkpoint {
                id: 0,
                marking: Marking::Marked,
            },
        )
        .unwrap();
        let root = tree.root_at_checkpoint_id(&0).unwrap().unwrap();
        let position = tree.max_leaf_position(None).unwrap().unwrap();
        let merkle_path = tree
            .witness_at_checkpoint_id(position, &0)
            .unwrap()
            .unwrap();
        assert_eq!(root, merkle_path.root(MerkleHashOrchard::from_cmx(&cmx)));

        let mut builder = Builder::new(BundleType::DEFAULT, root.into());
        assert_eq!(builder.add_spend(fvk, note, merkle_path.into()), Ok(()));
        assert_eq!(
            builder.add_output(None, recipient, NoteValue::from_raw(5000), [0u8; 512]),
            Ok(())
        );
        let (unauthorized, _) = builder.build(&mut rng).unwrap().unwrap();
        let sighash = unauthorized.commitment().into();
        let proven = unauthorized.create_proof(&pk, &mut rng).unwrap();
        proven
            .apply_signatures(rng, sighash, &[SpendAuthorizingKey::from(&sk)])
            .unwrap()
    };

    // Verify the shielded bundle.
    verify_bundle(&shielded_bundle, &vk);
}

// A bundle built with the circuit version set to `InsecurePreNu6_2` produces a proof against
// the historical (insecure) circuit, which verifies under the insecure verifying key but not
// the fixed one. This is the path that lets tests reproduce pre-NU6.2 proofs.
#[test]
fn builder_builds_for_insecure_circuit_version() {
    let mut rng = OsRng;
    let insecure_pk = ProvingKey::build_for_version(OrchardCircuitVersion::InsecurePreNu6_2);
    let insecure_vk = VerifyingKey::build_for_version(OrchardCircuitVersion::InsecurePreNu6_2);
    let fixed_vk = VerifyingKey::build();

    let sk = SpendingKey::from_bytes([0; 32]).unwrap();
    let fvk = FullViewingKey::from(&sk);
    let recipient = fvk.address_at(0u32, Scope::External);

    let anchor = MerkleHashOrchard::empty_root(32.into()).into();
    let mut builder = Builder::new_for_version(
        BundleType::Transactional {
            flags: Flags::SPENDS_DISABLED,
            bundle_required: false,
        },
        anchor,
        OrchardCircuitVersion::InsecurePreNu6_2,
    );
    assert_eq!(
        builder.add_output(None, recipient, NoteValue::from_raw(5000), [0u8; 512]),
        Ok(())
    );

    let (unauthorized, _) = builder.build::<i64>(&mut rng).unwrap().unwrap();
    let sighash: [u8; 32] = unauthorized.commitment().into();
    let proven = unauthorized.create_proof(&insecure_pk, &mut rng).unwrap();
    let bundle = proven.apply_signatures(rng, sighash, &[]).unwrap();

    assert!(matches!(bundle.verify_proof(&insecure_vk), Ok(())));
    assert!(bundle.verify_proof(&fixed_vk).is_err());
}
