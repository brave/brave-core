# User Data

Return to [confirmation redemption](../../account/utility/redeem_confirmation/README.md).

> [!IMPORTANT]
> Approval from the privacy team is required for changes to existing user data or for new user data.

## Non-Brave Rewards User

### User Data Submitted when Redeeming Anonymous Confirmations

Included as part of the "confirmation" payload. See [non-Brave Rewards user confirmation payload](../confirmations/non_reward/README.md).

| key  | example  | description  |
|---|---|---|
| buildChannel  | {"buildChannel": "beta"}  | Browser build channel.  |
| conversion.action[^2]  | {"conversion":[{"action":"click"}]}  | Conversion attribution to determine which advertising efforts led a user to view or click on an ad and subsequently complete a desired action. Also, see [reporting metrics](https://ads-help.brave.com/campaign-performance/reporting/#available-reporting-metrics-in-brave-ads-manager).<br><br> Supported types:<br><br>Brave Search ads:<br>- click-through  |
| countryCode  | {"countryCode":"US"}  | Two-letter [ISO 3166-1 alpha-2](https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2) country code representing the user's current country. Uses the variations country code, which is determined by IP geolocation, when available; otherwise, falls back to the device's country code.  |
| platform  | {"platform":"windows"}  | Operating system.  |
| versionNumber  | {"versionNumber":"1.2.0.0"}  | Browser version number with fixed values for build and patch.  |

## Brave Rewards User

### User Data Submitted when Redeeming Anonymous Confirmation Tokens

Included as part of the "confirmation token" payload. See [Brave Rewards user confirmation token payload](../confirmations/reward/README.md).

| key  | example  | description  |
|---|---|---|
| buildChannel  | {"buildChannel": "beta"}  | Browser build channel.  |
| catalog  | {"catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}]}  |  Refers to the catalog that the ad was pulled from.  |
| conversion.action[^2]  | {"conversion":[{"action":"view"}]}  | View-through or click-through conversion attribution to determine which advertising efforts led a user to view or click on an ad and subsequently complete a desired action. Also, see [reporting metrics](https://ads-help.brave.com/campaign-performance/reporting/#available-reporting-metrics-in-brave-ads-manager).  |
| countryCode  | {"countryCode":"US"}  | Two-letter [ISO 3166-1 alpha-2](https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2) country code representing the user's current country. Uses the variations country code, which is determined by IP geolocation, when available; otherwise, falls back to the device's country code.  |
| createdAtTimestamp  | {"createdAtTimestamp":"2020-11-18T12:00:00.000Z"}  | Transaction creation time as an [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) UTC timestamp, rounded down to the hour.  |
| diagnosticId[^1][^2]  | {"diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2"}  | Diagnostic id from `brave://ads-internals` to assist in troubleshooting issues. This is only included if it has been set manually by the user, for example, at the request of Brave Support in order to troubleshoot a problem.  |
| httpResponseStatus  | {"httpResponseStatus":"404"}  | HTTP status of the landing page, included with landed confirmations. A limited set of status codes is reported exactly; other valid codes are reduced to their status class (for example, `5xx`).  |
| platform  | {"platform":"windows"}  | Operating system.  |
| rotatingHash[^2]  | {"rotatingHash":"j9D7eKSoPLYNfxkG2Mx+SbgKJ9hcKg1QwDB8B5qxlpk="}  | Hashed device identifier combined with the ad's creative instance ID and current hour, producing a per-ad identifier that rotates hourly. This is used for rate-limiting purposes.  |
| segment[^2]  | {"segment":"sports"}  | Advertising taxonomy for the chosen ad. Also see, https://ads-help.brave.com/campaign-performance/targeting.  |
| studies  | {"studies":[{"group":"GroupA","name":"BraveAds.FooStudy"}]}  | User studies used for A/B testing should not exceed one active study at a time. The payload includes a study only when exactly one `BraveAds.` study is active; otherwise, the list is empty. Configure studies through Griffin by adding the `BraveAds.` prefix to the experiment name. See [Griffin](https://github.com/brave/brave-browser/wiki/Brave-Variations-(Griffin)).  |
| systemTimestamp[^1]  | {"systemTimestamp": "2020-11-18T12:00:00.000Z"}  | Current time as an [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) UTC timestamp, rounded down to the hour.  |
| versionNumber  | {"versionNumber":"1.2.0.0"}  | Browser version number with fixed values for build and patch.  |

### User Data Submitted when Redeeming Identifiable Payment Tokens

Included as part of the "payment token" payload. See [Brave Rewards user payment token redemption](../utility/redeem_payment_tokens/README.md).

| key  | example  | description  |
|---|---|---|
| platform  | {"platform":"macos"}  | Operating system.  |
| summary  | {"totals":[{"ad_format":"ad_notification","click":1,"view":2},{"ad_format":"new_tab_page_ad","view":1}]}  | Summary of payment tokens for each creative.  |


## Retry Behavior

Fixed user data remains unchanged when a failed confirmation redemption is
retried. Dynamic user data can change between attempts; fields marked with [^1]
are dynamic.

[^1]: will change if a confirmation redemption attempt fails and is subsequently retried.
[^2]: optional user data might not be included in the confirmation redemption payload.

Please add to it!
