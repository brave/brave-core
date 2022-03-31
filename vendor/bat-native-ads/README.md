# BAT Ads

## API

Public facing API documentation can be found in `ads.h` and `ads_client.h`

## Command-line Switches

Use production Ads server. Default for official builds, i.e. Release

```
--brave-ads-production
```

Use staging Ads server. Default for non official builds, i.e. Debug

```
--brave-ads-staging
```

Collect initial activity after 25 seconds instead of 1 hour as defined by
`kDebugOneHourInSeconds` in `static_values.h`. Download the catalog after 15
minutes instead of 2 hours as defined by `kDebugCatalogPing` in
`static_values.h`

```
--brave-ads-debug
```

## Unit Tests

```
npm run test -- brave_unit_tests --filter=BatAds*
```
