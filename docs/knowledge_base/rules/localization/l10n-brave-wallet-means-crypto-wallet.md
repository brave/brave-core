---
name: brave/l10n-brave-wallet-means-crypto-wallet
description:
  '"Brave Wallet" in user-facing strings refers only to the cryptocurrency
  wallet, never to autofill or payment card features.'
rubric: readability
severity: should_fix
---

**Rule.** Use "Brave Wallet" only for the cryptocurrency wallet. Do not use it,
or "wallet" in a way that suggests it, for autofill, saved payment cards or
other payment features.

**Why.** Mixing the two leads users to believe their cards are stored in, or
used by, the crypto wallet.

## Examples

Bad:

```xml
<message name="IDS_AUTOFILL_SAVE_CARD" desc="Prompt to save a payment card for autofill">
  Save this card to Brave Wallet?
</message>
```

Good:

```xml
<message name="IDS_AUTOFILL_SAVE_CARD" desc="Prompt to save a payment card for autofill">
  Save this card?
</message>
```
