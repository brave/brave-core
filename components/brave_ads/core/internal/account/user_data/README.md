# User Data

Return to [confirmation redemption](../../account/utility/redeem_confirmation/README.md).

> [!IMPORTANT]
> Approval from the privacy team is required for changes to existing user data or for new user data.

## Non-Brave Rewards User

### User Data Submitted when Redeeming Anonymous Confirmations

Included as part of the "confirmation" payload. See [non-Brave Rewards user confirmation payload](../confirmations/non_reward/README.md).

| key  | example  | description  |
|---|---|---|
| conversion.action[^2]  | {"conversion":[{"action":"click"}]}  | Conversion attribution to determine which advertising efforts led a user to view or click on an ad and subsequently complete a desired action. Also, see [reporting metrics](https://ads-help.brave.com/campaign-performance/reporting/#available-reporting-metrics-in-brave-ads-manager).<br><br> Supported types:<br><br>Brave Search ads:<br>- click-through<br><br>Brave News ads:<br>- view-through<br>- click-through  |

## Brave Rewards User

### User Data Submitted when Redeeming Anonymous Confirmation Tokens

Included as part of the "confirmation token" payload. See [Brave Rewards user confirmation token payload](../confirmations/reward/README.md).

| key  | example  | description  |
|---|---|---|
| buildChannel  | {"buildChannel": "beta"}  | Browser build channel.  |
| catalog  | {"catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}]}  |  Refers to the catalog that the ad was pulled from.  |
| conversion.action[^2]  | {"conversion":[{"action":"view"}]}  | View-through or click-through conversion attribution to determine which advertising efforts led a user to view or click on an ad and subsequently complete a desired action. Also, see [reporting metrics](https://ads-help.brave.com/campaign-performance/reporting/#available-reporting-metrics-in-brave-ads-manager).  |
| conversion.envelope[^2]  | {"conversion":[{"envelope":<br>{"alg":"crypto_box_curve25519xsalsa20poly1305",<br>"ciphertext":"3f8a8aeb5e1e4b1a8e7b4e3f8a8aeb5e1e4b1a8e7b4e3f8a8aeb5e1e4b1a8e7b",<br>"epk":"d1f2e3d4c5b6a79887a6b5c4d3e2f1d1f2e3d4c5b6a79887a6",<br>"nonce":"a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6"}}]}  | Allow advertisers to affirm that conversions are legitimate by wrapping an identifier with an additional layer of encryption to protect its integrity. Also, see https://ads-help.brave.com/campaign-performance/reporting#verifiable-ad-conversions-vac.  |
| createdAtTimestamp  | {"createdAtTimestamp":"2020-11-18T12:00:00.000Z"}  | [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) timestamp with fixed values for minutes, seconds, milliseconds and timezone.  |
| diagnosticId[^1][^2]  | {"diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2"}  | Diagnostic id from [brave://rewards-internals](brave://rewards-internals) to assist in troubleshooting issues. This is only included if it has been set manually by the user, for example, at the request of Brave Support in order to troubleshoot a problem.  |
| httpResponseStatus  | {"httpResponseStatus":"404"}  | Indicates the landing page's HTTP status code.  |
| platform  | {"platform":"windows"}  | Operating system.  |
| rotatingHash[^2]  | {"rotatingHash":"j9D7eKSoPLYNfxkG2Mx+SbgKJ9hcKg1QwDB8B5qxlpk="}  | Hashed device identifier that is unique to each ad and rotated several times a day. This is used for rate-limiting purposes.  |
| segment[^2]  | {"segment":"sports"}  | Advertising taxonomy for the chosen ad. Also see, https://ads-help.brave.com/campaign-performance/targeting.  |
| studies  | {"studies":[{"group":"GroupA","name":"BraveAds.FooStudy"},{"group":"GroupB","name":"BraveAds.BarStudy"}]}  | User studies, utilized for A/B testing, should not exceed one active study at any time. Configuration of studies is achievable through Griffin by adding "BraveAds." as a prefix to the experiment name. See [Griffin](https://github.com/brave/brave-browser/wiki/Brave-Variations-(Griffin)).  |
| systemTimestamp[^1]  | {"systemTimestamp": "2020-11-18T12:00:00.000Z"}  | [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) timestamp with fixed values for minutes, seconds, milliseconds and timezone.  |
| versionNumber  | {"versionNumber":"1.2.3.4"}  | Browser version number.  |

### User Data Submitted when Redeeming Identifiable Payment Tokens

Included as part of the "payment token" payload. See [Brave Rewards user payment token redemption](../utility/redeem_payment_tokens/README.md).

| key  | example  | description  |
|---|---|---|
| platform  | {"platform":"macos"}  | Operating system.  |
| summary  | {"totals":[{"ad_format":"ad_notification","click":1,"view":2},{"ad_format":"inline_content_ad","view":1}]}  | Summary of payment tokens for each creative.  |

Please add to it!

[^1]: will change if a confirmation redemption attempt fails and is subsequently retried.
[^2]: optional user data might not be included in the confirmation redemption payload.
