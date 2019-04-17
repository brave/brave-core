# BAT Native Ads

## Resources

The `resources/` directory contains the following structure:

```
locales/
├── de/
│   ├── user_model.json
├── en/
│   ├── user_model.json
├── fr/
│   └── user_model.json
catalog-schema.json
bundle-schema.json
```

`user_model.json` see https://github.com/brave-intl/bat-native-usermodel/blob/master/README.md

`catalog-schema.json` and `bundle-schema.json` are JSON Schemas which specify the JSON-based format to define the structure of the JSON data for validation, documentation, and interaction control. It provides the contract for the JSON data and how that data can be modified.

## API

### Native

Initialize Ads by calling `Initialize` when Ads are enabled or disabled on the Client as follows:

```
void Initialize()
```

`IsSupportedRegion` should be called to determine if Ads are supported for the specified region
```
static bool IsSupportedRegion(
    const std::string& locale)
```

`OnForeground` should be called when the browser enters the foreground
```
void OnForeground()
```

`OnBackground` should be called when the browser enters the background
```
void OnBackground()
```

`OnIdle` should be called periodically on desktop browsers as set by `SetIdleThreshold` to record when the browser is idle. This call is optional for mobile devices
```
void OnIdle()
```

`OnUnidle` should be called periodically on desktop browsers as set by `SetIdleThreshold` to record when the browser is no longer idle. This call is optional for mobile devices
```
void OnUnIdle()
```

`OnMediaPlaying` should be called to record when a tab has started playing media (A/V)
```
void OnMediaPlaying(
    const int32_t tab_id)
```

`OnMediaStopped` should be called to record when a tab has stopped playing media (A/V)
```
void OnMediaStopped(
    const int32_t tab_id)
```

`TabUpdated` should be called to record user activity on a browser tab
```
void TabUpdated(
    const int32_t tab_id,
    const std::string& url,
    const bool is_active,
    const bool is_incognito)
```

`TabClosed` should be called to record when a browser tab is closed
```
void TabClosed(
    const int32_t tab_id)
```

`RemoveAllHistory` should be called to remove all cached history when the user clears browsing data
```
void RemoveAllHistory()
```

`SetConfirmationsIsReady` should be called to inform Ads if Confirmations is ready
```
void SetConfirmationsIsReady(
    const bool is_ready)
```

`ChangeLocale` should be called when the user changes the operating system's locale, i.e. `en`, `en_US` or `en_GB.UTF-8`. This call is not required if the operating system restarts the app when the user changes their locale
```
void ChangeLocale(
    const std::string& locale)
```

`ClassifyPage` should be called when a page has loaded in the current browser tab, and the HTML is available for analysis
```
void ClassifyPage(
    const std::string& url,
    const std::string& html)
```

`ServeSampleAd` should be called when the user invokes "Show Sample Ad" on the Client; a notification is then sent to the Client for processing
```
void ServeSampleAd()
```

`OnTimer` should be called when a timer is triggered
```
void OnTimer(
    const uint32_t timer_id)
```

`GenerateAdReportingNotificationShownEvent` should be called when a notification has been shown
```
void GenerateAdReportingNotificationShownEvent(
    const NotificationInfo& info)
```

`GenerateAdReportingNotificationResultEvent` should be called when a notification has been clicked, dismissed or times out on the Client. Dismiss events for local Notifications may not be available for every version of Android, making the Dismiss notification capture optional for Android devices
```
void GenerateAdReportingNotificationResultEvent(
    const NotificationInfo& info,
    const NotificationResultInfoResultType type)
```

### Client

`IsAdsEnabled` should return `true` if Ads are enabled otherwise returns `false`
```
bool IsAdsEnabled() const
```

`GetAdsLocale` should return the operating system's locale, i.e. `en`, `en_US` or `en_GB.UTF-8`
```
std::string GetAdsLocale() const
```

`GetAdsPerHour` should return the number of Ads that can be shown per hour
```
uint64_t GetAdsPerHour() const
```

`GetAdsPerDay` should return the number of Ads that can be shown per day
```
uint64_t GetAdsPerDay() const
```

`SetIdleThreshold` should set the idle threshold specified in seconds, for how often `OnIdle` or `OnUndle` should be called
```
void SetIdleThreshold(
    const int threshold)
```

`IsNetworkConnectionAvailable` should return `true` if there is a network connection otherwise returns `false`
```
bool IsNetworkConnectionAvailable()
```

`GetClientInfo` should get information about the client
```
void GetClientInfo(
    ClientInfo* info) const
```

`GetLocales` should return a list of supported User Model locales, see [resources](#resources)
```
const std::vector<std::string> GetLocales() const
```

`LoadUserModelForLocale` should load the User Model for the specified locale, see [resources](#resources)
```
void LoadUserModelForLocale(
    const std::string& locale,
    OnLoadCallback callback) const
```

`GenerateUUID` should generate and return a v4 UUID
```
const std::string GenerateUUID() const
```

`IsForeground` should return `true` if the browser is in the foreground otherwise returns `false`
```
bool IsForeground() const
```

`IsNotificationsAvailable` should return `true` if the operating system supports notifications otherwise returns `false`
```
bool IsNotificationsAvailable() const
```

`ShowNotification` should show a notification
```
void ShowNotification(std::unique_ptr<NotificationInfo> info)
```

`SetCatalogIssuers` should notify that the catalog issuers have changed
```
  void SetCatalogIssuers(std::unique_ptr<IssuersInfo> info)
```

`ConfirmAd` should be called to inform Confirmations that an Ad was clicked, viewed, dismissed or landed
```
void ConfirmAd(std::unique_ptr<NotificationInfo> info)
```

`SetTimer` should create a timer to trigger after the time offset specified in seconds. If the timer was created successfully a unique identifier should be returned, otherwise returns `0`
```
uint32_t SetTimer(const uint64_t time_offset)
```

`KillTimer` should destroy the timer associated with the specified timer identifier
```
void KillTimer(uint32_t timer_id)
```

`URLRequest` should start a URL request
```
void URLRequest(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const URLRequestMethod method,
    URLRequestCallback callback)
```

`Save` should save a value to persistent storage
```
void Save(
    const std::string& name,
    const std::string& value,
    OnSaveCallback callback)
```

`SaveBundleState` should save the bundle state to persistent storage
```
void SaveBundleState(
    std::unique_ptr<BundleState> state,
    OnSaveCallback callback)
```

`Load` should load a value from persistent storage
```
void Load(const std::string& name, OnLoadCallback callback)
```

`LoadJsonSchema` should load a JSON schema from persistent storage, see [resources](#resources)
```
const std::string LoadJsonSchema(const std::string& name)
```

`LoadSampleBundle` should load the sample bundle from persistent storage
```
void LoadSampleBundle(OnLoadSampleBundleCallback callback)
```

`Reset` should reset a previously saved value, i.e. remove the file from persistent storage
```
void Reset(const std::string& name, OnResetCallback callback)
```

`GetAds` should get Ads for the specified category from the previously persisted bundle state
```
void GetAds(
    const std::string& category,
    OnGetAdsCallback callback)
```

`EventLog` should log an event to persistent storage
```
void EventLog(const std::string& json)
```

`Log` should log diagnostic information to the console
```
std::unique_ptr<LogStream> Log(
    const char* file,
    const int line,
    const LogLevel log_level) const
```

## Command-line Switches

Use production Ads Serve as defined by `PRODUCTION_SERVER` in `static_values.h`. Default for Official Builds

```
--brave-ads-production
```

Use staging Ads Serve as defined by `STAGING_SERVER` in `static_values.h`. Default for non Office Builds, i.e. Debug

```
--brave-ads-staging
```

Collect initial activity after 25 seconds instead of 1 hour as defined by `kDebugOneHourInSeconds` in `static_values.h`

```
--brave-ads-debug
```

Enable testing of notifications while viewing `www.iab.com` so that page refreshes force a notification to show after 30 seconds as defined by `kNextEasterEggStartsInSeconds` in `static_values.h`

```
--brave-ads-testing
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

./Brave\ Browser\ Dev --brave-ads-staging --enable-logging=stderr --log-level=2
```

## Unit Tests
```
npm run test -- brave_unit_tests --filter=Ads*
```
