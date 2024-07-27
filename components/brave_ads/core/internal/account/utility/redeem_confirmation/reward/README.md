# Brave Rewards User Confirmation Token Redemption

Return to [confirmation redemption](../../../utility/redeem_confirmation/README.md).

Redeem an anonymous confirmation token and receive a [payment token](../../redeem_payment_tokens/README.md) in return by making a call to https://anonymous.ads.brave.com/v{version}/confirmation/{transactionId}/{credential}.

Here, `{version}` represents the version number of the targeted API, `{transactionId}` is a unique id that is not linkable between confirmation token redemptions, and `{credential}` is a digital signature used to ensure the integrity and authenticity of the data being sent.

The request body includes the [Brave Rewards user confirmation token payload](../../../confirmations/reward/README.md).

Please add to it!
