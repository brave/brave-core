#![doc = include_str!("../dkg.md")]
use super::*;

/// DKG Round 1 structures.
pub mod round1 {
    use super::*;

    /// The secret package that must be kept in memory by the participant
    /// between the first and second parts of the DKG protocol (round 1).
    ///
    /// # Security
    ///
    /// This package MUST NOT be sent to other participants!
    pub type SecretPackage = frost::keys::dkg::round1::SecretPackage<P>;

    /// The package that must be broadcast by each participant to all other participants
    /// between the first and second parts of the DKG protocol (round 1).
    pub type Package = frost::keys::dkg::round1::Package<P>;
}

/// DKG Round 2 structures.
pub mod round2 {
    use super::*;

    /// The secret package that must be kept in memory by the participant
    /// between the second and third parts of the DKG protocol (round 2).
    ///
    /// # Security
    ///
    /// This package MUST NOT be sent to other participants!
    pub type SecretPackage = frost::keys::dkg::round2::SecretPackage<P>;

    /// A package that must be sent by each participant to some other participants
    /// in Round 2 of the DKG protocol. Note that there is one specific package
    /// for each specific recipient, in contrast to Round 1.
    ///
    /// # Security
    ///
    /// The package must be sent on an *confidential* and *authenticated* channel.
    pub type Package = frost::keys::dkg::round2::Package<P>;
}

/// Performs the first part of the distributed key generation protocol
/// for the given participant.
///
/// It returns the [`round1::SecretPackage`] that must be kept in memory
/// by the participant for the other steps, and the [`round1::Package`] that
/// must be sent to other participants.
pub fn part1<R: RngCore + CryptoRng>(
    identifier: Identifier,
    max_signers: u16,
    min_signers: u16,
    mut rng: R,
) -> Result<(round1::SecretPackage, round1::Package), Error> {
    frost::keys::dkg::part1(identifier, max_signers, min_signers, &mut rng)
}

/// Performs the second part of the distributed key generation protocol
/// for the participant holding the given [`round1::SecretPackage`],
/// given the received [`round1::Package`]s received from the other participants.
///
/// It returns the [`round2::SecretPackage`] that must be kept in memory
/// by the participant for the final step, and the [`round2::Package`]s that
/// must be sent to other participants.
pub fn part2(
    secret_package: round1::SecretPackage,
    round1_packages: &[round1::Package],
) -> Result<(round2::SecretPackage, Vec<round2::Package>), Error> {
    frost::keys::dkg::part2(secret_package, round1_packages)
}

/// Performs the third and final part of the distributed key generation protocol
/// for the participant holding the given [`round2::SecretPackage`],
/// given the received [`round1::Package`]s and [`round2::Package`]s received from
/// the other participants.
///
/// It returns the [`KeyPackage`] that has the long-lived key share for the
/// participant, and the [`PublicKeyPackage`]s that has public information
/// about all participants; both of which are required to compute FROST
/// signatures.
pub fn part3(
    round2_secret_package: &round2::SecretPackage,
    round1_packages: &[round1::Package],
    round2_packages: &[round2::Package],
) -> Result<(KeyPackage, PublicKeyPackage), Error> {
    frost::keys::dkg::part3(round2_secret_package, round1_packages, round2_packages)
}
