# FROST messages

Proposes a message layout to exchange information between participants of a FROST setup using the [jubjub](https://github.com/zkcrypto/jubjub) curve.

## Motivation

Currently FROST library is complete for 2 round signatures with a dealer/aggregator setup.
This proposal acknowledges that specific features, additions and upgrades will need to be made when DKG is implemented.

Assuming all participants have a FROST library available, we need to define message structures in a way that data can be exchanged between participants. The proposal is a collection of data types so each side can do all the actions needed for a real life situation.

## Definitions

- `dealer` - Participant who distributes the initial package to all the other participants.
- `aggregator` - Participant in charge of collecting all the signatures from the other participants and generating the final group signature.
- `signer` - Participant that will receive the initial package, sign and send the signature to the aggregator to receive the final group signature.

Note: In this RFC we consider the above 3 participants to be different. `dealer` and `aggregator` have specific hard coded `ParticipantId`s, so for example a `dealer` can't be a `signer`. This is not a protocol limitation but a specific rule introduced in this document.

## Guide-level explanation

We propose a message separated in 2 parts, a header and a payload:

```rust
/// The data required to serialize a frost message.
struct Message {
    header: Header,
    payload: Payload,
}
```

`Header` will look as follows:

```rust
/// The data required to serialize the common header fields for every message.
///
/// Note: the `msg_type` is derived from the `payload` enum variant.
struct Header {
    version: MsgVersion,
    sender: ParticipantId,
    receiver: ParticipantId,
}
```

While `Payload` will be defined as:

```rust
/// The data required to serialize the payload for a message.
enum Payload {
    SharePackage(messages::SharePackage),
    SigningCommitments(messages::SigningCommitments),
    SigningPackage(messages::SigningPackage),
    SignatureShare(messages::SignatureShare),
    AggregateSignature(messages::AggregateSignature),
}
```

All the messages and new types will be defined in a new file `src/frost/messages.rs`

## Reference-level explanation

Here we explore in detail the header types and all the message payloads. 

### Header

Fields of the header define new types. Proposed implementation for them is as follows:

```rust
/// The numeric values used to identify each `Payload` variant during serialization.
#[repr(u8)]
#[non_exhaustive]
enum MsgType {
    SharePackage,
    SigningCommitments,
    SigningPackage,
    SignatureShare,
    AggregateSignature,
}

/// The numeric values used to identify the protocol version during serialization.
struct MsgVersion(u8);

const BASIC_FROST_SERIALIZATION: MsgVersion = MsgVersion(0);

/// The numeric values used to identify each participant during serialization.
///
/// In the `frost` module, participant ID `0` should be invalid.
/// But in serialization, we want participants to be indexed from `0..n`,
/// where `n` is the number of participants.
/// This helps us look up their shares and commitments in serialized arrays.
/// So in serialization, we assign the dealer and aggregator the highest IDs,
/// and mark those IDs as invalid for signers. Then we serialize the
/// participants in numeric order of their FROST IDs.
///
/// "When performing Shamir secret sharing, a polynomial `f(x)` is used to generate
/// each party’s share of the secret. The actual secret is `f(0)` and the party with
/// ID `i` will be given a share with value `f(i)`.
/// Since a DKG may be implemented in the future, we recommend that the ID `0` be declared invalid."
/// https://raw.githubusercontent.com/ZcashFoundation/redjubjub/main/zcash-frost-audit-report-20210323.pdf#d
enum ParticipantId {
    /// A serialized participant ID for a signer.
    ///
    /// Must be less than or equal to `MAX_SIGNER_PARTICIPANT_ID`.
    Signer(u64),
    /// The fixed participant ID for the dealer.
    Dealer,
    /// The fixed participant ID for the aggregator.
    Aggregator,
}

/// The fixed participant ID for the dealer.
const DEALER_PARTICIPANT_ID: u64 = u64::MAX - 1;

/// The fixed participant ID for the aggregator.
const AGGREGATOR_PARTICIPANT_ID: u64 = u64::MAX;

/// The maximum `ParticipantId::Signer` in this serialization format.
///
/// We reserve two participant IDs for the dealer and aggregator.
const MAX_SIGNER_PARTICIPANT_ID: u64 = u64::MAX - 2;
```

### Payloads

Each payload defines a new message:

```rust
/// The data required to serialize `frost::SharePackage`.
///
/// The dealer sends this message to each signer for this round.
/// With this, the signer should be able to build a `SharePackage` and use
/// the `sign()` function.
///
/// Note: `frost::SharePackage.public` can be calculated from `secret_share`.
struct messages::SharePackage {
    /// The public signing key that represents the entire group:
    /// `frost::SharePackage.group_public`.
    group_public: VerificationKey<SpendAuth>,
    /// This participant's secret key share: `frost::SharePackage.share.value`.
    secret_share: frost::Secret,
    /// The commitments to the coefficients for our secret polynomial _f_,
    /// used to generate participants' key shares. Participants use these to perform
    /// verifiable secret sharing.
    /// Share packages that contain duplicate or missing `ParticipantId`s are invalid.
    /// `ParticipantId`s must be serialized in ascending numeric order.
    share_commitment: BTreeMap<ParticipantId, frost::Commitment>,
}

/// The data required to serialize `frost::SigningCommitments`.
///
/// Each signer must send this message to the aggregator.
/// A signing commitment from the first round of the signing protocol.
struct messages::SigningCommitments {
    /// The hiding point: `frost::SigningCommitments.hiding`
    hiding: frost::Commitment,
    /// The binding point: `frost::SigningCommitments.binding`
    binding: frost::Commitment,
}

/// The data required to serialize `frost::SigningPackage`.
///
/// The aggregator decides what message is going to be signed and
/// sends it to each signer with all the commitments collected.
struct messages::SigningPackage {
    /// The collected commitments for each signer as an ordered map of
    /// unique participant identifiers: `frost::SigningPackage.signing_commitments`
    ///
    /// Signing packages that contain duplicate or missing `ParticipantId`s are invalid.
    /// `ParticipantId`s must be serialized in ascending numeric order.
    signing_commitments: BTreeMap<ParticipantId, SigningCommitments>,
    /// The message to be signed: `frost::SigningPackage.message`.
    ///
    /// Each signer should perform protocol-specific verification on the message.
    message: Vec<u8>,
}

/// The data required to serialize `frost::SignatureShare`.
///
/// Each signer sends their signatures to the aggregator who is going to collect them
/// and generate a final spend signature.
struct messages::SignatureShare {
     /// This participant's signature over the message: `frost::SignatureShare.signature`
    signature: frost::SignatureResponse,
}

/// The data required to serialize a successful output from `frost::aggregate()`.
///
/// The final signature is broadcasted by the aggregator to all signers.
struct messages::AggregateSignature {
    /// The aggregated group commitment: `Signature<SpendAuth>.r_bytes` returned by `frost::aggregate`
    group_commitment: frost::GroupCommitment,
    /// A plain Schnorr signature created by summing all the signature shares:
    /// `Signature<SpendAuth>.s_bytes` returned by `frost::aggregate`
    schnorr_signature: frost::SignatureResponse,
}
```

## Validation

Validation is implemented to each new data type as needed. This will ensure the creation of valid messages before they are send and right after they are received. We create a trait for this as follows:

```rust
pub trait Validate {
    fn validate(&self) -> Result<&Self, MsgErr>;
}
```

And we implement where needed. For example, in the header, sender and receiver can't be the same:

```rust
impl Validate for Header {
    fn validate(&self) -> Result<&Self, MsgErr> {
        if self.sender.0 == self.receiver.0 {
            return Err(MsgErr::SameSenderAndReceiver);
        }
        Ok(self)
    }
}
```

This will require to have validation error messages as:

```rust
use thiserror::Error;

#[derive(Clone, Error, Debug)]
pub enum MsgErr {
    #[error("sender and receiver are the same")]
    SameSenderAndReceiver,
}
```

Then to create a valid `Header` in the sender side we call:

```rust
let header = Validate::validate(&Header {
    ..
}).expect("a valid header");
```

The receiver side will validate the header using the same method. Instead of panicking the error can be ignored to don't crash and keep waiting for other (potentially valid) messages.

```rust
if let Ok(header) = msg.header.validate() {
    ..
}
```

### Rules

The following rules must be implemented:

#### Header

- `version` must be a supported version.
- `sender` and `receiver` can't be the same.
- The `ParticipantId` variants of `sender` and `receiver` must match the message type.

#### Payloads

- Each jubjub type must be validated during deserialization.
- `share_commitments`:
  - Length must be less than or equal to `MAX_SIGNER_PARTICIPANT_ID`.
  - Length must be at least `MIN_SIGNERS` (`2` signers).
  - Duplicate `ParticipantId`s are invalid. This is implicit in the use of `BTreeMap` during serialization, but must be checked during deserialization.
  - Commitments must be serialized in ascending numeric `ParticipantId` order. This is the order of `BTreeMap.iter` during serialization, but must be checked during deserialization.
- `signing_commitments`:
    - Length must be less than or equal to `MAX_SIGNER_PARTICIPANT_ID`.
    - Length must be at least `MIN_THRESHOLD` (`2` required signers).
    - Signing packages that contain duplicate `ParticipantId`s are invalid. This is implicit in the use of `BTreeMap` during serialization, but must be checked during deserialization.
    - Signing packages must serialize in ascending numeric `ParticipantId` order. This is the order of `BTreeMap.iter` during serialization, but must be checked during deserialization..
- `message`: signed messages have a protocol-specific length limit. For Zcash, that limit is the maximum network protocol message length: `2^21` bytes (2 MB).

## Serialization/Deserialization

Each message struct needs to serialize to bytes representation before it is sent through the wire and must deserialize to the same struct (round trip) on the receiver side. We use `serde` and macro derivations (`Serialize` and `Deserialize`) to automatically implement where possible.

This will require deriving serde in several types defined in `frost.rs`. 
Manual implementation of serialization/deserialization will be located at a new mod `src/frost/serialize.rs`.

### Byte order

Each byte chunk specified below is in little-endian order unless is specified otherwise.

Multi-byte integers **must not** be used for serialization, because they have different byte orders on different platforms.

### Header

The `Header` part of the message is 18 bytes total:

Bytes | Field name | Data type
------|------------|-----------
1     | version    | u8
1     | msg_type   | u8
8     | sender     | u64
8     | receiver   | u64

### Frost types

The FROST types we will be using in the messages can be represented always as a primitive type. For serialization/deserialization purposes:

- `Commitment` = `AffinePoint`
- `Secret` = `Scalar`
- `GroupCommitment` = `AffinePoint`
- `SignatureResponse` = `Scalar`

### Primitive types

`Payload`s use data types that we need to specify first. We have 3 primitive types inside the payload messages:

#### `Scalar`

`jubjub::Scalar` is a an alias for `jubjub::Fr`. We use `Scalar::to_bytes` and `Scalar::from_bytes` to get a 32-byte little-endian canonical representation. See https://github.com/zkcrypto/bls12_381/blob/main/src/scalar.rs#L260 and https://github.com/zkcrypto/bls12_381/blob/main/src/scalar.rs#L232

#### `AffinePoint`

Much of the math in FROST is done using `jubjub::ExtendedPoint`. But for message exchange `jubjub::AffinePoint`s are a better choice, as their byte representation is smaller.

Conversion from one type to the other is trivial:

https://docs.rs/jubjub/0.6.0/jubjub/struct.AffinePoint.html#impl-From%3CExtendedPoint%3E
https://docs.rs/jubjub/0.6.0/jubjub/struct.ExtendedPoint.html#impl-From%3CAffinePoint%3E

We use `AffinePoint::to_bytes` and `AffinePoint::from_bytes` to get a 32-byte little-endian canonical representation. See https://github.com/zkcrypto/jubjub/blob/main/src/lib.rs#L443

#### VerificationKey

`redjubjub::VerificationKey<SpendAuth>`s can be serialized and deserialized using `<[u8; 32]>::from` and `VerificationKey::from`. See https://github.com/ZcashFoundation/redjubjub/blob/main/src/verification_key.rs#L80-L90 and https://github.com/ZcashFoundation/redjubjub/blob/main/src/verification_key.rs#L114-L121.

### Payload

Payload part of the message is variable in size and depends on message type.

#### `SharePackage`

Bytes           | Field name       | Data type
----------------|------------------|-----------
32              | group_public     | VerificationKey<SpendAuth>
32              | secret_share     | Share
1               | participants     | u8
(8+32)*participants | share_commitment | BTreeMap<ParticipantId, Commitment>

#### `SigningCommitments`

Bytes   | Field name          | Data type
--------|---------------------|-----------
32   | hiding | Commitment
32   | binding | Commitment

#### `SigningPackage`

Bytes                  | Field name         | Data type
-----------------------|--------------------|-----------
1                      | participants       | u8
(8+32+32)*participants | signing_commitments| BTreeMap<ParticipantId, SigningCommitments>
8                      | message_length     | u64
message_length         | message            | Vec\<u8\>


#### `SignatureShare`

Bytes | Field name | Data type
------|------------|-----------
32    | signature  | SignatureResponse

#### `AggregateSignature`

Bytes | Field name       | Data type
------|------------------|-----------
32    | group_commitment | GroupCommitment
32    | schnorr_signature| SignatureResponse

## Not included

The following are a few things this RFC is not considering:

- The RFC does not describe implementation-specific issues - it is focused on message structure and serialization.
- Implementations using this serialization should handle missing messages using timeouts or similar protocol-specific mechanisms.
    - This is particularly important for `SigningPackage`s, which only need a threshold of participants to continue.
- Messages larger than 4 GB are not supported on 32-bit platforms.
- Implementations should validate that message lengths are lower than a protocol-specific maximum length, then allocate message memory.
- Implementations should distinguish between FROST messages from different signature schemes using implementation-specific mechanisms.

### State-Based Validation

The following validation rules should be checked by the implementation:

- `share_commitments`: The number of participants in each round is set by the length of `share_commitments`.
    - If `sender` and `receiver` are a `ParticipantId::Signer`, they must be less than the number of participants in this round.
    - The length of `signing_commitments` must be less than or equal to the number of participants in this round.
- `signing_commitments`: Signing packages that contain missing `ParticipantId`s are invalid
    - Note: missing participants are supported by this serialization format.
    But implementations can require all participants to fully participate in each round.

If the implementation knows the number of key shares, it should re-check all the validation rules involving `MAX_SIGNER_PARTICIPANT_ID` using that lower limit.

## Testing plan

### Test Vectors

#### Conversion on Test Vectors

- Test conversion from `frost` to `message` on a test vector
    1. Implement the Rust `message` struct
    2. Implement conversion from and to the `frost` type
    3. Do a round-trip test from `frost` to `message` on a test vector
- Test conversion from `message` to bytes on a test vector
    1. Implement conversion from and to the `message` type
    2. Do a round-trip test from `message` to bytes on a test vector

#### Signing Rounds on Test Vectors

- Test signing using `frost` types on a test vector
    1. Implement a single round of `frost` signing using a test vector
- Test signing using `message` types on a test vector
- Test signing using byte vectors on a test vector

### Property Tests

#### Conversion Property Tests

- Create property tests for each message
    - Test round-trip conversion from `frost` to `message` types
    - Test round-trip serialization and deserialization for each `message` type

#### Signing Round Property Tests
    
- Create property tests for signing rounds
    - Test a signing round with `frost` types
    - Test a signing round with `message` types
    - Test a signing round with byte vectors
