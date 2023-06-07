# User Data

Mutable user data is changed when a confirmation fails and is retried later.

## Redeemed with Anonymous Confirmation Tokens

See [anonymous confirmation token redemption](../utility/redeem_confirmation/README.md).

| user data  | optional  | mutable  | description  |
|---|---|---|---|
| buildChannel  | no  | no  | Browser build channel  |
| catalog  | no  | no  |  Catalog id  |
| conversion  | yes  | no  | Envelope encryption for verifiable conversions  |
| createdAtTimestamp  | no  | no  | Privacy-preserving [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) timestamp  |
| diagnosticId  | yes  | yes  | Diagnostic id from brave://rewards-internals to help diagnose issues.  |
| locale  | yes  | no  | Privacy-preserving operating system locale  |
| platform  | no  | no  | Operating system  |
| rotating_hash  | yes  | no  | Privacy-preserving time-based rotating hash  |
| segment  | yes  | no  | Advertising segment taxonomy for the chosen ad  |
| studies  | no  | no  | User studies. See [Griffin](https://github.com/brave/brave-browser/wiki/Brave-Variations-(Griffin))  |
| systemTimestamp  | no  | yes  | Privacy-preserving [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) timestamp  |
| versionNumber  | no  | no  | Browser version number  |

## Redeemed with Identifiable Confirmation Tokens

See [identifiable confirmation token redemption](../utility/redeem_unblinded_payment_tokens/README.md).

| user data  | optional  | mutable  | description  |
|---|---|---|---|
| platform  | no  | no  | Operating system  |
| summary  | no  | no  | Total count of confirmation tokens for each creative  |

Please add to it!
