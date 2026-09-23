An implementation of Schnorr signatures on the Jubjub curve for both single and threshold numbers
of signers (FROST).

## Example: key generation with trusted dealer and FROST signing

Creating a key with a trusted dealer and splitting into shares; then signing a message
and aggregating the signature. Note that the example just simulates a distributed
scenario in a single thread and it abstracts away any communication between peers.


```rust
use reddsa::frost::redjubjub as frost;
use rand::thread_rng;
use std::collections::HashMap;

let mut rng = thread_rng();
let max_signers = 5;
let min_signers = 3;
let (shares, pubkeys) = frost::keys::keygen_with_dealer(max_signers, min_signers, &mut rng)?;

// Verifies the secret shares from the dealer and stores them in a HashMap.
// In practice, each KeyPackage must be sent to its respective participant
// through a confidential and authenticated channel.
let key_packages: HashMap<_, _> = shares
    .into_iter()
    .map(|share| Ok((share.identifier, frost::keys::KeyPackage::try_from(share)?)))
    .collect::<Result<_, _>>()?;

let mut nonces = HashMap::new();
let mut commitments = HashMap::new();

////////////////////////////////////////////////////////////////////////////
// Round 1: generating nonces and signing commitments for each participant
////////////////////////////////////////////////////////////////////////////

// In practice, each iteration of this loop will be executed by its respective participant.
for participant_index in 1..(min_signers as u16 + 1) {
    let participant_identifier = participant_index.try_into().expect("should be nonzero");
    // Generate one (1) nonce and one SigningCommitments instance for each
    // participant, up to _threshold_.
    let (nonce, commitment) = frost::round1::commit(
        participant_identifier,
        key_packages[&participant_identifier].secret_share(),
        &mut rng,
    );
    // In practice, the nonces and commitments must be sent to the coordinator
    // (or to every other participant if there is no coordinator) using
    // an authenticated channel.
    nonces.insert(participant_identifier, nonce);
    commitments.insert(participant_identifier, commitment);
}

// This is what the signature aggregator / coordinator needs to do:
// - decide what message to sign
// - take one (unused) commitment per signing participant
let mut signature_shares = Vec::new();
let message = "message to sign".as_bytes();
let comms = commitments.clone().into_values().collect();
// In practice, the SigningPackage must be sent to all participants
// involved in the current signing (at least min_signers participants),
// using an authenticated channel (and confidential if the message is secret).
let signing_package = frost::SigningPackage::new(comms, message.to_vec());

////////////////////////////////////////////////////////////////////////////
// Round 2: each participant generates their signature share
////////////////////////////////////////////////////////////////////////////

// In practice, each iteration of this loop will be executed by its respective participant.
for participant_identifier in nonces.keys() {
    let key_package = &key_packages[participant_identifier];

    let nonces_to_use = &nonces[participant_identifier];

    // Each participant generates their signature share.
    let signature_share = frost::round2::sign(&signing_package, nonces_to_use, key_package)?;

    // In practice, the signature share must be sent to the Coordinator
    // using an authenticated channel.
    signature_shares.push(signature_share);
}

////////////////////////////////////////////////////////////////////////////
// Aggregation: collects the signing shares from all participants,
// generates the final signature.
////////////////////////////////////////////////////////////////////////////

// Aggregate (also verifies the signature shares)
let group_signature = frost::aggregate(&signing_package, &signature_shares[..], &pubkeys)?;

// Check that the threshold signature can be verified by the group public
// key (the verification key).
assert!(pubkeys
    .group_public
    .verify(message, &group_signature)
    .is_ok());

# Ok::<(), frost::Error>(())
```
