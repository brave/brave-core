# Distributed Key Generation (DKG)

The DKG module supports generating FROST key shares in a distributed manner,
without a trusted dealer.

Before starting, each participant needs a unique identifier, which can be built from
a `u16`. The process in which these identifiers are allocated is up to the application.

The distributed key generation process has 3 parts, with 2 communication rounds
between them, in which each participant needs to send a "package" to every other
participant. In the first round, each participant sends the same package
(a [`round1::Package`]) to every other. In the second round, each receiver gets
their own package (a [`round2::Package`]).

Between part 1 and 2, each participant needs to hold onto a [`round1::SecretPackage`]
that MUST be kept secret. Between part 2 and 3, each participant needs to hold
onto a [`round2::SecretPackage`].

After the third part, each participant will get a [`KeyPackage`] with their
long-term secret share that must be kept secret, and a [`PublicKeyPackage`]
that is public (and will be the same between all participants). With those
they can proceed to sign messages with FROST.


## Example

```rust
use rand::thread_rng;
use std::collections::HashMap;

use reddsa::frost::redpallas as frost;

let mut rng = thread_rng();

////////////////////////////////////////////////////////////////////////////
// Key generation, Round 1
////////////////////////////////////////////////////////////////////////////

let max_signers = 5;
let min_signers = 3;

// Keep track of each participant's round 1 secret package.
// In practice each participant will keep its copy; no one
// will have all the participant's packages.
let mut round1_secret_packages = HashMap::new();

// Keep track of all round 1 packages sent to the given participant.
// This is used to simulate the broadcast; in practice the packages
// will be sent through some communication channel.
let mut received_round1_packages = HashMap::new();

// For each participant, perform the first part of the DKG protocol.
// In practice, each participant will perform this on their own environments.
for participant_index in 1..=max_signers {
    let participant_identifier = participant_index.try_into().expect("should be nonzero");
    let (secret_package, round1_package) = frost::keys::dkg::part1(
        participant_identifier,
        max_signers,
        min_signers,
        &mut rng,
    )?;

    // Store the participant's secret package for later use.
    // In practice each participant will store it in their own environment.
    round1_secret_packages.insert(participant_identifier, secret_package);

    // "Send" the round 1 package to all other participants. In this
    // test this is simulated using a HashMap; in practice this will be
    // sent through some communication channel.
    for receiver_participant_index in 1..=max_signers {
        if receiver_participant_index == participant_index {
            continue;
        }
        let receiver_participant_identifier: frost::Identifier = receiver_participant_index
            .try_into()
            .expect("should be nonzero");
        received_round1_packages
            .entry(receiver_participant_identifier)
            .or_insert_with(Vec::new)
            .push(round1_package.clone());
    }
}

////////////////////////////////////////////////////////////////////////////
// Key generation, Round 2
////////////////////////////////////////////////////////////////////////////

// Keep track of each participant's round 2 secret package.
// In practice each participant will keep its copy; no one
// will have all the participant's packages.
let mut round2_secret_packages = HashMap::new();

// Keep track of all round 2 packages sent to the given participant.
// This is used to simulate the broadcast; in practice the packages
// will be sent through some communication channel.
let mut received_round2_packages = HashMap::new();

// For each participant, perform the second part of the DKG protocol.
// In practice, each participant will perform this on their own environments.
for participant_index in 1..=max_signers {
    let participant_identifier = participant_index.try_into().expect("should be nonzero");
    let (round2_secret_package, round2_packages) = frost::keys::dkg::part2(
        round1_secret_packages
            .remove(&participant_identifier)
            .unwrap(),
        &received_round1_packages[&participant_identifier],
    )?;

    // Store the participant's secret package for later use.
    // In practice each participant will store it in their own environment.
    round2_secret_packages.insert(participant_identifier, round2_secret_package);

    // "Send" the round 2 package to all other participants. In this
    // test this is simulated using a HashMap; in practice this will be
    // sent through some communication channel.
    // Note that, in contrast to the previous part, here each other participant
    // gets its own specific package.
    for round2_package in round2_packages {
        received_round2_packages
            .entry(round2_package.receiver_identifier)
            .or_insert_with(Vec::new)
            .push(round2_package);
    }
}

////////////////////////////////////////////////////////////////////////////
// Key generation, final computation
////////////////////////////////////////////////////////////////////////////

// Keep track of each participant's long-lived key package.
// In practice each participant will keep its copy; no one
// will have all the participant's packages.
let mut key_packages = HashMap::new();

// Keep track of each participant's public key package.
// In practice, if there is a Coordinator, only they need to store the set.
// If there is not, then all candidates must store their own sets.
// All participants will have the same exact public key package.
let mut pubkey_packages = HashMap::new();

// For each participant, perform the third part of the DKG protocol.
// In practice, each participant will perform this on their own environments.
for participant_index in 1..=max_signers {
    let participant_identifier = participant_index.try_into().expect("should be nonzero");
    let (key_package, pubkey_package_for_participant) = frost::keys::dkg::part3(
        &round2_secret_packages[&participant_identifier],
        &received_round1_packages[&participant_identifier],
        &received_round2_packages[&participant_identifier],
    )?;
    key_packages.insert(participant_identifier, key_package);
    pubkey_packages.insert(participant_identifier, pubkey_package_for_participant);
}

// With its own key package and the pubkey package, each participant can now proceed
// to sign with FROST.
# Ok::<(), frost::Error>(())
```
