# Brave Private Ads

Users earn tokens by viewing Brave Private Ads. Ads presented are based on the user's interests, as inferred from the user's browsing behavior. No personal data or browsing history should ever leave the browser.

## API

Documentation can be found in `components/brave_ads/browser/ads_service.h` and [ads_client.h](include/bat/ads/ads_client.h).

## Command Line Switches

| condition |explanation|
|-----------|-----------|
| --rewards | Multiple options can be comma separated (no spaces). Note: all options are in the format 'foo=x'. Values are case-insensitive. Options are 'staging', which forces ads to use the staging environment if set to 'true' or the production environment if set to 'false'. 'debug', which reduces the delay before downloading the catalog, fetching subdivision targets, redeem unblinded payment tokens and submitting conversions. |

## Browser Tests
```
npm run test -- brave_browser_tests --filter=BraveAds*
```

## Unit Tests
```
npm run test -- brave_unit_tests --filter=BatAds*
```
