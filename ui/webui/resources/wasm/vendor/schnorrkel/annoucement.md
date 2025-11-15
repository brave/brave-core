

We are pleased to announce the wire format stabilisation of the primary feature set of our [schnorrkel](https://github.com/w3f/schnorrkel) crate ([docs](https://docs.rs/schnorrkel)), which provides safer access to basic functionality now expected from signatures on blockchains.  In particular, schnorrkel provides Schnorr signatures, a fast Schnorr DLEQ proof based VRF, hierarchical deterministic key derivation (HDKD), and the safest currently known three round trip Schnorr multi-signature variant.

We want more diverse functionality from signatures used in blockchain applications than from signatures used only in TLS, PGP, etc.  Schnorr signatures support more [functionality](https://github.com/sipa/bips/blob/bip-schnorr/bip-schnorr.mediawiki) than ECDSA, and do so far more simply.  In particular, multi-signatures and threshold signatures are vastly simpler with Schnorr signatures.  

Also, the security arguments for Schnorr signatures require only standard assumptions plus the hash function being a random oracle.  By comparison, ECDSA arguments employ ad hoc dubious assumptions like an elliptic curve point compression function being a random oracle.  The simpler arguments ror Schnorr more often provide clear answers about the security of Schnorr composed with other protocols.

We use Schnorr-like signature schemes in schnorrkel for the reason above.

We believe schnorrkel strikes an optimal balance between security concerns, including defense-in-depth and miss-use resistance, and the flexibility and diverse functionality wanted from signature schemes in blockchain protocols.   We achieve this balance in basically two ways:  First, we select modern safer cryptographic primitives which actually extend the strongest primitives in widespread use.  Second, we assess the security of the protocols we provide as one cohesive whole, not piecemeal. 

In the subsequent two sections, we discuss first how these modern primitives reduce risks, then the interesting features schnorrkel provides, and finally our direction for the future.  

### Primitives

As primitives, we adopt (1) the [Ristretto](https://doc.dalek.rs/curve25519_dalek/ristretto/index.html) implementation of section 7 of Mike Hamburg's [Decaf](https://eprint.iacr.org/2015/673.pdf) paper for our elliptic curve arithmetic, as well as (2) the [merlin](https://doc.dalek.rs/merlin/index.html) limited implementation of Mike Hamberg's [STROBE](http://strobe.sourceforge.io/) for our hashing.

There are many cryptographic protocols that demand a group of large prime order $q$ (aka the number of points or group elements) in which the discrete logarithm problem is hard.  There are two competing design goals in implementing such groups, the [strongest](https://safecurves.cr.yp.to/) elliptic curve choices, like Edwards curves, come with a non-trivial cofactor $h$, meaning the elliptic curve actually has non-prime order $h q$. 

You can adapt simple protocols to a non-trivial cofactor $h$ easily enough by adding a few multiplications by $h$, but they often inherit some malleability and more complex protocols are famously bug prone, including when build from simpler malleabile protocols.  Monero has had multiple serious cofactor bugs, including at least one "infinite inflation" bug in which users could repeatedly multiply their balances' value by the cofactor $h=8$.  In RedDSA, ZCash spent considerable effort excising all malleability from simple Schnorr signtures (Ed25519). 

There are curves like secp256k1 that actually have prime order, making complex protocols safer, but these curve appear [weaker](https://safecurves.cr.yp.to/disc.html) and [serious](https://safecurves.cr.yp.to/ladder.html) [implementation](https://twitter.com/isislovecruft/status/1154184043791040512) [pitfalls](https://safecurves.cr.yp.to/complete.html) [abound](https://safecurves.cr.yp.to/ind.html) for the curves themselves.  And [some](https://mailarchive.ietf.org/arch/msg/tls/Ng1eNx-Q130FWdmYZCER4mlXqMc) people [distrust](https://safecurves.cr.yp.to/rigid.html) the more standard NIST curves like P-256.

In 2015, Mike Hamburg resolved this conflict in his [Decaf](https://eprint.iacr.org/2015/673.pdf) paper, by using an Edwards curve for performance and safety, but using an isogenous prime order curve for serialisation and testing equality.  

[Ristretto](https://doc.dalek.rs/curve25519_dalek/ristretto/index.html) implements roughly section 7 of Mike Hamburg's [Decaf](https://eprint.iacr.org/2015/673.pdf) paper, which handles cofactor 8.  It uses the Ed25519 curve under the hood, so any weaknesses translate into weaknesses in Ed25519.

We employ Ristretto throughout schnorrkel, which reduces malleability, simplifies the analysis of our higher level protocols, and makes it safer to compose schnorrkel protocols with each other and with other similar protocols.

Also, there are minor practical hiccups with the hash functions designed for NIST competitions, but most especially that they create a byte stream oriented interface over a block oriented permutation.  This improves performance of computing MACs on data that arrives in order but piecemeal, but weakens the permutation's natural domain seperation.  We need strong domain seperation in more complex protocols like signatures and NIZKs.  [STROBE](https://strobe.sourceforge.io/) is a strong general purpose symmetric cryptography construction based on Keccak-f(1600), the permutation driving the SHA3 competition winner.  [Merlin](https://doc.dalek.rs/merlin/index.html) is a STROBE scheme for NIZKs, which does almost perfect domain seperation.  

In principle, ristretto and merlin together should let schnorrkel play nicely with other future dalek ecosystem crates, like [bulletproofs](https://github.com/dalek-cryptography/bulletproofs).  We've no current plans to exploit this, but this should simplify weak anonymity parachains analogous to Monero or Mimblewimble. 

### Nonces

Almost all fast signature-like protocols hide the secret key using random nonces aka witnesses.  ECDSA should invoke the system CSPRNG for this.  Yet, there are many bitcoin wallets whose ECDSA does not correctly source randomness, and many small devices with weak CSPRNGs, which resulted in the compromise of numerous bitcoin accounts.

We should largely blame bitcoin's original core developers for these wallet developers' mistakes.  Ed25519 has always avoided randomness fears during singing by hashing the message with secret key data to generate "derandomized" nonces.  If one Ed25519 key signs the same message twice then both use the same nonce, but this creates no vulnerability in a simple Schnorr signature like Ed25519. 

There are more advanced protocols that cannot be derandomized, including multi-signatures, as well as schemes like batch verification, in which derandomization becomes a delicate Fiat-Shamir transform.  We therefore cannot apply derandomization blindly when going beyond simple signatures.

In schnorrkel, we adopt merlin's favoured approachs for nonce generation:  We first apply derandomization but then hash output with some CSPRNG immediately before returning the nonce.  We make the OS's CSPRNG a strong default for this, so it normally provides the security of both approaches.  We must permit user supplied CSPRNGs for test vectors of course.  

There are risks when using our multi-signatures on hardware devices without randomness, but only against your cosigners, and much less than with classical randomized ECDSA signatures.  

### HDKD

Almost all functionality enhancements exploit some limited form of malleability, so their use can make aspects of the signature scheme more fragile.  As such, they should be evaluated carefully, not just alone, but together.  

There are two bizarre signature variations already in heavy usage by current blockchains:  

First, wallets support multiple related addresses using hierarchical deterministic key derivation (HDKD) aka BIP32.  You cannot prevent users from doing HDKD because you cannot remove the public key malleability employed.  We embrace HDKD by providing [clear derivation  routines](https://github.com/w3f/schnorrkel/blob/master/src/derive.rs) in schnorrkel.

As an aside, we also published a crate doing this in [Ed25519](https://github.com/w3f/hd-ed25519), and Tor has a related scheme, but [warn against most variants published for blockchain use](https://forum.web3.foundation/t/key-recovery-attack-on-bip32-ed25519/44).

Second, the ecrecover scheme used with ECDSA on Ethereum saves 32 bytes per signature producing the public key as an output, instead of taking it as an input.  We love saving blocksize, but ecrecover prevents batch verification, making block verification significantly heavier.  We note batch verifications done with ristretto even benefit from [Pippenger multiscalar multiplication](https://github.com/dalek-cryptography/curve25519-dalek/pull/249).  

In ECDSA, there are no known problems with using ecrecover and HDKD together, but using HDKD actually breaks both BLS signatures and the malleable Schnorr signatures that [support ecrecover.](https://www.deadalnix.me/2017/02/17/schnorr-signatures-for-not-so-dummies/).  We therefore use  non-malleable signatures that forbid ecrecover.  

We do not support BLS keys for accounts in polkadot for the same reasons and more.  We note BLS reamins usable in more controlled environments, like consensus protocols.

### Verifiable Random Functions (VRF)

A verifiable random function (VRF) is the public-key analog of a cryptographic hash function with a distinguished key, such as many MACs.  Anyone with the input and the public key can verify the correctness of the VRF output, but only the secret key holder could compute this output.  A hash function requires the key be shared for verification. 

VRFs have become increasingly important building blocks across an array of protocols.  Applications include preventing offline enumeration in directory services like certificate transparency, random beacons, leader selection and common coins in consensus protocols, block production in proof-of-stake, and assigning work so as to prevent denial-of-service attacks. 

We [implement](https://github.com/w3f/schnorrkel/blob/master/src/vrf.rs) a VRF in schnorrkel for polkadot's block production scheme BABE.  We based schnorrkel's VRF on ["Making NSEC5 Practical for DNSSEC"](https://eprint.iacr.org/2017/099.pdf) but again use Ristretto and merlin.  Some similar VRFs designs include [V(X)EdDSA](https://www.signal.org/docs/specifications/xeddsa/#vxeddsa) and the [IRTF CFRG draft](https://tools.ietf.org/html/draft-irtf-cfrg-vrf-05).

We improve upon these similar schemes in three important ways:

We reduce VRF output malleability by [hashing](https://github.com/w3f/schnorrkel/blob/master/src/vrf.rs#L125) the signer's public key along side the input, which dramatically improves security when used with HDKD.  An adversary could otherwise compute the VRF output for one key from the VRF output for another public key related by a soft derivation.

We also [hash](https://github.com/w3f/schnorrkel/blob/master/src/vrf.rs#L287) the VRF input and output together when providing output used elsewhere, which improves compossibility in security proofs.  See the 2Hash-DH construction from Theorem 2 on page 32 in appendix C of ["Ouroboros Praos: An adaptively-secure, semi-synchronous proof-of-stake blockchain"](https://eprint.iacr.org/2017/573.pdf).  

We avoid complexity around cofactors by using Ristretto, while all three similar VRF designs above handle the cofactor slightly differently, and do not consider factors addressed in RedDSA.  

In the schnorrkel VRF, we expose a general discrete-log equality (DLEQ) proof interface that sacrifices some of these protections for flexibility.  We expect this an interface to simplify developing more complex protocols, like publicly verifiable secret sharing (PVSS), but sounds miss-use prone without Ristretto.

Also, the schnorrkel VRF supports individual signers merging numerous VRF outputs created with the same keypair, which parallels the "DLEQ Proofs" and "Batching the Proofs" sections of ["Privacy Pass - The Math"](https://blog.cloudflare.com/privacy-pass-the-math/#dleqproofs) by Alex Davidson, and ["Privacy Pass: Bypassing Internet Challenges Anonymously"](https://www.petsymposium.org/2018/files/papers/issue3/popets-2018-0026.pdf)
by Alex Davidson, Ian Goldberg, Nick Sullivan, George Tankersley, and Filippo Valsorda.

### Multi-signatures

In schnorrkel, we [implement](https://github.com/w3f/schnorrkel/blob/master/src/musig.rs) the three round trip Schnorr multi-signature scheme from [_"Simple Schnorr Multi-Signatures with Applications to Bitcoin"_](https://eprint.iacr.org/2018/068) by Gregory Maxwell, Andrew Poelstra, Yannick Seurin, and Pieter Wuille.  We enforce this three round trip protocol using "session types", so users cannot deviate form the protocol, except by using only `unsafe` code, rewinding their machine somehow, or similar. 

There are attacks on all previously known two round trip Schnorr multi-signature schemes given in [_"On the Security of Two-Round Multi-Signatures"_](https://eprint.iacr.org/2018/417.pdf) by Manu Drijvers, Kasra Edalatnejad, Bryan Ford, Eike Kiltz, Julian Loss, Gregory Neven, and Igors Stepanovs.  We acknowledge these "concurrency" attacks sound extremely challenging, with two round trip versions actually being secure in various circumstances.  Yet, we do not known any defenses that really appear easily enforceable from library code, so the three round trip version remains preferable for now.

There is one secure two round signature scheme called mBCJ provided in section 5 starting on page 21 of this second article.  Actually deploying mBCJ would require distinct signature verification code, which sets a high bar.  As mBCJ involves some questionable design decisions, we currently view mBCJ merely as a proof that two round trip schemes exist, not actually a deployment target.  

We continue searching for a suitable two-round trip multi-signature scheme, but with lesser ambitions than [Blockstream's effort for threshold signatures on secp256k1](https://github.com/ElementsProject/secp256k1-zkp/blob/f6e4f869e03e1b4e0628e173745764038a2715f7/src/modules/thresholdsig/design.md), as some of their concerns sound unsolvable.  

### Threshold multi-signature goals

We do not currently support threshold signing in our multi-signature scheme.  We expect to do so in future, but so far our priorities lay elsewhere.  We'd also prefer to find a two round trip version before implementing threshold multi-signature.  In fact, there are two several threshold multi-signatures approaches whose costs and benefits depend upon whether we require ne verification code. 

An accountable threshold signature requires your signing key contain either all signers, or else a Merkle root of all signers, so that signatures can identify the signer subset with either a bitfield, or else a Merkle proofs for each signer, respectively.  Any such signature requires bespoke verification code, which again sets a high bar.  We'll do this from the beginning if deploying any scheme that requires new verification code, like mBCJ.

There are non-accountable threshold signing algorithms using Lagrange polynomial interpolation that work for almost any signature scheme, including our existing multi-signature that uses only a standard Schnorr verifier.  Yet, these require a distributed key generation protocol (DKG) using verifiable threshold secret sharing (VSS), which creates one problem:

We cannot permit the threshold shared secret key to be used for anything else, including soft derivations:  If Alice's wallet A has a soft derived wallet B used in a DKG then a threshold of DKG participants can obtain her secret key for B, and derive her secret key for A.  In fact, these DKGs actually create a new shared secret key, on which soft but not hard derivations again make sense, but actually sorting all this out in a miss-use resistant way requires some care.  As an example, we might ask signers to handle keypairs with combined public keys, and lone combined public keys, but never lone secret keys, which likely requires some new keypair that denied uses access to the secret key.  

We think foremost among the threshold use cases being deployed now lies the wallet with one share being recoverable from a central server via a PAKE.  We want to support this case without either adding undo complexity or diving into the hardware wallet market outselves.  ;)

As an aside, there are now several approaches to doing threshold multi-signatures with ECDSA, like https://eprint.iacr.org/2018/499.pdf and https://eprint.iacr.org/2019/114.pdf and https://eprint.iacr.org/2017/552.pdf, as implemented in [KZen](https://github.com/KZen-networks/multi-party-ecdsa).  These are complex protocols that present interesting targets for attacks, both using the concurrency methods, as well as novel combinations of ecrecover, HDKD, and one another.  I expect ECDSA to eventually crack under the weight of enough such extensions. 

### Future 

In future, we want schnorrkel to grow by providing an even more diverse array of cryptographic building blocks, while retaining our existing safety promisses.  We therefore welcome discussions with other implementors around our future directions, like threshold multi-signatures, but also tooling for layer two solutions, like adaptor, blind, and ring signatures.



