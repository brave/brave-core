# Brave Private Ads

Users earn tokens by viewing Brave Private Ads. Ads presented are based on the user's interests, as inferred from the user's browsing behavior. No personal data or browsing history should ever leave the browser.

## API

Public facing documentation can be found in `components/brave_ads/browser/ads_service.h` and [ads_client.h](include/bat/ads/ads_client.h).

## Command Line Switches

| switch  | explanation  |
|---|---|
| rewards  | Multiple options can be comma separated (no spaces). Note: all options are in the format `foo=x`. Values are case-insensitive. Options are `staging`, which forces ads to use the staging environment if set to `true` or the production environment if set to `false`. `debug`, which reduces the delay before downloading the catalog, fetching subdivision-targeting codes, paying out confirmation tokens, and submitting conversions if set to `true`. e.g. `--rewards=staging=true,debug=true`.  |
| vmodule  | Gives the per-module maximal V-logging levels to override the value given by `--v`. e.g. `*/bat-native-ads/*"=6,"*/brave_ads/*"=6,"*/bat_ads/*"=6` would change the logging level for Brave Private Ads to 6.  |

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
npm run test -- brave_unit_tests --filter=BatAds*
```
