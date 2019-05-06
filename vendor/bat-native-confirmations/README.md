# BAT Native Confirmations

## API

### Native

Initialize Confirmations by calling Initialize() as follows:

```
Initialize()
```

`SetWalletInfo` should be called by Ledger to set the wallet info
```
void SetWalletInfo(
    const bool is_ready)
```

`SetCatalogIssuers` should be called by Ads to set the catalog issuers
```
void SetCatalogIssuers(
    std::unique_ptr<IssuersInfo> info)
```

`GetTransactionHistoryForThisCycle` should be called to get transaction history for this cycle
```
void GetTransactionHistory(
    OnGetTransactionHistoryForThisCycle callback)
```

`ConfirmAd` should be called by Ads when clicking, viewing, dismissing or landing an Ad
```
void ConfirmAd(
    std::unique_ptr<NotificationInfo> info)
```

`OnTimer` should be called when a timer is triggered
```
void OnTimer(
    const uint32_t timer_id)
```

### Client

`SetConfirmationsIsReady` should notify Ads if Confirmations is ready
```
void SetConfirmationsIsReady(
    const bool is_ready)
```

`ConfirmationsTransactionHistoryDidChange` should notify Ledger if transaction if the transaction history has changed
```
void ConfirmationsTransactionHistoryDidChange()
```

`SetTimer` should create a timer to trigger after the time offset specified in seconds. If the timer was created successfully a unique identifier should be returned, otherwise returns `0`
```
uint32_t SetTimer(
    uint64_t time_offset,
    uint32_t* timer_id)
```

`KillTimer` should destroy the timer associated with the specified timer identifier
```
void KillTimer(
    const uint32_t timer_id)
```

`LoadURL` should start a URL request
```
void LoadURL(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const URLRequestMethod method,
    URLRequestCallback callback)
```

`SaveState` should save a value to persistent storage
```
void SaveState(
    const std::string& name,
    const std::string& value,
    OnSaveCallback callback)
```

`LoadState` should load a value from persistent storage
```
void LoadState(
    const std::string& name,
    OnLoadCallback callback)
```

`ResetState` should reset a previously saved value, i.e. remove the file from persistent storage
```
void Reset(
    const std::string& name,
    OnResetCallback callback)
```

`Log` should log diagnostic information to the console
```
std::unique_ptr<LogStream> Log(
    const char* file,
    const int line,
    const LogLevel log_level) const
```

`VerboseLog` should log verbose diagnostic information to the console
```
std::unique_ptr<LogStream> Log(
    const char* file,
    const int line,
    const int log_level) const
```

## Command-line Switches

Use production Ads Serve as defined by `PRODUCTION_SERVER` in `static_values.h`. Default for Official Builds

```
--rewards=staging=false
```

Use staging Ads Serve as defined by `STAGING_SERVER` in `static_values.h`. Default for non Office Builds, i.e. Debug

```
--rewards=staging=true
```

Use shorter timers to help with testing token redemption as defined by `kDebugNextTokenRedemptionAfterSeconds` in `static_values.h`.

```
--rewards=debug=true
```

Enable diagnostic logging, where `#` should set to a minimum log level. Valid values are from 0 to 3 where INFO = 0, WARNING = 1, ERROR = 2 and FATAL = 3. So if you want INFO, WARNING and ERROR you would choose 2

```
--enable-logging=stderr --log-level=#
```

i.e. to launch using Staging Server and logging for INFO, WARNING and ERROR on macOS open the `Terminal` application and enter the below commands:

```
cd /Applications

cd Brave\ Browser\ Dev.app/

cd Contents

./Brave\ Browser\ Dev --rewards=staging=false --enable-logging=stderr --log-level=2
```

## Unit Tests
```
npm run test -- brave_unit_tests --filter=Confirmations*
```
