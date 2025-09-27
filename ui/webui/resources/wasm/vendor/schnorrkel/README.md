# schnorrkel [![](https://img.shields.io/crates/v/schnorrkel.svg)](https://crates.io/crates/schnorrkel) [![](https://docs.rs/schnorrkel/badge.svg)](https://docs.rs/schnorrkel) [![](https://travis-ci.org/w3f/schnorrkel.svg?branch=master)](https://travis-ci.org/w3f/schnorrkel?branch=master)


Schnorrkel implements Schnorr signature on [Ristretto](https://ristretto.group) compressed Ed25519 points, as well as [related](https://github.com/sipa/bips/blob/bip-schnorr/bip-schnorr.mediawiki) protocols like HDKD, [MuSig](https://eprint.iacr.org/2018/068), and a verifiable random function (VRF).  

[Ristretto](https://doc.dalek.rs/curve25519_dalek/ristretto/index.html) implements roughly section 7 of Mike Hamburg's [Decaf](https://eprint.iacr.org/2015/673.pdf) paper to provide the 2-torsion free points of the Ed25519 curve as a prime order group.  ([related](https://forum.web3.foundation/t/account-signatures-and-keys-in-polkadot/70/3?u=burdges))

We employ the [merlin](https://github.com/dalek-cryptography/merlin) strategy of [type specific hashing methods](https://docs.rs/merlin/1.0.3/merlin/struct.Transcript.html) with sound domain separation.  These wrap Mike Hamburg's [STROBE128](https://strobe.sourceforge.io) construction for symmetric cryptography, itself based on Keccak.  

In practice, all our methods consume either a `merlin::Transcript` which developers create handily by feeding data to context specific builders.  We do however also support `&mut merlin::Transcript` like the `merlin` crate prefers.   We shall exploit this in future to adapt schnorrkel to better conform with the dalek ecosystem's zero-knowledge proof tooling. 

We model the VRF itself on ["Making NSEC5 Practical for DNSSEC"](https://eprint.iacr.org/2017/099.pdf) by Dimitrios Papadopoulos, Duane Wessels, Shumon Huque, Moni Naor, Jan Včelák, Leonid Rezyin, andd Sharon Goldberg.  We note the [V(X)EdDSA signature scheme](https://www.signal.org/docs/specifications/xeddsa/#vxeddsa) by Trevor Perrin at is basically identical to the NSEC5 construction.  Also, the VRF supports individual signers merging numerous VRF outputs created with the same keypair, which parallels the "DLEQ Proofs" and "Batching the Proofs" sections of ["Privacy Pass - The Math"](https://blog.cloudflare.com/privacy-pass-the-math/#dleqproofs) by Alex Davidson, and ["Privacy Pass: Bypassing Internet Challenges Anonymously"](https://www.petsymposium.org/2018/files/papers/issue3/popets-2018-0026.pdf)
by Alex Davidson, Ian Goldberg, Nick Sullivan, George Tankersley, and Filippo Valsorda.

Aside from some naive sequential VRF construction, we currently only support the three-round [MuSig](https://eprint.iacr.org/2018/068) for Schnorr multi-signatures, due to all other Schnorr multi-signatures being somewhat broken.  In future, we should develop secure schemes like mBCJ from section 5.1 starting page 21 of https://eprint.iacr.org/2018/417 however mBCJ itself works by proof-of-possession, while a [delinearized](http://crypto.stanford.edu/~dabo/pubs/abstracts/aggsurvey.html) variant sounds more applicable.


There are partial bindings for [C](https://github.com/Warchant/sr25519-crust), [JavaScript](https://github.com/paritytech/schnorrkel-js/), and [Python](https://gitlab.com/kauriid/schnorrpy/) as well.


