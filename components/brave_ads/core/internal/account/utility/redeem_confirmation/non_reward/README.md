# Non-Brave Rewards User Confirmation Redemption

Return to [confirmation redemption](../../../utility/redeem_confirmation/README.md).

Redeem an anonymous confirmation, by making a call to https://anonymous.ads.brave.com/v{version}/confirmation/{transactionId}.

Here, `{version}` represents the version number of the targeted API, and `{transactionId}` is a unique identifier that cannot be linked between different confirmation redemptions.

The request body includes the [non-Brave Rewards user confirmation payload](../../../confirmations/non_reward/README.md).

Please add to it!
