# Ad Units

An advertisement, or ad unit, is a form of marketing communication to promote a product, service, or brand. The primary purpose of an ad is to persuade the target audience to take some action, such as purchasing a product, visiting a website, or learning more about a service.

| unit  | served  | when  | description  |
|---|---|---|---|
| inline content ad  | yes¹  | Only when opted into Brave News  | Displayed on the Brave News feed  |
| new tab page ad  | yes¹ ²  | Always, but the frequency is capped when the user has joined Brave Rewards.  | Displayed on new tab pages  |
| notification ad  | yes²  | Only when opted into notification ads  | Displayed as system or custom push notifications  |
| promoted content ad  | no² ³  | Only when opted into Brave News  | Displayed on the Brave News feed  |
| search result ad  | no² ³  | Always, but the frequency is capped when the user has joined Brave Rewards.  | Displayed on [search.brave.com](search.brave.com).  |

Users are rewarded for viewed ad impressions if they join Brave Rewards.

1. `kServedImpression` events should not be manually triggered as they are handled when calling `MaybeServe`. `PurgeOrphanedAdEventsForType` should be called periodically to remove orphaned served impression events, i.e., ads that were served but never viewed. Orphaned served impression ad events are removed during initializing.
2. `kServedImpression` events should not be manually triggered as they are handled when calling `TriggerEvent` with `kViewedImpression` unless already triggerd by ¹.
3. Only responsible for triggering ad events and not serving the ad.
