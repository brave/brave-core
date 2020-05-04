# BAT Confirmations

## API

Public facing API documentation can be found in `confirmations.h` and
`confirmations_client.h`

## Command-line Switches

Use production Ads Server as defined by `PRODUCTION_SERVER` in
`static_values.h`. Default for official builds, i.e. Release

```
--rewards=staging=false
```

Use staging Ads Server as defined by `STAGING_SERVER` in `static_values.h`.
Default for non offical builds, i.e. Debug

```
--rewards=staging=true
```

Use development Ads Server as defined by `DEVELOPMENT_SERVER` in `static_values.h`.

```
--rewards=development=true
```

Use shorter timers to help with testing token redemption as defined by
`kDebugNextTokenRedemptionAfterSeconds` in `static_values.h`.

```
--rewards=debug=true
```

## Unit Tests
```
npm run test -- brave_unit_tests --filter=BatConfirmations*
```
