# Brave Browser Cryptographic Features

This document captures those cryptographic features in the Brave browser
that are beyond the standard encryption available in every Chromium-based
browser.

| Feature | Cryptography | Platform | Notes | Open Source |
| :--- | :--- | :--- | :--- | :--- |
| Brave Accounts | RFC 9807 | all | Similar to what's been available in Firefox for 20+ years | Yes |
| Brave Premium | Based on [PrivacyPass](https://github.com/brave/brave-core/blob/master/docs/premium_account_privacy.md) (RFC 9576) | all | To validate credentials | Yes |
| Brave Rewards | VOPRF (RFC9497) | | Anonymous rewards and anonymous access to Premium Services | Yes |
| Brave Leo NEAR Integration | HPKE for [OHTTP](https://datatracker.ietf.org/doc/draft-ietf-ohai-chunked-ohttp/) (RFC9458) | | Establish secure channel to inference | Yes |
| Brave Wallet ZEC chain | RedPallas Signature scheme. ZkSNARKs system for zero-knowledge proofs | TBD (Desktop at least) | Follows [Orchard Shielded Protocol](https://zips.z.cash/zip-0224) which uses RedPallas | Yes |
| Brave Wallet | ECDSA w/ curve Secp256k1 (koblitz curve not required in TLS) | all | Signing transactions for Ethereum, Bitcoin, Filecoin, and ZCash public transactions. ECDSA is used in chromium but for TLS and we're just signing different messages | Yes |
| Brave Wallet | EdDSA w/ curve Ed25519 (RFC8032) | all | Signing transactions for SOL, ADA. The cryptography here is used in chromium, we're just signing different messages | Yes |
| Brave Wallet | Sr25519 (Schnorr Signatures w/ Ristretto255 curve, Ed25519 under-the-hood) | Not released | Signing for DOT. Not released yet, but incoming for polkadot feature | Yes |
| Brave Wallet | BLS12-381 | all | Only used if a user imports a filecoin account. We don't generate keys for this signature scheme by default | Yes |
| Brave Wallet | PBKDF2 (RFC2898) | all | Derive password into an encryption key for the wallet | Yes |
| Brave [P3A](https://support.brave.app/hc/en-us/articles/9140465918093-What-is-P3A-in-Brave) | PPOPRF (puncturable partially oblivious pseudorandom function) and Shamir Secret Sharing for [STAR protocol](https://github.com/brave/sta-rs) | all | Privacy-preserving telemetry that provides K-anonymity to Brave clients via [STAR](https://brave.com/privacy-updates/19-star/) | Yes |
