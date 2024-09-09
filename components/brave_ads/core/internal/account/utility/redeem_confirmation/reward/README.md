# Brave Rewards User Confirmation Token Redemption

Return to [confirmation redemption](../../../utility/redeem_confirmation/README.md).

Redeem an anonymous confirmation token and receive a [payment token](../../redeem_payment_tokens/README.md) in return by making a call to https://anonymous.ads.brave.com/v{version}/confirmation/{transactionId}/{credential}.

Here, `{version}` represents the version number of the targeted API, `{transactionId}` is a unique id that is not linkable between confirmation token redemptions, and `{credential}` is a digital signature used to ensure the integrity and authenticity of the data being sent.

The request body includes the [Brave Rewards user confirmation token payload](../../../confirmations/reward/README.md).

Here's a breakdown of the process:

1. The client creates a random token.
2. The client disguises the token using a blinding scalar to hide its original value.
3. The server generates a random signing key.
4. The server generates a random public key.
5. The server signs the disguised token with its public key, endorsing it.
6. The server creates a batch Discrete Log Equivalence (DLEQ) proof to show consistency across multiple disguised tokens.
7. The client verifies the proof using the public key provided by the server to confirm validity.
8. The client reveals the original tokens by unblinding them.
9. The client periodically redeems unblinded tokens, also known as payment tokens, with the server.

This protocol is effective for maintaining anonymity and ensuring secure transactions. For a more detailed explanation, see the [challenge bypass ristretto test](../../common/challenge_bypass_ristretto/challenge_bypass_ristretto_test.cc).

Please add to it!
