# BAT Ads

## API

Public facing API documentation can be found in `ads.h` and `ads_client.h`

## Command-line Switches

Use production Ads Serve as defined by `PRODUCTION_SERVER` in `static_values.h`. Default for official builds, i.e. Release

```
--brave-ads-production
```

Use staging Ads Serve as defined by `STAGING_SERVER` in `static_values.h`.
Default for non official builds, i.e. Debug

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

Enable testing of notifications while viewing `www.iab.com` so that page
refreshes force a notification to be displayed after 30 seconds as defined by
`kNextEasterEggStartsInSeconds` in `static_values.h`

```
--brave-ads-testing
```

Enable diagnostic logging, where `#` should set to a minimum log level. Valid
values are from 0 to 3 where INFO = 0, WARNING = 1, ERROR = 2 and FATAL = 3. So
if you want INFO, WARNING and ERROR you would choose 2

```
--enable-logging=stderr --log-level=#
```

i.e. to launch using Staging Server and logging for INFO, WARNING and ERROR on
macOS open the `Terminal` application and enter the following commands:

```
cd /Applications/Brave\ Browser\ Dev.app/Contents

./Brave\ Browser\ Dev --brave-ads-staging --enable-logging=stderr --log-level=2
```

## Unit Tests

```
npm run test -- brave_unit_tests --filter=Ads*
```
