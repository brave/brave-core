# Keys and addresses

Orchard keys and payment addresses are structurally similar to Sapling. The main change is
that Orchard keys use the Pallas curve instead of Jubjub, in order to enable the future
use of the Pallas-Vesta curve cycle in the Orchard protocol. (We already use Vesta as
the curve on which Halo 2 proofs are computed, but this doesn't yet require a cycle.)

Using the Pallas curve and making the most efficient use of the Halo 2 proof system
involves corresponding changes to the key derivation process, such as using Sinsemilla
for Pallas-efficient commitments. We also take the opportunity to remove all uses of
expensive general-purpose hashes (such as BLAKE2s) from the circuit.

We make several structural changes, building on the lessons learned from Sapling:

- The nullifier private key $\mathsf{nsk}$ is removed. Its purpose in Sapling was as
  defense-in-depth, in case RedDSA was found to have weaknesses; an adversary who could
  recover $\mathsf{ask}$ would not be able to spend funds. In practice it has not been
  feasible to manage $\mathsf{nsk}$ much more securely than a full viewing key, as the
  computational power required to generate Sapling proofs has made it necessary to perform
  this step on the same device that is creating the overall transaction (rather than on a
  more constrained device like a hardware wallet). We are also more confident in RedDSA
  now.

- $\mathsf{nk}$ is now a field element instead of a curve point, making it more efficient
  to generate nullifiers.

- $\mathsf{ovk}$ is now derived from $\mathsf{fvk}$, instead of being derived in parallel.
  This places it in a similar position within the key structure to $\mathsf{ivk}$, and
  also removes an issue where two full viewing keys could be constructed that have the
  same $\mathsf{ivk}$ but different $\mathsf{ovk}$s. Users still have control over whether
  $\mathsf{ovk}$ is used when constructing a transaction.

- All diversifiers now result in valid payment addresses, due to group hashing into Pallas
  being specified to be infallible. This removes significant complexity from the use cases
  for diversified addresses.

- The fact that Pallas is a prime-order curve simplifies the protocol and removes the need
  for cofactor multiplication in key agreement. Unlike Sapling, we define public (including
  ephemeral) and private keys used for note encryption to exclude the zero point and the
  zero scalar. Without this change, the implementation of the Orchard Action circuit would
  need special cases for the zero point, since Pallas is a short Weierstrass rather than
  an Edwards curve. This also has the advantage of ensuring that the key agreement has
  "contributory behaviour" — that is, if *either* party contributes a random scalar, then
  the shared secret will be random to an observer who does not know that scalar and cannot
  break Diffie–Hellman.

Other than the above, Orchard retains the same design rationale for its keys and addresses
as Sapling. For example, diversifiers remain at 11 bytes, so that a raw Orchard address is
the same length as a raw Sapling address.

Orchard payment addresses do not have a stand-alone string encoding. Instead, we define
"unified addresses" that can bundle together addresses of different types, including
Orchard. Unified addresses have a Human-Readable Part of "u" on Mainnet, i.e. they will
have the prefix "u1". For specifications of this and other formats (e.g. for Orchard viewing
and spending keys), see section 5.6.4 of the NU5 protocol specification [#NU5-orchardencodings].

## Hierarchical deterministic wallets

When designing Sapling, we defined a [BIP 32]-like mechanism for generating hierarchical
deterministic wallets in [ZIP 32]. We decided at the time to stick closely to the design
of BIP 32, on the assumption that there were Bitcoin use cases that used both hardened and
non-hardened derivation that we might not be aware of. This decision created significant
complexity for Sapling: we needed to handle derivation separately for each component of
the expanded spending key and full viewing key (whereas for transparent addresses there is
only a single component in the spending key).

Non-hardened derivation enables creating a multi-level path of child addresses below some
parent address, without involving the parent spending key. The primary use case for this
is HD wallets for transparent addresses, which use the following structure defined in
[BIP 44]:

- (H) BIP 44
  - (H) Coin type: Zcash
    - (H) Account 0
      - (N) Normal addresses
        - (N) Address 0
        - (N) Address 1...
      - (N) Change addresses
        - (N) Change address 0
        - (N) Change address 1...
    - (H) Account 1...

Shielded accounts do not require separating change addresses from normal addresses, because
addresses are not revealed in transactions. Similarly, there is also no need to generate
a fresh spending key for every transaction, and in fact this would cause a linear slow-down
in wallet scanning. But for users who do want to generate multiple addresses per account,
they can generate the following structure, which does not use non-hardened derivation:

- (H) ZIP 32
  - (H) Coin type: Zcash
    - (H) Account 0
      - Diversified address 0
      - Diversified address 1...
    - (H) Account 1...

Non-hardened derivation is therefore only required for use-cases that require the ability
to derive more than one child layer of addresses. However, in the years since Sapling was
deployed, we have not seen *any* such use cases appear.

Therefore, for Orchard we only define hardened derivation, and do so with a much simpler
design than ZIP 32. All derivations produce an opaque binary spending key, from which the
keys and addresses are then derived. As a side benefit, this makes key formats
shorter. (The formats that will actually be used in practice for Orchard will correspond
to the simpler Sapling formats in the protocol specification, rather than the longer
and more complicated "extended" ones defined by ZIP 32.)

[BIP 32]: https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki
[BIP 44]: https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
[ZIP 32]: https://zips.z.cash/zip-0032
[NU5-orchardencodings]: https://zips.z.cash/protocol/nu5.pdf#orchardencodings
