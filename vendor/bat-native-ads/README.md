# Brave Private Ads

## API

Public facing API documentation can be found in `ads_service.h`, `ads.h` and
`ads_client.h`

## Command-line Switches

Use production ads server. Default for official builds, i.e. Release

```
--rewards=staging=false
```

Use staging ads server. Default for non official builds, i.e. Debug

```
--rewards=staging=true
```

## Unit Tests

```
npm run test -- brave_unit_tests --filter=BatAds*
```
