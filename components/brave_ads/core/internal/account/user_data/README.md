# User Data

> [!IMPORTANT]
> The privacy team's approval is required for new user data.

> [!NOTE]
> **Optional** user data may not be included in the confirmation payload. **Mutable** user data can change when a confirmation redemption fails and is retried later.

## Non-Brave Rewards User

#### User Data Submitted when Redeeming Anonymous Confirmations

Included in a "confirmation" payload. See [anonymous confirmation redemption](../utility/redeem_confirmation/README.md). **[TODO: Update link]**

| key  | optional  | mutable  | description  |
|---|---|---|---|
| conversion.action  | yes  | no  | i.e., view-through (brave-news only) otherwise just click-through conversion. **[TODO: Reword]**  |
| httpResponseStatus  | yes  | no  | Whether the navigation committed an error page. **[TODO: Mention that this is only sent for page lands]**  |

## Brave Rewards User

#### User Data Submitted when Redeeming Anonymous Confirmation Tokens

Included in a "confirmation token" payload. See [anonymous confirmation token redemption](../utility/redeem_confirmation/README.md). **[TODO: Update link]**

| key  | optional  | mutable  | description  |
|---|---|---|---|
| buildChannel  | no  | no  | Browser build channel.  |
| catalog  | no  | no  |  Catalog identifier.  |
| conversion.action  | yes  | no  | i.e., view-through or click-through conversion. **[TODO: Reword]**  |
| conversion.envelope  | yes  | no  | Encrypted verifiable envelope.  |
| createdAtTimestamp  | no  | no  | Privacy-preserving [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) timestamp.  |
| diagnosticId  | yes  | yes  | Diagnostic id from [brave://rewards-internals](brave://rewards-internals) to help diagnose issues.  |
| httpResponseStatus  | yes  | no  | Whether the navigation committed an error page. **[TODO: Mention that this is only sent for page lands]**  |
| locale  | yes  | no  | Privacy-preserving operating system locale  |
| platform  | no  | no  | Operating system.  |
| rotatingHash  | yes  | no  | Privacy-preserving time-based rotating hash.  |
| segment  | yes  | no  | Advertising taxonomy for the chosen ad.  |
| studies  | no  | no  | User studies. See [Griffin](https://github.com/brave/brave-browser/wiki/Brave-Variations-(Griffin)).  |
| systemTimestamp  | no  | yes  | Privacy-preserving [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) timestamp.  |
| topSegment  | yes  | no  | Top user interest segment. |
| versionNumber  | no  | no  | Browser version number.  |

#### User Data Submitted when Redeeming Identifiable Payment Tokens

See [identifiable payment token redemption](../utility/redeem_payment_tokens/README.md).

| user data  | optional  | mutable  | description  |
|---|---|---|---|
| platform  | no  | no  | Operating system.  |
| summary  | no  | no  | Summary of confirmation tokens for each creative.  |

Please add to it!
