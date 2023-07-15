# Ads

An advertisement, or ad, is a form of marketing communication that is used to promote a product, service, or brand. The primary purpose of an ad is to persuade the target audience to take some action, such as purchasing a product, visiting a website, or learning more about a service.

| format  | served  | when  | description  |
|---|---|---|---|
| inline content ad  | yes¹  | Only when opted-in to Brave News  | Displayed on the Brave News feed  |
| new tab page ad  | yes¹ ²  | Always, but the frequency is capped when opted-in to Brave Private Ads.  | Displayed on new tab pages   |
| notification ad  | yes²  | Only when opted-in to Brave Private Ads  | Displayed as system or custom push notifications  |
| promoted content ad  | no² ³  | Only when opted-in to Brave News  | Displayed on the Brave News feed  |
| search result ad  | no² ³  | Always, but the frequency is capped when opted-in to Brave Private Ads.  | Displayed on [search.brave.com](search.brave.com)  |

Users are rewarded for ad impressions if they are opted-in to Brave Private Ads on brave://rewards.

1. `kServed` events should not be manually triggered as they are handled when calling `MaybeServe`. `PurgeOrphanedAdEventsForType` should be called periodically to remove orphaned served events, i.e., ads that were served but never viewed.
2. `kServed` events should not be manually triggered as they are handled when calling `TriggerEvent` with `kViewed` unless already triggerd by ¹.
3. Only responsible for triggering ad events and not serving the ad.
