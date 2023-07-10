# Brave Ads

Users earn tokens by viewing privacy-first Brave Ads. Ads presented are based on the user's interests, as inferred from the user's browsing behavior. No personal data or browsing history should ever leave the browser.

## Glossary of Terms

| term  | description  |
|---|---|
| Ad event  | Ad events are actions or interactions with an advertisement, such as a user viewing or clicking an ad.  |
| Advertiser  | An advertiser is someone or a company that promotes products, services, or ideas through various mediums to reach and persuade potential customers or target audiences.  |
| Anti-targeting  | Exclude users from receiving targeted advertisements based on specific attributes such as demographics, interests, or behaviors.  |
| Basic Attention Token  | See the [whitepaper](https://basicattentiontoken.org/static-assets/documents/BasicAttentionTokenWhitePaper-4.pdf). Also known as BAT.  |
| Behavioral  | Behavioral advertising targets ads based on the user's previous online behavior, such as browsing history and interactions, to deliver relevant and personalized advertisements.  |
| Brave Rewards User  | Refers to an individual who has joined Brave Rewards and will be rewarded BAT for viewing ads.  |
| Campaign  | A campaign is a coordinated series of marketing activities and messages designed to achieve specific goals within a defined timeframe.  |
| Catalog  | A collection of available campaigns, creative sets and creative instances.  |
| Click-through rate  | The percentage of ad impressions that result in clicks.  |
| Confirmations  | See [security and privacy model for ad confirmations](https://github.com/brave/brave-browser/wiki/Security-and-privacy-model-for-ad-confirmations).  |
| Contextual  | Contextual advertising targets ads based on the web page's content or the user's online activity context to deliver relevant and personalized advertisements.  |
| Conversion  | When a user triggers an action, it is counted as a conversion. Conversions include making a purchase or signing up for a newsletter.  |
| Creative instance  | A creative instance refers to a specific version or variation of an advertisement allowing advertisers to test different approaches, optimize performance, and effectively engage their target audience.  |
| Creative set  | A creative set refers to a collection of related creative instances or variations of an advertisement.  |
| Day parting  | Day parting is a strategy that involves scheduling and targeting ads based on specific times of the day to maximize their effectiveness.  |
| Delivery  | Delivering ad content to the user, also known as serving.  |
| Frequency capping  | Limits how many times a given ad will be shown within a specified period, also known as exclusion rules.  |
| Geographic  | Geographic advertising targets ads based on specific geographic locations to reach the desired audience.  |
| Impression  | An impression is when an ad is displayed or shown to a user, also known as a view.  |
| Multi-armed bandits  | Multi-armed bandits optimize the allocation of resources between different ad variations or strategies to maximize performance.  |
| Non-Brave Rewards User  | Refers to a user who has not joined Brave Rewards and will not be rewarded for viewing ads.  |
| Pacing  | Rate at which an ad campaign uses up its pre-set number of impressions.  |
| Placement  | The location at which an ad is displayed to the user. Also known as an ad unit.  |
| Priority  | Priority at which an ad campaign uses up its impressions.  |
| Purchase intent  | Likelihood or inclination of a consumer to make a purchase based on their expressed interest or behavior. It indicates the consumer's readiness to buy a product or service.  |
| Resource  | Resources provided by [components](brave://components).  |
| Segment  | An advertising taxonomy to target ads to reach the desired audience.  |
| Split testing  | Comparing ad or landing page versions to determine which performs better. Also known as A/B testing.  |
| Targeting  | Delivering an ad to the appropriate audience through behavioral, contextual, or geographic targeting.  |
| Text classification  | Target audiences based on the analysis and understanding of textual data to deliver more relevant and personalized advertisements.  |
| Text embedding  | Converts text-based information, such as user reviews or product descriptions, into numerical representations (embeddings), enabling more precise audience targeting based on the semantic meaning of the text.  |
| Transfer  | Delivery of an advertiser's website, i.e., the landing page after the user clicks on an ad, also known as landed.  |
| User attention  | Measure user attention to drive performance.  |

Please add to it!

## Command Line Switches

| switch  | explanation  |
|---|---|
| rewards  | Multiple options can be comma separated (no spaces). Note: all options are in the format `foo=x`. Values are case-insensitive. Options are `staging`, which forces ads to use the staging environment if set to `true` or the production environment if set to `false`. `debug`, which reduces the delay before downloading the catalog, fetching subdivisions, paying out confirmation tokens, and submitting conversions if set to `true`. e.g. `--rewards=staging=true,debug=true`.  |
| vmodule  | Options are `--vmodule` gives the per-module maximal V-logging levels to override the value given by `--v`, e.g., `"*/brave_ads/*"=6,"*/bat_ads/*"=6` would change the logging level for Brave Ads to 6.  |
| use-dev-goupdater-url  | Options are `--use-dev-goupdater-url`, which forces the component updater to use the dev environment.  |

## Logs

You can enable diagnostic logging to the `Rewards.log` file stored on your device; see [Brave Rewards](brave://flags/#brave-rewards-verbose-logging). View this log file on the `Logs` tab at [rewards internals](brave://rewards-internals).

## Diagnostics

View diagnostics at [rewards internals](brave://rewards-internals) on the `Ad diagnostics` tab.

## Browser Tests

```
npm run test -- brave_browser_tests --filter=BraveAds*
```

## Unit Tests

```
npm run test -- brave_unit_tests --filter=BraveAds*
```
