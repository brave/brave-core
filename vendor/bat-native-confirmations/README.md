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

./Brave\ Browser\ Dev --rewards=staging=false --enable-logging=stderr --log-level=2
```

## Unit Tests
```
npm run test -- brave_unit_tests --filter=Confirmations*
```
