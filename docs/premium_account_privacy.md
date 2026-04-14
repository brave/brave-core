# Privacy-Preserving Subscription Credentials

Brave's premium services (VPN, Leo, Search Premium, etc.) use a blind token
protocol based on [Privacy Pass](https://privacypass.github.io/) to decouple
**payment identity** from **service usage**. When you use a premium feature,
the service verifies you have a valid subscription without learning who you
are.

## Overview

The system uses a **Verifiable Oblivious Pseudorandom Function (VOPRF)** via
the
[challenge-bypass-ristretto](https://github.com/nickel-city/challenge-bypass-ristretto)
library. The core idea: the server signs tokens it cannot see, and the client
later presents those tokens to access services. Because the server never saw the
original tokens, it cannot link a presented credential back to the signing
request.

```
Payment Provider              Subscription Service            Challenge Bypass Server (CBR)
(Stripe, Apple, Google)       (payment.bsg.brave.com)         (blind signer)
        |                              |                              |
        |  1. User pays                |                              |
        |----------------------------->|                              |
        |                              |  2. Order marked "paid"      |
        |                              |                              |
        |                              |                              |
Browser  --3. Generate random tokens-->|                              |
         --4. Blind tokens locally     |                              |
         --5. Send blinded tokens----->|--6. Forward blinded tokens-->|
                                       |<--7. Signed tokens + proof---|
Browser <--8. Return signed tokens-----|                              |
         --9. Verify DLEQ proof        |                              |
         --10. Unblind tokens locally  |                              |
                                       |                              |
                                Premium Service (Leo, VPN, etc.)
                                       |
Browser --11. Present unblinded token->|
                                       |--12. Verify via CBR--------->|
                                       |<--13. Valid (or 409 reused)--|
                                       |--14. Grant access----------->Browser
```

## Credential Lifecycle

### 1. Token generation (client-side)

After a purchase, the browser generates random tokens locally using a
cryptographic RNG and blinds them before transmitting anything
(`components/skus/browser/rs/lib/src/sdk/credentials/fetch.rs`):

```rust
let creds: Vec<Token> =
    iter::repeat_with(|| Token::random::<Sha512, _>(&mut csprng))
        .take(num_creds)
        .collect();
let blinded_creds: Vec<BlindedToken> =
    creds.iter().map(|t| t.blind()).collect();
```

Only the blinded versions leave the device.

### 2. Blind signing (server-side)

The subscription service forwards the blinded tokens to the **Challenge Bypass
Server (CBR)**, which signs them without ever seeing the originals. The CBR
returns:

*   **Signed tokens** -- blind signatures over the blinded tokens.
*   **Batch proof** -- a DLEQ (Discrete Log Equality) proof that the signing
    was performed correctly with the issuer's key.

### 3. Unblinding and verification (client-side)

The browser verifies the DLEQ batch proof and unblinds the signed tokens
(`fetch.rs`):

```rust
let unblinded_creds = batch_proof
    .verify_and_unblind::<Sha512, _>(
        &bucket_creds,         // original tokens (never sent to server)
        &bucket_blinded_creds, // blinded versions that were sent
        &signed_creds,         // server's blind signatures
        &public_key,           // issuer's public key
    )
    .or(Err(InternalError::InvalidProof))?;
```

After this step the browser holds signed tokens that are cryptographically
valid but **cannot be correlated** to the signing request by the server.

### 4. Credential presentation (client-side)

When accessing a premium service, the browser picks an unspent credential,
derives a verification key, and signs the issuer identifier to bind the token
to the specific service
(`components/skus/browser/rs/lib/src/sdk/credentials/present.rs`):

```rust
let verification_key =
    cred.unblinded_cred.derive_verification_key::<Sha512>();
let signature =
    verification_key.sign::<HmacSha512>(issuer.as_bytes()).encode_base64();
```

The presentation is sent as an HTTP cookie containing the token preimage,
HMAC signature, and validity window. No user identity is included.

### 5. Redemption (server-side)

The premium service forwards the presentation to the CBR, which verifies the
signature against its issuer key and checks the token has not been spent
before. On success the service grants access; a duplicate redemption returns
409 Conflict.

## Credential Types

| Type | Description |
|------|-------------|
| **TimeLimitedV2** | Primary production type. Tokens are bucketed into time windows (e.g. daily) with per-window sub-issuer keys. Single-use within each window. |
| **SingleUse** | One-time tokens using the same blind signature scheme. |
| **TimeLimited (v1)** | Legacy. Server-generated HMAC-derived tokens -- weaker privacy since the server produces the token values directly. |

TimeLimitedV2 credentials store a `valid_from` / `valid_to` window and a
`spent` flag per token in local storage, allowing the SDK to automatically
select the right credential for the current time period and track usage.

## Privacy Properties

| Property | How it is achieved |
|---|---|
| **Unlinkability** | Blind signing means the CBR signs tokens it cannot see; it cannot later match a presented token to a signing request. |
| **No identity in presentation** | The credential cookie contains only the token, its HMAC, and the validity window -- no account ID, email, or payment info. |
| **Double-spend prevention** | The CBR tracks redeemed token preimages and rejects duplicates (409 Conflict). |
| **Issuer binding** | The HMAC signature binds each token to a specific merchant and SKU (e.g. `brave.com?sku=leo-monthly`), preventing cross-service usage. |
| **Time-limited validity** | V3 issuers create sub-issuer keys per time window; expired credentials are cryptographically unverifiable. |

## What Each Party Knows

| Party | Knows | Does not know |
|---|---|---|
| **Payment provider** (Stripe, Apple, Google) | Your payment identity and that you purchased a Brave subscription | Which credentials you use or when you use services |
| **Subscription service** (payment.bsg.brave.com) | Your account, order, and that blinded tokens were requested | The original token values; cannot link signing requests to presented credentials |
| **Premium service** (Leo, VPN, Search, etc.) | That a valid credential was presented | Who presented it -- no account or payment info is included |
| **CBR** (blind signer) | That it signed some blinded tokens and that some tokens were redeemed | Cannot link signing to redemption due to the blinding step |

## Key Source Locations

| Component | Path |
|---|---|
| SKUs SDK (Rust) | `components/skus/browser/rs/lib/src/` |
| Token generation and fetch | `components/skus/browser/rs/lib/src/sdk/credentials/fetch.rs` |
| Credential presentation | `components/skus/browser/rs/lib/src/sdk/credentials/present.rs` |
| Data models | `components/skus/browser/rs/lib/src/models.rs` |
| SKUs C++ service | `components/skus/browser/skus_service_impl.cc` |
| VPN credential integration | `components/brave_vpn/browser/connection/brave_vpn_region_data_manager.cc` |
