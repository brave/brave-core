/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>

#import "BATBraveAds.h"
#import "BATAdNotification.h"
#import "BATBraveLedger.h"

#import "bat/ads/ads.h"
#import "bat/ads/database.h"

#import "NativeAdsClient.h"
#import "NativeAdsClientBridge.h"
#import "CppTransformations.h"
#import "BATCommonOperations.h"

#import <Network/Network.h>
#import <UIKit/UIKit.h>

#import "base/strings/sys_string_conversions.h"
#import "brave/base/containers/utils.h"

#import "RewardsLogging.h"

#define BATClassAdsBridge(__type, __objc_getter, __objc_setter, __cpp_var) \
  + (__type)__objc_getter { return ads::__cpp_var; } \
  + (void)__objc_setter:(__type)newValue { ads::__cpp_var = newValue; }

static const NSInteger kDefaultNumberOfAdsPerDay = 20;
static const NSInteger kDefaultNumberOfAdsPerHour = 2;

static const int kCurrentUserModelManifestSchemaVersion = 1;

static NSString * const kAdsEnabledPrefKey = @"BATAdsEnabled";
static NSString * const kNumberOfAdsPerDayKey = @"BATNumberOfAdsPerDay";
static NSString * const kNumberOfAdsPerHourKey = @"BATNumberOfAdsPerHour";
static NSString * const kShouldAllowAdsSubdivisionTargetingPrefKey = @"BATShouldAllowAdsSubdivisionTargetingPrefKey";
static NSString * const kAdsSubdivisionTargetingCodePrefKey = @"BATAdsSubdivisionTargetingCodePrefKey";
static NSString * const kAutomaticallyDetectedAdsSubdivisionTargetingCodePrefKey = @"BATAutomaticallyDetectedAdsSubdivisionTargetingCodePrefKey";
static NSString * const kUserModelMetadataPrefKey = @"BATUserModelMetadata";

@interface BATAdNotification ()
- (instancetype)initWithNotificationInfo:(const ads::AdNotificationInfo&)info;
@end

@interface BATBraveAds () <NativeAdsClientBridge> {
  NativeAdsClient *adsClient;
  ads::Ads *ads;
  ads::Database *adsDatabase;

  nw_path_monitor_t networkMonitor;
  dispatch_queue_t monitorQueue;
}
@property (nonatomic) BATCommonOperations *commonOps;
@property (nonatomic) BOOL networkConnectivityAvailable;
@property (nonatomic, copy) NSString *storagePath;
@property (nonatomic) dispatch_queue_t prefsWriteThread;
@property (nonatomic) NSMutableDictionary *prefs;
@property (nonatomic) dispatch_queue_t databaseQueue;
@property (nonatomic, copy) NSDictionary *userModelMetadata;
@property (nonatomic) NSTimer *updateUserModelTimer;
@property (nonatomic) int64_t userModelRetryCount;
@property (nonatomic, readonly) NSDictionary *userModelPaths;
@end

@implementation BATBraveAds

- (instancetype)initWithStateStoragePath:(NSString *)path
{
  if ((self = [super init])) {
    self.storagePath = path;
    self.commonOps = [[BATCommonOperations alloc] initWithStoragePath:path];
    adsDatabase = nullptr;

    self.prefsWriteThread = dispatch_queue_create("com.rewards.ads.prefs", DISPATCH_QUEUE_SERIAL);
    self.prefs = [[NSMutableDictionary alloc] initWithContentsOfFile:[self prefsPath]];
    if (!self.prefs) {
      self.prefs = [[NSMutableDictionary alloc] init];
      self.numberOfAllowableAdsPerDay = kDefaultNumberOfAdsPerDay;
      self.numberOfAllowableAdsPerHour = kDefaultNumberOfAdsPerHour;
    }

    [self setupNetworkMonitoring];

    if (self.userModelMetadata == nil) {
      self.userModelMetadata = [[NSDictionary alloc] init];
    }

    self.userModelRetryCount = 1;

    // Add notifications for standard app foreground/background
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationDidBecomeActive) name:UIApplicationDidBecomeActiveNotification object:nil];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationDidBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];
  }
  return self;
}

- (void)dealloc
{
  [self.updateUserModelTimer invalidate];
  self.updateUserModelTimer = nil;

  [NSNotificationCenter.defaultCenter removeObserver:self];
  if (networkMonitor) { nw_path_monitor_cancel(networkMonitor); }
  if (ads != nil) {
    delete ads;
    delete adsClient;
    ads = nil;
    adsClient = nil;
  }
}

- (NSString *)prefsPath
{
  return [self.storagePath stringByAppendingPathComponent:@"ads_pref.plist"];
}

#pragma mark - Global

+ (BOOL)isSupportedLocale:(NSString *)locale
{
  return ads::IsSupportedLocale(std::string(locale.UTF8String));
}

+ (BOOL)isNewlySupportedLocale:(NSString *)locale
{
  // TODO(khickinson): Add support for last schema version, however for the MVP
  // we can safely pass 0 as all locales are newly supported
  return ads::IsNewlySupportedLocale(std::string(locale.UTF8String), 0);
}

+ (BOOL)isCurrentLocaleSupported
{
  return [self isSupportedLocale:[[NSLocale preferredLanguages] firstObject]];
}

BATClassAdsBridge(BOOL, isDebug, setDebug, _is_debug)

+ (int)environment
{
  return static_cast<int>(ads::_environment);
}

+ (void)setEnvironment:(int)environment
{
  ads::_environment = static_cast<ads::Environment>(environment);
}

+ (BATBraveAdsBuildChannel *)buildChannel
{
  auto build_channel = [[BATBraveAdsBuildChannel alloc] init];
  build_channel.isRelease = ads::_build_channel.is_release;
  build_channel.name = [NSString stringWithUTF8String: ads::_build_channel.name.c_str()];

  return build_channel;
}

+ (void)setBuildChannel:(BATBraveAdsBuildChannel *)buildChannel
{
  ads::_build_channel.is_release = buildChannel.isRelease;
  ads::_build_channel.name = buildChannel.name.UTF8String;
}

#pragma mark - Initialization / Shutdown

- (void)initializeIfAdsEnabled
{
  if (![self isAdsServiceRunning] && self.enabled) {
    self.databaseQueue = dispatch_queue_create("com.rewards.ads.db-transactions", DISPATCH_QUEUE_SERIAL);
    
    const auto* dbPath = [self adsDatabasePath].UTF8String;
    adsDatabase = new ads::Database(base::FilePath(dbPath));

    adsClient = new NativeAdsClient(self);
    ads = ads::Ads::CreateInstance(adsClient);
    ads->Initialize(^(bool) {
      [self periodicallyCheckForUserModelUpdates];

      NSString *localeIdentifier = [[NSLocale preferredLanguages] firstObject];
      NSLocale *locale = [NSLocale localeWithLocaleIdentifier:localeIdentifier];
      [self registerUserModels:locale];
    });
  }
}

- (NSString *)adsDatabasePath
{
  return [self.storagePath stringByAppendingPathComponent:@"Ads.db"];
}

- (void)resetAdsDatabase
{
  delete adsDatabase;
  const auto dbPath = [self adsDatabasePath];
  [NSFileManager.defaultManager removeItemAtPath:dbPath error:nil];
  [NSFileManager.defaultManager removeItemAtPath:[dbPath stringByAppendingString:@"-journal"] error:nil];
  adsDatabase = new ads::Database(base::FilePath(dbPath.UTF8String));
}

- (void)shutdown
{
  if ([self isAdsServiceRunning]) {
    ads->Shutdown(^(bool) {
      delete ads;
      delete adsClient;
      delete adsDatabase;
      ads = nil;
      adsClient = nil;
    });
  }
}

- (BOOL)isAdsServiceRunning
{
  return ads != nil;
}

#pragma mark - Configuration

- (BOOL)isEnabled
{
  return [self.prefs[kAdsEnabledPrefKey] boolValue];
}

- (void)setEnabled:(BOOL)enabled
{
  self.prefs[kAdsEnabledPrefKey] = @(enabled);
  [self savePrefs];
  if (enabled) {
    [self initializeIfAdsEnabled];
  } else {
    [self shutdown];
  }
}

- (BOOL)shouldAllowAdConversionTracking
{
  return true;
}

- (NSInteger)numberOfAllowableAdsPerDay
{
  return [self.prefs[kNumberOfAdsPerDayKey] integerValue];
}

- (void)setNumberOfAllowableAdsPerDay:(NSInteger)numberOfAllowableAdsPerDay
{
  self.prefs[kNumberOfAdsPerDayKey] = @(numberOfAllowableAdsPerDay);
  [self savePrefs];
}

- (NSInteger)numberOfAllowableAdsPerHour
{
  return [self.prefs[kNumberOfAdsPerHourKey] integerValue];
}

- (void)setNumberOfAllowableAdsPerHour:(NSInteger)numberOfAllowableAdsPerHour
{
  self.prefs[kNumberOfAdsPerHourKey] = @(numberOfAllowableAdsPerHour);
  [self savePrefs];
}

- (BOOL)shouldAllowSubdivisionTargeting
{
  return [self.prefs[kShouldAllowAdsSubdivisionTargetingPrefKey] boolValue];
}

- (void)setAllowSubdivisionTargeting:(BOOL)allowAdsSubdivisionTargeting
{
  self.prefs[kShouldAllowAdsSubdivisionTargetingPrefKey] = @(allowAdsSubdivisionTargeting);
  [self savePrefs];
}

- (NSString *)subdivisionTargetingCode
{
  return (NSString *)self.prefs[kAdsSubdivisionTargetingCodePrefKey] ?: @"AUTO";
}

- (void)setSubdivisionTargetingCode:(NSString *)subdivisionTargetingCode
{
  const NSString* lastSubdivisionTargetingCode = [self subdivisionTargetingCode];

  self.prefs[kAdsSubdivisionTargetingCodePrefKey] = subdivisionTargetingCode;
  [self savePrefs];

  if (lastSubdivisionTargetingCode == subdivisionTargetingCode) {
    return;
  }

  if (![self isAdsServiceRunning]) { return; }

  ads->OnAdsSubdivisionTargetingCodeHasChanged();
}

- (NSString *)automaticallyDetectedSubdivisionTargetingCode
{
  return (NSString *)self.prefs[kAutomaticallyDetectedAdsSubdivisionTargetingCodePrefKey] ?: @"";
}

- (void)setAutomaticallyDetectedSubdivisionTargetingCode:(NSString *)automaticallyDetectedSubdivisionTargetingCode
{
  self.prefs[kAutomaticallyDetectedAdsSubdivisionTargetingCodePrefKey] = automaticallyDetectedSubdivisionTargetingCode;
  [self savePrefs];
}

- (void)savePrefs
{
  dispatch_async(self.prefsWriteThread, ^{
    [self.prefs writeToFile:[self prefsPath] atomically:YES];
  });
}

#pragma mark -

- (void)setupNetworkMonitoring
{
  auto const __weak weakSelf = self;

  monitorQueue = dispatch_queue_create("bat.nw.monitor", DISPATCH_QUEUE_SERIAL);
  networkMonitor = nw_path_monitor_create();
  nw_path_monitor_set_queue(networkMonitor, monitorQueue);
  nw_path_monitor_set_update_handler(networkMonitor, ^(nw_path_t  _Nonnull path) {
    const auto strongSelf = weakSelf;
    if (!strongSelf) { return; }
    strongSelf.networkConnectivityAvailable = (nw_path_get_status(path) == nw_path_status_satisfied ||
                                               nw_path_get_status(path) == nw_path_status_satisfiable);
  });
  nw_path_monitor_start(networkMonitor);
}

- (void)removeAllHistory:(void (^)(BOOL))completion
{
  if (![self isAdsServiceRunning]) { return; }
  ads->RemoveAllHistory(completion);
}

#pragma mark - Observers

- (void)applicationDidBecomeActive
{
  if (![self isAdsServiceRunning]) { return; }
  ads->OnForeground();
}

- (void)applicationDidBackground
{
  if (![self isAdsServiceRunning]) { return; }
  ads->OnBackground();
}

#pragma mark - History

- (NSArray<NSDate *> *)getAdsHistoryDates
{
  if (![self isAdsServiceRunning]) { return @[]; }
  const uint64_t from_timestamp = 0;
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  const auto history = ads->GetAdsHistory(ads::AdsHistory::FilterType::kNone,
      ads::AdsHistory::SortType::kNone, from_timestamp, to_timestamp);

  const auto dates = [[NSMutableArray<NSDate *> alloc] init];
  for (const auto& entry : history.entries) {
    const auto date = [NSDate dateWithTimeIntervalSince1970:
        entry.timestamp_in_seconds];
    [dates addObject:date];
  }

  return dates;
}
- (BOOL)hasViewedAdsInPreviousCycle
{
  const auto calendar = [NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian];
  const auto now = NSDate.date;
  const auto previousCycleDate = [calendar dateByAddingUnit:NSCalendarUnitMonth value:-1 toDate:now options:0];
  const auto previousCycleMonth = [calendar component:NSCalendarUnitMonth fromDate:previousCycleDate];
  const auto previousCycleYear = [calendar component:NSCalendarUnitYear fromDate:previousCycleDate];
  const auto viewedDates = [self getAdsHistoryDates];
  for (NSDate *date in viewedDates) {
    const auto components = [calendar components:NSCalendarUnitMonth|NSCalendarUnitYear fromDate:date];
    if (components.month == previousCycleMonth && components.year == previousCycleYear) {
      // Was from previous cycle
      return YES;
    }
  }
  return NO;
}

#pragma mark - Reporting

- (void)reportLoadedPageWithURL:(NSURL *)url innerText:(NSString *)text tabId:(NSInteger)tabId
{
  if (![self isAdsServiceRunning]) { return; }
  const auto urlString = base::SysNSStringToUTF8(url.absoluteString);
  ads->OnPageLoaded((int32_t)tabId, urlString, urlString, base::SysNSStringToUTF8(text));
}

- (void)reportMediaStartedWithTabId:(NSInteger)tabId
{
  if (![self isAdsServiceRunning]) { return; }
  ads->OnMediaPlaying((int32_t)tabId);
}

- (void)reportMediaStoppedWithTabId:(NSInteger)tabId
{
  if (![self isAdsServiceRunning]) { return; }
  ads->OnMediaStopped((int32_t)tabId);
}

- (void)reportTabUpdated:(NSInteger)tabId url:(NSURL *)url isSelected:(BOOL)isSelected isPrivate:(BOOL)isPrivate
{
  if (![self isAdsServiceRunning]) { return; }
  const auto urlString = std::string(url.absoluteString.UTF8String);
  ads->OnTabUpdated((int32_t)tabId, urlString, isSelected, [self isForeground], isPrivate);
}

- (void)reportTabClosedWithTabId:(NSInteger)tabId
{
  if (![self isAdsServiceRunning]) { return; }
  ads->OnTabClosed((int32_t)tabId);
}

- (void)reportAdNotificationEvent:(NSString *)notificationUuid eventType:(BATAdNotificationEventType)eventType
{
  if (![self isAdsServiceRunning]) { return; }
  ads->OnAdNotificationEvent(notificationUuid.UTF8String,
                             static_cast<ads::AdNotificationEventType>(eventType));
}

- (void)updateAdRewards:(BOOL)shouldReconcile
{
  if (![self isAdsServiceRunning]) { return; }
  ads->UpdateAdRewards(shouldReconcile);
}

- (void)detailsForCurrentCycle:(void (^)(NSInteger adsReceived, double estimatedEarnings, NSDate *nextPaymentDate))completion
{
  ads->GetTransactionHistory(^(bool success, ads::StatementInfo list) {
    if (!success) {
      completion(0, 0, nil);
      return;
    }

    NSDate *nextPaymentDate = nil;
    if (list.next_payment_date_in_seconds > 0) {
      nextPaymentDate = [NSDate dateWithTimeIntervalSince1970:list.next_payment_date_in_seconds];
    }
    completion(list.ad_notifications_received_this_month,
               list.estimated_pending_rewards,
               nextPaymentDate);
  });
}

- (void)toggleThumbsUpForAd:(NSString *)creativeInstanceId creativeSetID:(NSString *)creativeSetID
{
  if (![self isAdsServiceRunning]) { return; }
  ads->ToggleAdThumbUp(creativeInstanceId.UTF8String,
                       creativeSetID.UTF8String,
                       ads::AdContent::LikeAction::kThumbsUp);
}

- (void)toggleThumbsDownForAd:(NSString *)creativeInstanceId creativeSetID:(NSString *)creativeSetID
{
  if (![self isAdsServiceRunning]) { return; }
  ads->ToggleAdThumbDown(creativeInstanceId.UTF8String,
                         creativeSetID.UTF8String,
                         ads::AdContent::LikeAction::kThumbsDown);
}

#pragma mark - Configuration

- (uint64_t)getAdsPerDay
{
  return self.numberOfAllowableAdsPerDay;
}

- (uint64_t)getAdsPerHour
{
  return self.numberOfAllowableAdsPerHour;
}

- (bool)isAdsEnabled
{
  return self.enabled;
}

- (bool)isForeground
{
  return UIApplication.sharedApplication.applicationState == UIApplicationStateActive;
}

- (bool)isNetworkConnectionAvailable
{
  return self.networkConnectivityAvailable;
}

- (void)setIdleThreshold:(const int)threshold
{
  // Not needed on mobile
}

#pragma mark - Network

- (void)UrlRequest:(ads::UrlRequestPtr)url_request  callback:(ads::UrlRequestCallback)callback {
  std::map<ads::UrlRequestMethod, std::string> methodMap {
    {ads::UrlRequestMethod::GET, "GET"},
    {ads::UrlRequestMethod::POST, "POST"},
    {ads::UrlRequestMethod::PUT, "PUT"}
  };

  const auto copiedURL = [NSString stringWithUTF8String:url_request->url.c_str()];

  return [self.commonOps loadURLRequest:url_request->url headers:url_request->headers content:url_request->content content_type:url_request->content_type method:methodMap[url_request->method] callback:^(const std::string& errorDescription, int statusCode, const std::string &response, const std::map<std::string, std::string> &headers) {
    ads::UrlResponse url_response;
    url_response.url = copiedURL.UTF8String;
    url_response.status_code = statusCode;
    url_response.body = response;
    url_response.headers = base::MapToFlatMap(headers);

    callback(url_response);
  }];
}

#pragma mark - File IO

- (NSDictionary *)userModelMetadata
{
  return (NSDictionary *)self.prefs[kUserModelMetadataPrefKey];
}

- (void)setUserModelMetadata:(NSDictionary *)userModelMetadata
{
  self.prefs[kUserModelMetadataPrefKey] = userModelMetadata;
  [self savePrefs];
}

- (BOOL)registerUserModelsForLanguageCode:(NSString *)languageCode
{
  NSString *isoLanguageCode = [@"iso_639_1_" stringByAppendingString:[languageCode lowercaseString]];

  NSArray *languageCodeUserModelIds = [self.userModelPaths allKeysForObject:isoLanguageCode];
  if ([languageCodeUserModelIds count] == 0) {
    return NO;
  }

  NSString *languageCodeUserModelId = [languageCodeUserModelIds firstObject];
  BLOG(1, @"Registering Brave User Model Installer (%@) with id %@", languageCode, languageCodeUserModelId);

  BLOG(1, @"Notifying user model observers");
  const std::string bridged_language_code_user_model_idkey = languageCodeUserModelId.UTF8String;
  ads->OnUserModelUpdated(bridged_language_code_user_model_idkey);

  return YES;
}

- (BOOL)registerUserModelsForCountryCode:(NSString *)countryCode
{
  NSString *isoCountryCode = [@"iso_3166_1_" stringByAppendingString:[countryCode lowercaseString]];

  NSArray *countryCodeUserModelIds = [self.userModelPaths allKeysForObject:isoCountryCode];
  if ([countryCodeUserModelIds count] == 0) {
    return NO;
  }

  NSString *countryCodeUserModelId = [countryCodeUserModelIds firstObject];
  BLOG(1, @"Registering Brave User Model Installer (%@) with id %@", countryCode, countryCodeUserModelId);

  BLOG(1, @"Notifying user model observers");
  const std::string bridged_country_code_user_model_idkey = countryCodeUserModelId.UTF8String;
  ads->OnUserModelUpdated(bridged_country_code_user_model_idkey);

  return YES;
}

- (void)registerUserModels:(NSLocale *)locale
{
  if (![self registerUserModelsForLanguageCode:locale.languageCode]) {
    BLOG(1, @"%@ not supported for user model installer", locale.languageCode);
  }

  if (![self registerUserModelsForCountryCode:locale.countryCode]) {
    BLOG(1, @"%@ not supported for user model installer", locale.countryCode);
  }
}

- (void)periodicallyCheckForUserModelUpdates
{
  const uint64_t time_offset = 6 * 60 * 60;  // every 6 hours and on browser launch

  const auto __weak weakSelf = self;
  self.updateUserModelTimer = [NSTimer scheduledTimerWithTimeInterval:time_offset repeats:YES block:^(NSTimer * _Nonnull timer) {
    const auto strongSelf = weakSelf;
    if (!strongSelf) { return; }

    [strongSelf updateUserModels];

    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    formatter.dateStyle = NSDateFormatterFullStyle;
    formatter.timeStyle = NSDateFormatterFullStyle;
    BLOG(1, @"Update user models on %@", [formatter stringFromDate:[[NSDate date] dateByAddingTimeInterval:time_offset]]);
  }];

  [self.updateUserModelTimer fire];
}

- (void)updateUserModels
{
  NSDictionary *dict = [self userModelMetadata];
  BLOG(1, @"Updating user models");

  for (NSString *key in dict) {
    BLOG(1, @"Checking %@ user model for updates", key);

    const auto __weak weakSelf = self;
    [self downloadUserModelForId:key completion:^(BOOL success, BOOL shouldRetry) {
      const auto strongSelf = weakSelf;
      if (!strongSelf) { return; }

      if (!success) {
        BLOG(1, @"Failed to update user models");
        return;
      }

      BLOG(1, @"Notifying user model observers");
      strongSelf->ads->OnUserModelUpdated(key.UTF8String);
    }];
  }
}

- (void)downloadUserModelForId:(NSString *)id completion:(void (^)(BOOL success, BOOL shouldRetry))completion
{
  BLOG(1, @"Downloading %@ user model manifest", id);

  const auto __weak weakSelf = self;

  NSString *baseUrl;
  if (ads::_environment == ads::Environment::PRODUCTION) {
    baseUrl = @"https://brave-user-model-installer-input.s3.brave.com";
  } else {
    baseUrl = @"https://brave-user-model-installer-input-dev.s3.bravesoftware.com";
  }

  NSString *userModelPath = self.userModelPaths[id] ?: @"";
  baseUrl = [baseUrl stringByAppendingPathComponent:userModelPath];

  NSString *manifestUrl = [baseUrl stringByAppendingPathComponent:@"models.json"];
  return [self.commonOps loadURLRequest:manifestUrl.UTF8String headers:{} content:"" content_type:"" method:"GET" callback:^(const std::string& errorDescription, int statusCode, const std::string &response, const std::map<std::string, std::string> &headers) {
    const auto strongSelf = weakSelf;
    if (!strongSelf) { return; }

    if (statusCode == 404) {
      BLOG(1, @"%@ user model manifest not found", id);
      completion(NO, NO);
      return;
    }

    if (statusCode != 200) {
      BLOG(1, @"Failed to download %@ user model manifest", id);
      completion(NO, YES);
      return;
    }

    NSData *data = [[NSString stringWithUTF8String:response.c_str()] dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary *dict = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    if (!dict) {
      completion(NO, YES);
      return;
    }

    NSNumber *schemaVersion = dict[@"schemaVersion"];
    if ([schemaVersion intValue] != kCurrentUserModelManifestSchemaVersion) {
      completion(NO, YES);
      return;
    }

    NSArray *models = dict[@"models"];

    for (NSDictionary *model in models) {
      NSString *modelId = model[@"id"];
      if (!modelId) {
        continue;
      }

      NSString *filename = model[@"filename"];
      if (!filename) {
        completion(NO, YES);
        continue;
      }

      NSNumber *version = model[@"version"];
      if (!version) {
        completion(NO, YES);
        continue;
      }

      NSDictionary *userModelMetadataDict = [strongSelf userModelMetadata];
      if (version <= userModelMetadataDict[modelId]) {
        BLOG(1, @"%@ user model is up to date on version %@", modelId, version);
        completion(YES, NO);
        return;
      }

      NSString *modelUrl = [baseUrl stringByAppendingPathComponent:filename];

      BLOG(1, @"Downloading %@ user model version %@", modelId, version);

      return [strongSelf.commonOps loadURLRequest:modelUrl.UTF8String headers:{} content:"" content_type:"" method:"GET" callback:^(const std::string& errorDescription, int statusCode, const std::string &response, const std::map<std::string, std::string> &headers) {
        const auto strongSelf = weakSelf;
        if (!strongSelf) { return; }

        if (statusCode == 404) {
          BLOG(1, @"%@ user model not found", id);
          completion(NO, NO);
          return;
        }

        if (statusCode != 200) {
          BLOG(1, @"Failed to download %@ user model version %@", modelId, version);
          completion(NO, YES);
          return;
        }

        [strongSelf.commonOps saveContents:response name:modelId.UTF8String];
        BLOG(1, @"Cached %@ user model version %@", modelId, version);

        NSMutableDictionary *dict = [[strongSelf userModelMetadata] mutableCopy];
        dict[modelId] = version;
        [strongSelf setUserModelMetadata:dict];

        BLOG(1, @"%@ user model updated to version %@", modelId, version);

        completion(YES, NO);
      }];
    }
  }];
}

- (void)loadUserModelForId:(const std::string &)id callback:(ads::LoadCallback)callback
{
  NSString *bridgedId = [NSString stringWithUTF8String:id.c_str()];

  BLOG(1, @"Loading %@ user model", bridgedId);

  const std::string contents = [self.commonOps loadContentsFromFileWithName:bridgedId.UTF8String];
  if (!contents.empty()) {
    BLOG(1, @"%@ user model is cached", bridgedId);
    callback(ads::Result::SUCCESS, contents);
    return;
  }

  BLOG(1, @"%@ user model not cached", bridgedId);

  const auto __weak weakSelf = self;
  [self downloadUserModelForId:bridgedId completion:^(BOOL success, BOOL shouldRetry) {
    const auto strongSelf = weakSelf;
    if (!strongSelf) { return; }

    const auto contents = [strongSelf.commonOps loadContentsFromFileWithName:bridgedId.UTF8String];
    if (!success || contents.empty()) {
      if (shouldRetry) {
        const int64_t backoff = 1 * 60;
        int64_t delay = backoff << strongSelf.userModelRetryCount;
        if (delay >= 60 * 60) {
          delay = 60 * 60;
        } else {
          strongSelf.userModelRetryCount++;
        }

        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        formatter.dateStyle = NSDateFormatterFullStyle;
        formatter.timeStyle = NSDateFormatterFullStyle;
        BLOG(1, @"Retry loading %@ user model on %@", bridgedId, [formatter stringFromDate:[[NSDate date] dateByAddingTimeInterval:delay]]);

        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, delay * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
          [strongSelf loadUserModelForId:bridgedId.UTF8String callback:callback];
        });
      }

      return;
    }

    strongSelf.userModelRetryCount = 1;

    callback(ads::Result::SUCCESS, contents);
  }];
}

- (void)load:(const std::string &)name callback:(ads::LoadCallback)callback
{
  const auto contents = [self.commonOps loadContentsFromFileWithName:name];
  if (contents.empty()) {
    callback(ads::Result::FAILED, "");
  } else {
    callback(ads::Result::SUCCESS, contents);
  }
}

- (const std::string)loadResourceForId:(const std::string &)id
{
  const auto bundle = [NSBundle bundleForClass:[BATBraveAds class]];
  const auto path = [bundle pathForResource:[NSString stringWithUTF8String:id.c_str()] ofType:nil];
  if (!path || path.length == 0) {
    return "";
  }
  NSError *error = nil;
  const auto contents = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:&error];
  if (!contents || error) {
    return "";
  }
  return std::string(contents.UTF8String);
}

- (void)reset:(const std::string &)name callback:(ads::ResultCallback)callback
{
  if ([self.commonOps removeFileWithName:name]) {
    callback(ads::Result::SUCCESS);
  } else {
    callback(ads::Result::FAILED);
  }
}

- (void)save:(const std::string &)name value:(const std::string &)value callback:(ads::ResultCallback)callback
{
  if ([self.commonOps saveContents:value name:name]) {
    callback(ads::Result::SUCCESS);
  } else {
    callback(ads::Result::FAILED);
  }
}

#pragma mark - Logging

- (void)log:(const char *)file line:(const int)line verboseLevel:(const int)verbose_level message:(const std::string &) message
{
  rewards::LogMessage(file, line, verbose_level, [NSString stringWithUTF8String:message.c_str()]);
}

#pragma mark - Notifications

- (nullable BATAdNotification *)adsNotificationForIdentifier:(NSString *)identifier
{
  if (![self isAdsServiceRunning]) { return nil; }
  ads::AdNotificationInfo info;
  if (ads->GetAdNotification(identifier.UTF8String, &info)) {
    return [[BATAdNotification alloc] initWithNotificationInfo:info];
  }
  return nil;
}

- (bool)shouldShowNotifications
{
  return [self.notificationsHandler shouldShowNotifications];
}

- (void)showNotification:(std::unique_ptr<ads::AdNotificationInfo>)info
{
  if (info.get() == nullptr) {
    return;
  }
  const auto notification = [[BATAdNotification alloc] initWithNotificationInfo:*info];
  [self.notificationsHandler showNotification:notification];
}

- (void)closeNotification:(const std::string &)id
{
  const auto bridgedId = [NSString stringWithUTF8String:id.c_str()];
  [self.notificationsHandler clearNotificationWithIdentifier:bridgedId];
}

- (bool)shouldAllowAdsSubdivisionTargeting {
  return self.shouldAllowSubdivisionTargeting;
}

- (void)setAllowAdsSubdivisionTargeting:(const bool)should_allow
{
  self.allowSubdivisionTargeting = should_allow;
}

- (std::string)adsSubdivisionTargetingCode
{
  return std::string([self.subdivisionTargetingCode UTF8String]);
}

- (void)setAdsSubdivisionTargetingCode:(const std::string &)subdivision_targeting_code
{
  self.subdivisionTargetingCode = [NSString stringWithCString:subdivision_targeting_code.c_str() encoding:[NSString defaultCStringEncoding]];
}

- (std::string)automaticallyDetectedAdsSubdivisionTargetingCode
{
  return std::string([self.automaticallyDetectedSubdivisionTargetingCode UTF8String]);
}

- (void)setAutomaticallyDetectedAdsSubdivisionTargetingCode:(const std::string &)subdivision_targeting_code
{
  self.automaticallyDetectedSubdivisionTargetingCode = [NSString stringWithCString:subdivision_targeting_code.c_str() encoding:[NSString defaultCStringEncoding]];
}

- (void)runDBTransaction:(ads::DBTransactionPtr)transaction callback:(ads::RunDBTransactionCallback)callback
{
  if (!adsDatabase || transaction.get() == nullptr) {
    auto response = ads::DBCommandResponse::New();
    response->status = ads::DBCommandResponse::Status::RESPONSE_ERROR;
    callback(std::move(response));
  } else {
    __block auto transactionClone = transaction->Clone();
    dispatch_async(self.databaseQueue, ^{
      __block auto response = ads::DBCommandResponse::New();
      adsDatabase->RunTransaction(std::move(transactionClone), response.get());
      dispatch_async(dispatch_get_main_queue(), ^{
        callback(std::move(response));
      });
    });
  }
}

- (void)onAdRewardsChanged
{
  // Not needed on iOS because ads do not show unless you are viewing a tab
}

#pragma mark - User Model Paths

- (NSDictionary *)userModelPaths {
  static NSDictionary *_paths = nil;

  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _paths = @{
      @"jememeholcpjpoahinnlafoiaknnmfgl": @"iso_3166_1_af",
      @"hfonhokmgmjionconfpknjfphfahdklo": @"iso_3166_1_ax",
      @"anlkmbkbgleadcacchhgdoecllpllknb": @"iso_3166_1_al",
      @"imoolhehjnpebcjecoinphmohihmbccj": @"iso_3166_1_dz",
      @"kgnhcdjacgcanjnbdcmngdeoncckfmfh": @"iso_3166_1_as",
      @"pmlmnjficamnkblapnohndlnhkkoaoco": @"iso_3166_1_ad",
      @"majdffglhcddbbanjnhfocagmmhobghd": @"iso_3166_1_ao",
      @"bhdlolcjjefaidodgffjhpbeeapabpgn": @"iso_3166_1_ai",
      @"pbanoihfljabneihobeplbciopfilajn": @"iso_3166_1_aq",
      @"cbdjliajiakicmdohhbjbgggacbjdnmn": @"iso_3166_1_ag",
      @"enadnicbppimlbpilkeaogmmfpennphn": @"iso_3166_1_ar",
      @"cpnhinhihfnhhmoknpbkcifgadjbcjnf": @"iso_3166_1_am",
      @"ocegkjjbmlnibhfjmeiaidplhcbdhomd": @"iso_3166_1_aw",
      @"kklfafolbojbonkjgifmmkdmaaimminj": @"iso_3166_1_au",
      @"jmneklmcodckmpipiekkfaokobhkflep": @"iso_3166_1_at",
      @"llmikniomoddmmghodjfbncbidjlhhid": @"iso_3166_1_az",
      @"aaoipmkakdldlippoaocoegnnfnpcokj": @"iso_3166_1_bs",
      @"megoieebjempmobckocciojgfhfmiejb": @"iso_3166_1_bh",
      @"ppkgobeickbpfmmkbhfgmgiloepdiagn": @"iso_3166_1_bd",
      @"ndfnmlonkimafoabeafnaignianecdhf": @"iso_3166_1_bb",
      @"apndmjdcfbhgfccccdmmeofpdgnlbbif": @"iso_3166_1_by",
      @"lnbdfmpjjckjhnmahgdojnfnmdmpebfn": @"iso_3166_1_be",
      @"demegfhfekncneocdlflandgegpcoofj": @"iso_3166_1_bz",
      @"jiodmgmkikfbkchgenlamoabbfnobnfh": @"iso_3166_1_bj",
      @"aeiffmlccgeaacefkfknodmnebanekbo": @"iso_3166_1_bm",
      @"hemccombjdaepjnhhdplhiaedaackooa": @"iso_3166_1_bt",
      @"dggplmjbjalpdgndkigojpikblaiflke": @"iso_3166_1_bo",
      @"jbibpedjodeigdoimlgpikphaabhdbie": @"iso_3166_1_bq",
      @"iefeaaegnnpiadjfoeifapjmflnbjehg": @"iso_3166_1_ba",
      @"bfdagcnfmfaidkkjcmfjmogiefoiglem": @"iso_3166_1_bw",
      @"kfimhlhdlhimaheficomcahhbaicoele": @"iso_3166_1_bv",
      @"fbpmbjccnaaeogogeldlomcmlhllgaje": @"iso_3166_1_br",
      @"cpbmgmnfoakodmmpppabghhckchppneg": @"iso_3166_1_io",
      @"gcikmigghagkligpileoekfjmohokjhm": @"iso_3166_1_bn",
      @"ahhbponhjohgifhjbjggafboffbimmmg": @"iso_3166_1_bg",
      @"fjgjapaemndekhnfeopeoeajfpmlgemo": @"iso_3166_1_bf",
      @"cfbbahiimladdkhblpkkokkmemlmkbhe": @"iso_3166_1_bi",
      @"oeneodeckioghmhokkmcbijfanjbanop": @"iso_3166_1_cv",
      @"cmknopomfihgdpjlnjhjkmogaooeoeic": @"iso_3166_1_kh",
      @"mmiflfidlgoehkhhkeodfdjpbkkjadgi": @"iso_3166_1_cm",
      @"gpaihfendegmjoffnpngjjhbipbioknd": @"iso_3166_1_ca",
      @"efpgpbmpbkhadlnpdnjigldeebgacopp": @"iso_3166_1_ky",
      @"ljfeoddejgcdofgnpgljaeiaemfimgej": @"iso_3166_1_cf",
      @"oahnhemdhgogkhgljdkhbficecbplmdf": @"iso_3166_1_td",
      @"gbbfjnnpelockljikcmjkeodlaebjokm": @"iso_3166_1_cl",
      @"gfccfpdijajblfnkmddbflphiiibifik": @"iso_3166_1_cn",
      @"mjennfbimaafgcoekloojmbhnkophgni": @"iso_3166_1_cx",
      @"pnfogoijegjkepejdabehdfadbkpgoed": @"iso_3166_1_cc",
      @"cegiaceckhbagmmfcoeebeghiddghbkk": @"iso_3166_1_co",
      @"efcmpmpbmaehimomnmonodlpnghelnfi": @"iso_3166_1_km",
      @"hkgnnbjmfcelmehmphhbjigedknjobaa": @"iso_3166_1_cg",
      @"kignebofcmcgmjfiapilgdfkbekmkdmg": @"iso_3166_1_cd",
      @"clcghlkineckendfhkgdpkplofknofjo": @"iso_3166_1_ck",
      @"hmmoibmjgckbeejmcfflnngeacaklchb": @"iso_3166_1_cr",
      @"nopcbglolocphcdeikfkoeppkhijacij": @"iso_3166_1_ci",
      @"mjhnpmgafkmildljebajibghemlbffni": @"iso_3166_1_hr",
      @"mdopdmalncfakkimlojioichjbebcaip": @"iso_3166_1_cu",
      @"boeecnnjahpahhockgdcbdlaimpcflig": @"iso_3166_1_cw",
      @"hknminnkgcjafipipbbalkakppehkpjn": @"iso_3166_1_cy",
      @"iejekkikpddbbockoldagmfcdbffomfc": @"iso_3166_1_cz",
      @"kmfkbonhconlbieplamnikedgfbggail": @"iso_3166_1_dk",
      @"phihhhnelfclomhodhgalooldjgejgfl": @"iso_3166_1_dj",
      @"obihnhimgbeenjgfnlcdbfdgkkgeogdp": @"iso_3166_1_dm",
      @"gciaobanmdlfkegiikhgdoogegeghhch": @"iso_3166_1_do",
      @"imnpbmpnmlmkjpgfnfeplikingjklpej": @"iso_3166_1_ec",
      @"ojfkdbfibflfejobeodhoepghgoghkii": @"iso_3166_1_eg",
      @"adnhangbagjlobdeicimblgcnafegpfb": @"iso_3166_1_sv",
      @"gncihmgakhljdlkadibldhdfnocfplci": @"iso_3166_1_gq",
      @"diacfpapelanfbkmehdpaaohmnkkngge": @"iso_3166_1_er",
      @"jdigggiplmjlocdopbdmjibckpamigij": @"iso_3166_1_ee",
      @"npacefioaofgbofilfnhliofkefklipp": @"iso_3166_1_sz",
      @"lbiagcddmfapjfbebccolcahfppaimmo": @"iso_3166_1_et",
      @"aogeahmaehgnkobkmnmkhkimdjpgfgen": @"iso_3166_1_fk",
      @"molpidmcmbmhbicckmbopbmiojddebke": @"iso_3166_1_fo",
      @"biobhkionbllnfljaapocfpdmhamedkf": @"iso_3166_1_fj",
      @"ecneelfoifpgnignhipdebhbkgcphmic": @"iso_3166_1_fi",
      @"bgifagoclclhhoflocdefiklgodpihog": @"iso_3166_1_fr",
      @"mhmfclafeemiphfebhnobinkplbmpocm": @"iso_3166_1_gf",
      @"mjaianjdadeiocokpoanbgjhegficcce": @"iso_3166_1_pf",
      @"jbjodokafbafhemocebljdblgnfajabi": @"iso_3166_1_tf",
      @"nchncleokkkkdfgbgmenkpkmhnlbibmg": @"iso_3166_1_ga",
      @"alebifccfdpcgpandngmalclpbjpaiak": @"iso_3166_1_gm",
      @"kaikhlldfkdjgddjdkangjobahokefeb": @"iso_3166_1_ge",
      @"dgkplhfdbkdogfblcghcfcgfalanhomi": @"iso_3166_1_de",
      @"panlkcipneniikplpjnoddnhonjljbdp": @"iso_3166_1_gh",
      @"pibapallcmncjajdoamgdnmgcbefekgn": @"iso_3166_1_gi",
      @"ochemplgmlglilaflfjnmngpfmpmjgnb": @"iso_3166_1_gr",
      @"gjknekmnjalchmfjbecifkoohcmlolhp": @"iso_3166_1_gl",
      @"kbllnlfaaoonbliendlhmoleflgdekki": @"iso_3166_1_gd",
      @"keggdlgkcfmmopgnogknokiocjjieapm": @"iso_3166_1_gp",
      @"mfnbeofelbcabhloibhpklffiifjcdfl": @"iso_3166_1_gu",
      @"jdhabeecpolnjdiplbcpjlohmlgdjgjh": @"iso_3166_1_gt",
      @"ncldbgolkmlgfkoignkdjehiadnfmlid": @"iso_3166_1_gg",
      @"gcfgdkmegcjaceeofigbabmjjiojdgnb": @"iso_3166_1_gn",
      @"kheejcjggceghjdinbcklehfkobloboc": @"iso_3166_1_gw",
      @"fehpbhdbjpnaibhncpeedfkogiiajcne": @"iso_3166_1_gy",
      @"pkpmecljhbjbiicbnfcfgpgmpehldemm": @"iso_3166_1_ht",
      @"kfjeoeekjccibpockdjcdjbgagaaopdj": @"iso_3166_1_hm",
      @"ljhceaiogeejahjjblnfaaocgogchpkb": @"iso_3166_1_va",
      @"llmmfofcojgcignfnaodhdhdhphhmfmf": @"iso_3166_1_hn",
      @"plpcpclbpkilccbegfbpediidmejaahc": @"iso_3166_1_hk",
      @"pofhnfhkonpjephlcjlmbjmlikiaddoc": @"iso_3166_1_hu",
      @"cljplhmgochlncaelcabfgeefebhmfnk": @"iso_3166_1_is",
      @"kdkakecfnmlfifkhlekmfkmmkpgeckcl": @"iso_3166_1_in",
      @"lanimmipljlbdnajkabaoiklnpcaoakp": @"iso_3166_1_id",
      @"mhiehehcmljjmpibmpiadoeefnchmbdm": @"iso_3166_1_ir",
      @"oejhcomgcgcofdfchkdhkjohnioijofn": @"iso_3166_1_iq",
      @"fbmfelhaipnlicodibhjkmeafbcgpfnm": @"iso_3166_1_ie",
      @"blofecpleppfodonanffdbepbiijmklm": @"iso_3166_1_im",
      @"fiodhmddlgkgajbhohfdmkliikidmaom": @"iso_3166_1_il",
      @"gjkhegliajlngffafldbadcnpfegmkmb": @"iso_3166_1_it",
      @"ncfbonfnhophngmkkihoieoclepddfhm": @"iso_3166_1_jm",
      @"ienmdlgalnmefnpjggommgdilkklopof": @"iso_3166_1_jp",
      @"lfgnglkpngeffaijiomlppnobpchhcgf": @"iso_3166_1_je",
      @"gnkmfghehkoegoabhndflbdmfnlgeind": @"iso_3166_1_jo",
      @"jadlfgggcfdhicaoacokfpmccbmedkim": @"iso_3166_1_kz",
      @"bfhpiebgnciehokapeppcinalnibbnan": @"iso_3166_1_ke",
      @"dkghhhflbpfknidjbhlophapheggpahk": @"iso_3166_1_ki",
      @"pnokpaenadbgpjpmlnoamnmpjbjlfoaf": @"iso_3166_1_kp",
      @"clgbjhhcdihjgbomhpmfdjdiagejadja": @"iso_3166_1_kr",
      @"ehkeinmpkojiiacjohbalbnhloiaifig": @"iso_3166_1_kw",
      @"hehalbiboicjbbcfhckdfeijjjppdhij": @"iso_3166_1_kg",
      @"lhjcndbhldpnapjddfgohdcdmfibgpon": @"iso_3166_1_la",
      @"pooflbdadogbmjmnnppfmklfjbmoblfa": @"iso_3166_1_lv",
      @"hkengofpokonjepdafjdeclejledncdj": @"iso_3166_1_lb",
      @"mdojkinfephdfhbfadcnnfcjfniefael": @"iso_3166_1_ls",
      @"alenpijagefjamgompebcjhbfhepnphh": @"iso_3166_1_lr",
      @"mnhglgpnnohpipdeinibpbnlnpledicf": @"iso_3166_1_ly",
      @"onhaidkdpiboaolhnaddeddfaabomchd": @"iso_3166_1_li",
      @"aokfbnlokidoepkhilbmfdkdhajkpbli": @"iso_3166_1_lt",
      @"gnmaofjfninpeccldcmlkbinhhohmbck": @"iso_3166_1_lu",
      @"ncmdondkaofghlnhiabnfilafhmabong": @"iso_3166_1_mo",
      @"lapgbedoccnchodbgfmafpkkhlfmcehe": @"iso_3166_1_mg",
      @"dhmcaoadkmoejegjpjgkjhibioemkfni": @"iso_3166_1_mw",
      @"dadpenhnclbbkjfbkgkgecknfjggpbmm": @"iso_3166_1_my",
      @"ggclalmmmmgjcoleeficgnnjkpgeinfd": @"iso_3166_1_mv",
      @"flocoipmnbpcodjfhmkjecjpbkcmkecp": @"iso_3166_1_ml",
      @"emckddclmcjoilbadmdjdakabpnkdkhk": @"iso_3166_1_mt",
      @"cpjafhooepmhnflmjabfeaiopfbljhpo": @"iso_3166_1_mh",
      @"chbeaiccoofemohdajloflfkblbgdiih": @"iso_3166_1_mq",
      @"dfmnoondmnbngeilibiicikjenjjeigi": @"iso_3166_1_mr",
      @"iobofpagkcicpcijjfmnghgppbghnpdo": @"iso_3166_1_mu",
      @"lcnaekpkllhpljanlibochejknjflodn": @"iso_3166_1_yt",
      @"dclpeadnefbjogjcamdglgmmbbgnjcob": @"iso_3166_1_mx",
      @"pjiglaefpchnekpbkbfngjnfhlcmhgbk": @"iso_3166_1_fm",
      @"paiickhoniddnnlhhdmhjcfemgkgfohn": @"iso_3166_1_md",
      @"iloglofhibeghkfbocphifnfpccmplgd": @"iso_3166_1_mc",
      @"pclbpikpdcdondhappcgloeohjgammia": @"iso_3166_1_mn",
      @"dkjadbekoidbnlmaomlcjjgkofkechlo": @"iso_3166_1_me",
      @"mknfcplgmgbfkklaiimdakgjbeonapeh": @"iso_3166_1_ms",
      @"pmbhpljpfciommdigfblnenpdiapdafl": @"iso_3166_1_ma",
      @"gilieeicpdnkcjbohfhjhpmpjocapbko": @"iso_3166_1_mz",
      @"bbeoopklmfincipdlffbbphpjefmimmp": @"iso_3166_1_mm",
      @"paoffgbbehbibcihhmboiaebgojdnibj": @"iso_3166_1_na",
      @"jpejbbflggaiaflclgomjcolpomjmhlh": @"iso_3166_1_nr",
      @"ohodaiianeochclnnobadfikohciggno": @"iso_3166_1_np",
      @"choggjlbfndjppfiidbhmefapnlhcdhe": @"iso_3166_1_nl",
      @"apmipakgigaapfahiojgjgkfgcdekbpp": @"iso_3166_1_nc",
      @"dlbokjgcdlhkgfeklggoncjhihaebnai": @"iso_3166_1_nz",
      @"jajkonoepahongnlnfbfmlnpnkjkchof": @"iso_3166_1_ni",
      @"mmhmpjfgnhibhfccegfohnibkpooppkn": @"iso_3166_1_ne",
      @"bhkddokohamnindobkmifljnpgijdjdh": @"iso_3166_1_ng",
      @"celbcocehclbnblfndjfjleagcbbpooc": @"iso_3166_1_nu",
      @"bcnnffpigdndbdohgifflckehcoofigc": @"iso_3166_1_nf",
      @"njlgnoebifbjpafbmnkkchknkinmeljm": @"iso_3166_1_mk",
      @"cpjjnbhhjohkkmkkplcfkobjfbjlildd": @"iso_3166_1_mp",
      @"ciibjdmjfejjghmnlonlihnjodfckfbo": @"iso_3166_1_no",
      @"cobdmgempkofdfhgmbhfckemppmlbjob": @"iso_3166_1_om",
      @"aiaabcbklimkipbpalfoaehfdebbainb": @"iso_3166_1_pk",
      @"ejlnmikcbnjpaaolkheodefhahiabjga": @"iso_3166_1_pw",
      @"iienfoenehmoepjgljgjdkenjedjfjol": @"iso_3166_1_ps",
      @"aafjalakdldginkbeobaiamnfphcdmko": @"iso_3166_1_pa",
      @"monkjbjmhjepdcaedlejhmjjjcjpiiaa": @"iso_3166_1_pg",
      @"aoidaoefdchfhdjfdffjnnlbfepfkadj": @"iso_3166_1_py",
      @"pmbmbglgbofljclfopjkkompfgedgjhi": @"iso_3166_1_pe",
      @"ocmnmegmbhbfmdnjoppmlbhfcpmedacn": @"iso_3166_1_ph",
      @"ccpkbhegiebckfidhnoihgdmddhnmdfh": @"iso_3166_1_pn",
      @"feeklcgpaolphdiamjaolkkcpbeihkbh": @"iso_3166_1_pl",
      @"gchnahcajhccobheiedkgdpfboljkhge": @"iso_3166_1_pt",
      @"bpjdfagamlhoojajkeifbendedaikinl": @"iso_3166_1_pr",
      @"jicllaljbaldhopinkkegkfpmmnnhmbc": @"iso_3166_1_qa",
      @"aeglmpapdhfhdicbifhnmaoehffffmie": @"iso_3166_1_re",
      @"jpapeieehcilkcfpljhopohigdhbnjck": @"iso_3166_1_ro",
      @"nfcegbjbohhjljcdogkmookngaiplbna": @"iso_3166_1_ru",
      @"djjoaejcadmjbgadeijpbokipgmolnih": @"iso_3166_1_rw",
      @"fjefhohmfmokjmnibamjnpiafikmmlef": @"iso_3166_1_bl",
      @"dpodaelfodkebmgmmdoecleopjggboln": @"iso_3166_1_sh",
      @"idmipdncpnfbfonogngaimigpbpnenpb": @"iso_3166_1_kn",
      @"lhlajkngiihbjjaakfgkencpppeahhfi": @"iso_3166_1_lc",
      @"hihpbgpfcelklhigbkfnbdgjmpbnabmo": @"iso_3166_1_mf",
      @"cpkbkgenaaododkijfnfmgmpekbcfjcg": @"iso_3166_1_pm",
      @"bnonnlpingklaggdalihppicgpaghpop": @"iso_3166_1_vc",
      @"jfckclnlfaekpfklphjagmjiphjcchmj": @"iso_3166_1_ws",
      @"lneikknijgnijfnpoahmfkefboallgin": @"iso_3166_1_sm",
      @"djlblammehomffbplemhekjeghekglpc": @"iso_3166_1_st",
      @"gmhojjgbbfachddbgojljenplnhialka": @"iso_3166_1_sa",
      @"haalbaecaigldhgnjfmjbedegjipkdfb": @"iso_3166_1_sn",
      @"dlfdepidpnibdoblimabdmgpobophapn": @"iso_3166_1_rs",
      @"dmdapbmagehdijbdhbdbfjijgmcppjml": @"iso_3166_1_sc",
      @"piajfdpbabffhdlmpkaejndbdnohljfn": @"iso_3166_1_sl",
      @"jilipkheolgjanjhhhdmbaleiiblnepe": @"iso_3166_1_sg",
      @"igdomgnppdmcglgohoamnpegjelohlkj": @"iso_3166_1_sx",
      @"obponfmfefkaeehakbehbnnlcbebebhd": @"iso_3166_1_sk",
      @"dckjbnoilglapbgmniiagempimbaicmn": @"iso_3166_1_si",
      @"mlbgbnccloeobccglpaachnaabgegcni": @"iso_3166_1_sb",
      @"hnfmhdkkmcgeppiiohpondjgibepgfeb": @"iso_3166_1_so",
      @"jadaiaonajphgnbamppeenldepoajgbf": @"iso_3166_1_za",
      @"ghclfflogdfhnjciileceoofmhkneggp": @"iso_3166_1_gs",
      @"kkfngpdjfcddimfgkgibaccaoehjplkn": @"iso_3166_1_ss",
      @"ganmbmiebelpdlnohnabgkkocholelbp": @"iso_3166_1_es",
      @"gmahgggkpliaoidcaknflpbgpehcjmhc": @"iso_3166_1_lk",
      @"dhcfofldcefkohnjcnfodlbiakgedidd": @"iso_3166_1_sd",
      @"khgbibnjdanhnoebnfjgpnfbkohdngph": @"iso_3166_1_sr",
      @"kchkidfjkghdocdicfpmbckmjfgnlndb": @"iso_3166_1_sj",
      @"clncjboijmbkcjgkechfhalginbnplpp": @"iso_3166_1_se",
      @"gnamhdlealpfbanappoephfdjeoehggd": @"iso_3166_1_ch",
      @"hnhakbhiamjojdoajhebemlajheokinm": @"iso_3166_1_sy",
      @"jejmkjlhckkijknapfhfoogakgoelhen": @"iso_3166_1_tw",
      @"nfpgpnagpefhcijfnabpdejiiejplonp": @"iso_3166_1_tj",
      @"jnlkpmlmfdipllbnjmjomkddplafclch": @"iso_3166_1_tz",
      @"mjkmkfbpiegjkbeolgpomaloeiiffodm": @"iso_3166_1_th",
      @"kmdanbbapegbkpjkfdldmekconhnmmmo": @"iso_3166_1_tl",
      @"alinepjaedjagibhfjcejemabijbohmi": @"iso_3166_1_tg",
      @"bbobjkhpggphapdpcchkbklglkindkcc": @"iso_3166_1_tk",
      @"jdkdhebphdakbabdbgefjkdbdoagmdec": @"iso_3166_1_to",
      @"nbmopmgpfmalleghhbkablkoamofibpk": @"iso_3166_1_tt",
      @"hgmkfpcpppjheoknjjfpajfmibkndjdf": @"iso_3166_1_tn",
      @"fahflofbglhemnakgdmillobeencekne": @"iso_3166_1_tr",
      @"fhbmmefncojhnjhbckninoliclloeeac": @"iso_3166_1_tm",
      @"hbiblijpgfgphhfoajjmcgdbhmobbfba": @"iso_3166_1_tc",
      @"kennokhomgefcfjjjhckbiidlhmkinca": @"iso_3166_1_tv",
      @"bolcbpmofcabjoflcmljongimpbpeagb": @"iso_3166_1_ug",
      @"enkpeojckjlmehbniegocfffdkanjhef": @"iso_3166_1_ua",
      @"ajdiilmgienedlgohldjicpcofnckdmn": @"iso_3166_1_ae",
      @"cdjnpippjnphaeahihhpafnneefcnnfh": @"iso_3166_1_gb",
      @"kkjipiepeooghlclkedllogndmohhnhi": @"iso_3166_1_us",
      @"ocikkcmgfagemkpbbkjlngjomkdobgpp": @"iso_3166_1_um",
      @"cejbfkalcdepkoekifpidloabepihogd": @"iso_3166_1_uy",
      @"chpbioaimigllimaalbibcmjjcfnbpid": @"iso_3166_1_uz",
      @"ogbkgicanbpgkemjooahemnoihlihonp": @"iso_3166_1_vu",
      @"okopabpainkagabcmkfnnchaakhimaoe": @"iso_3166_1_ve",
      @"jcffalbkohmmfjmgkdcphlimplejkmon": @"iso_3166_1_vn",
      @"jlfjphoakpnmhpldmdkdhekopbjmkljn": @"iso_3166_1_vg",
      @"infpagefbmdijbaigeagmldkjnjdhhfa": @"iso_3166_1_vi",
      @"hefgpgfflbaepfgbafaaadffchekggfg": @"iso_3166_1_wf",
      @"fjhkoeiglahhpcmgfpalgckcaoaifjkn": @"iso_3166_1_eh",
      @"cijopjkddpagbkjpnnbjcecfamjbkakp": @"iso_3166_1_ye",
      @"inmfjchmafaondfnpgffefgbdmmfgenb": @"iso_3166_1_zm",
      @"fmobbdfaoifmdjonfklmapdliabjdmjp": @"iso_3166_1_zw",
      @"ijmgabghpbflfadffhpmjklamndnonha": @"iso_639_1_ab",
      @"hddanjaanmjbdklklpldpgpmbdmaiihb": @"iso_639_1_aa",
      @"blcjdmhlkddhompnlbjlpccpelipihem": @"iso_639_1_af",
      @"pecokcgeeiabdlkfkfjpmfldfhhjlmom": @"iso_639_1_ak",
      @"pgkommhmfkkkfbbcnnfhlkagbodoimjm": @"iso_639_1_sq",
      @"emopjfcnkbjfedjbfgajdajnkkfekcbl": @"iso_639_1_am",
      @"hfiknbegiiiigegdgpcgekhdlpdmladb": @"iso_639_1_ar",
      @"onjbjcnjheabeghbflckfekjnnjgfabn": @"iso_639_1_an",
      @"ghgfdmhmhifphaokjfplffppmlfchofm": @"iso_639_1_hy",
      @"mbcmenffnlanegglgpdgbmmldfpclnkm": @"iso_639_1_as",
      @"clemenahenllbgeilmkllcpfcfhloebp": @"iso_639_1_av",
      @"cmggjadifnmfdfeohgomnaggonihmbli": @"iso_639_1_ae",
      @"fabdaiigiipinlpgkkakcffbhkmfnfek": @"iso_639_1_ay",
      @"cgpdnipmfmbfceokoadgbliclnoeddef": @"iso_639_1_az",
      @"hjfgajachhnlgjfjgbjbehgcoecfigap": @"iso_639_1_bm",
      @"keghklelbpkaiogcinbjnnjideedfmdd": @"iso_639_1_ba",
      @"lldcgicjgpomllohjncpcjkijjklooji": @"iso_639_1_eu",
      @"ogmkkmobemflinkielcngdanaojkamcc": @"iso_639_1_be",
      @"ligbalgipjnajannojamijenlackcioc": @"iso_639_1_bn",
      @"bkicjkclpdihdjbpajnegckopabcebff": @"iso_639_1_bh",
      @"dafhhkffcifanfgpjlgejcahkidpbnfj": @"iso_639_1_bi",
      @"ngnfehahlmgkoclalhldboigojplccbl": @"iso_639_1_bs",
      @"hkfnnljkbmadknefddjfligobbiagmea": @"iso_639_1_br",
      @"igdpeoeohlgknkdnjhbijpfeenfiepdc": @"iso_639_1_bg",
      @"mdacdgffhlidpdmhckokeoajhojeibmp": @"iso_639_1_my",
      @"keahfifnfebhoaaghffigjnhgbkkaibd": @"iso_639_1_ca",
      @"fllhkadnpidionapphjicdkfdiloghad": @"iso_639_1_ch",
      @"eakppbikiomjdnligoccikcdipojiljk": @"iso_639_1_ce",
      @"ddekfkhnjcpbankekbelkeekibbhmgnh": @"iso_639_1_ny",
      @"oblfikajhadjnmjiihdchdfdcfehlbpj": @"iso_639_1_zh",
      @"obdagonejiaflgbifdloghkllgdjpcdj": @"iso_639_1_cv",
      @"apmkijnjnhdabkckmkkejgdnbgdglocb": @"iso_639_1_kw",
      @"gdmbpbmoiogajaogpajfhonmlepcdokn": @"iso_639_1_co",
      @"amedpgedagedjlkgjajebgecfkldmdfa": @"iso_639_1_cr",
      @"ncjeinjdknabbgmaijmnipndbggmchff": @"iso_639_1_hr",
      @"nllaodpgkekajbabhjphhekenlnlpdmd": @"iso_639_1_cs",
      @"klifniioldbebiedppmbobpdiombacge": @"iso_639_1_da",
      @"aoljgchlinejchjbbkicamhfdapghahp": @"iso_639_1_dv",
      @"opoleacilplnkhobipjcihpdoklpnjkk": @"iso_639_1_nl",
      @"jginkacioplimdjobccplmgiphpjjigl": @"iso_639_1_dz",
      @"emgmepnebbddgnkhfmhdhmjifkglkamo": @"iso_639_1_en",
      @"halbpcgpgcafebkldcfhllomekophick": @"iso_639_1_eo",
      @"onmakacikbbnhmanodpjhijljadlpbda": @"iso_639_1_et",
      @"bjhkalknmdllcnkcjdjncplkbbeigklb": @"iso_639_1_ee",
      @"jamflccjbegjmghgaljipcjjbipgojbn": @"iso_639_1_fo",
      @"gfoibbmiaikelajlipoffiklghlbapjf": @"iso_639_1_fj",
      @"djokgcimofealcnfijnlfdnfajpdjcfg": @"iso_639_1_fi",
      @"hbejpnagkgeeohiojniljejpdpojmfdp": @"iso_639_1_fr",
      @"anhpkncejedojfopbigplmbfmbhkagao": @"iso_639_1_ff",
      @"ejioajnkmjfjfbbanmgfbagjbdlfodge": @"iso_639_1_gl",
      @"hlipecdabdcghhdkhfmhiclaobjjmhbo": @"iso_639_1_ka",
      @"eclclcmhpefndfimkgjknaenojpdffjp": @"iso_639_1_de",
      @"aefhgfnampgebnpchhfkaoaiijpmhcca": @"iso_639_1_el",
      @"ebgmbleidclecpicaccgpjdhndholiel": @"iso_639_1_gn",
      @"mdpcebhdjplkegddkiodmbjcmlnhbpog": @"iso_639_1_gu",
      @"hpkelamfnimiapfbeeelpfkhkgfoejil": @"iso_639_1_ht",
      @"khgpojhhoikmhodflkppdcakhbkaojpi": @"iso_639_1_ha",
      @"gffjpkbdngpbfkflpnoodjfkpelbappk": @"iso_639_1_he",
      @"pkmkhkpglegjekkfabaelkbgkfegnmde": @"iso_639_1_hz",
      @"emhbebmifclalgbdpodobmckfehlkhfp": @"iso_639_1_hi",
      @"cmkgcbedakcdopgjdhbbpjjaodjcdbdp": @"iso_639_1_ho",
      @"ifbdofecjcadpnkokmdfahhjadcppmko": @"iso_639_1_hu",
      @"hoeihggfhgnfpdnaeocfpmoelcenhfla": @"iso_639_1_ia",
      @"gbmolmcnbhegkhjhbhjehcpbjonjlgfg": @"iso_639_1_id",
      @"fioklomfllflofcdiklpgabemclgkkdh": @"iso_639_1_ie",
      @"oiihbhoknlbjonghmcplpbpkcdeinlfg": @"iso_639_1_ga",
      @"nchaailfhkbnlnaobgjmoamdfnclhdoo": @"iso_639_1_ig",
      @"fbdjobfnceggaokdnlebbaplnhednlhl": @"iso_639_1_ik",
      @"nkajjfpapgfhlcpmmoipafbfnnmojaep": @"iso_639_1_io",
      @"dhhkjdedjghadoekfklpheblplmlpdec": @"iso_639_1_is",
      @"ijaiihoedhaocihjjkfjnhfhbceekdkg": @"iso_639_1_it",
      @"eociikjclgmjinkgeoehleofopogehja": @"iso_639_1_iu",
      @"ncnmgkcadooabjhgjlkkdipdnfokpjnm": @"iso_639_1_ja",
      @"jdfafcdnmjeadcohbmjeeijgheobldng": @"iso_639_1_jv",
      @"jknflnmanienedkoeoginjmbpmkpclki": @"iso_639_1_kl",
      @"nggdckgfnooehkpnmjdodbknekmhcdeg": @"iso_639_1_kn",
      @"mhobnohaonkecggnifnffofnihbakjic": @"iso_639_1_kr",
      @"hhejjhncnckfmpkpkhkbknekhkepcill": @"iso_639_1_ks",
      @"ljllkgialkdmamlacenmjkhjmimndfil": @"iso_639_1_kk",
      @"dhigelnhdnmhffomicjdmdhefhedgijm": @"iso_639_1_km",
      @"jcfelbpkigpapilbogpidbfdffgdfafe": @"iso_639_1_ki",
      @"ookcfboifajbeojnnebiaoplifgiinof": @"iso_639_1_rw",
      @"njimokpabbaelbbpohoflcbjhdgonbmf": @"iso_639_1_ky",
      @"danmahhfdmncbiipbefmjdkembceajdk": @"iso_639_1_kv",
      @"lcahllbcfbhghpjjocdhmilokfbpbekn": @"iso_639_1_kg",
      @"jbhiacghlejpbieldkdfkgenhnolndlf": @"iso_639_1_ko",
      @"hfboaaehpnfpnpompagbamoknlnladfn": @"iso_639_1_ku",
      @"cppbcboljlmfdgeehadijemhkifhcpnl": @"iso_639_1_kj",
      @"knnpciecjoakhokllbgioaceglldlgan": @"iso_639_1_la",
      @"chnbfebpflegknnjiikofmnebcbphead": @"iso_639_1_lb",
      @"hkfkdejbdionnjdhgfhpkcmknmamddde": @"iso_639_1_lg",
      @"nnbaaidlgckbmfdlnioepikbcmjmbadb": @"iso_639_1_li",
      @"dedchpogdooaakplmpjobpkiggalnlif": @"iso_639_1_ln",
      @"alaghdpgppakjfjcipokbaempndnglfk": @"iso_639_1_lo",
      @"copfpkggfedomijbmceepmahananponb": @"iso_639_1_lt",
      @"ljambfnmibabkhcpgppfblodipceahab": @"iso_639_1_lu",
      @"lflklgkjbemnncejeanindnikiaicpod": @"iso_639_1_lv",
      @"lkcfaihllkinjhmdjgaccjhgdobabpbj": @"iso_639_1_gv",
      @"anikcbbbhcobgockdcemaboadbdcnhlg": @"iso_639_1_mk",
      @"gaompbafbaolhddgoafjhkgmnjpjpbip": @"iso_639_1_mg",
      @"pppjaeenalohmnhjpemhdkkkghfddbfp": @"iso_639_1_ms",
      @"papnfbjjblebaaeenodbmllepiijfmhn": @"iso_639_1_ml",
      @"jmakhmnfhlhioagnmgnakhigadgkpkcl": @"iso_639_1_mt",
      @"gakleannelcniohpkolmbgfpnkjlbnll": @"iso_639_1_mi",
      @"lmpaafjmphnclpkfcejgjbnieahkgemg": @"iso_639_1_mr",
      @"fehmnfinbijjdacjgeofhojfdjhbehic": @"iso_639_1_mh",
      @"aadlcogkfhaficfijoolhlajkngeecea": @"iso_639_1_mn",
      @"edcpinkoiknmjafcdpolkkeiieddmbab": @"iso_639_1_na",
      @"ooncphbdmekmhldbojgoboebnongkpic": @"iso_639_1_nv",
      @"kemfolmmdooeepfhbpiemnnekfjlbnnd": @"iso_639_1_nd",
      @"mihcpmkclenpfgielcipmdbcfpncfojc": @"iso_639_1_ne",
      @"jpleaamlgnfhfemdcfmbknnhcbfnglkh": @"iso_639_1_ng",
      @"gbigbbblbdmfhbpobcliehihdedicjfl": @"iso_639_1_nb",
      @"fnmjhgcaloniglpngailbaillhbenela": @"iso_639_1_nn",
      @"fgicknpghikljlipmfibgghcdikjfjfj": @"iso_639_1_no",
      @"nodfcenkjehpafmbmgnapoieilnoijap": @"iso_639_1_ii",
      @"dghnlalnaoekcligakadcmmioaieangj": @"iso_639_1_nr",
      @"fbfeebiglbpbmgbefgmijdbcchmdfdhm": @"iso_639_1_oc",
      @"gdkeabmpgllapbjgifgfmlfelpdlkapj": @"iso_639_1_oj",
      @"fnhldinjahkdbngcnjfcmidhpjedinbg": @"iso_639_1_cu",
      @"aegokocmijocdgiddgjbjkdfiheijfpl": @"iso_639_1_om",
      @"amkpggbpieoafkbmkijjnefikhjjfogn": @"iso_639_1_or",
      @"adccmiokahgjhdbmhldbnkkplgcgfkpp": @"iso_639_1_os",
      @"ghikcfknmlkdjiiljfpgpmcfjinpollk": @"iso_639_1_pa",
      @"hinecgnhkigghpncjnflaokadaclcfpm": @"iso_639_1_pi",
      @"blaocfojieebnkolagngecdhnipocicj": @"iso_639_1_fa",
      @"aijecnhpjljblhnogamehknbmljlbfgn": @"iso_639_1_pl",
      @"fikmpfipjepnnhiolongfjimedlhaemk": @"iso_639_1_ps",
      @"ikpplkdenofcphgejneekjmhepajgopf": @"iso_639_1_pt",
      @"ndlciiakjgfcefimfjlfcjgohlgidpnc": @"iso_639_1_qu",
      @"nlabdknfkecjaechkekdlkgnapljpfla": @"iso_639_1_rm",
      @"piebpodmljdloocefikhekfjajloneij": @"iso_639_1_rn",
      @"hffipkehifobjlkdjagndofmpjnpkgje": @"iso_639_1_ro",
      @"nigmjcnboijpcoikglccmoncigioojpa": @"iso_639_1_ru",
      @"inkmdnaeojfdngbdkbinoinflfahcjfc": @"iso_639_1_sa",
      @"golaphmkhjkdmcakpigbjhneagiapkfh": @"iso_639_1_sc",
      @"kcmiboiihhehobbffjebhgadbalknboh": @"iso_639_1_sd",
      @"cmomlghkjichjgbkakaoenfenincefnj": @"iso_639_1_se",
      @"mfaajikelgfodgcgnldapbgjdbncmibc": @"iso_639_1_sm",
      @"gndfhmmkadfdhmchhljmcdhlicdmmlbn": @"iso_639_1_sg",
      @"pdgppejghdoknllcnfikoaloobopajjo": @"iso_639_1_sr",
      @"djmefhmnkffabdodgcfjmgffpindaaak": @"iso_639_1_gd",
      @"bdepmnbdekgdgjimffimkfeoggmnlbbf": @"iso_639_1_sn",
      @"mogllbjhcnfhcejalaikleeogjmmfkdm": @"iso_639_1_si",
      @"gnhdcgmlejfbcccdjdhjalacjcimlkjh": @"iso_639_1_sk",
      @"jifgjineejhedlmjnkcijoincbhelicp": @"iso_639_1_sl",
      @"doclofgiadjildnifgkajdlpngijbpop": @"iso_639_1_so",
      @"mgdaglmilmjenimbkdmneckfbphfllic": @"iso_639_1_st",
      @"ahiocclicnhmiobhocikfdamfccbehhn": @"iso_639_1_es",
      @"aondicpcneldjpbfemaimbpomgaepjhg": @"iso_639_1_su",
      @"ccmmjlklhnoojaganaecljeecenhafok": @"iso_639_1_sw",
      @"khbhchcpljcejlmijghlabpcmlkkfnid": @"iso_639_1_ss",
      @"jpgndiehmchkacbfggdgkoohioocdhbp": @"iso_639_1_sv",
      @"nbmbpelgpalcgdghkeafabljjbhmalhf": @"iso_639_1_ta",
      @"nonmahhknjgpnoamcdihefcbpdioimbh": @"iso_639_1_te",
      @"olopfkdcklijkianjbegdegilmhdgpcj": @"iso_639_1_tg",
      @"jllmphacilbjnfngcojfgmiimipclfbm": @"iso_639_1_th",
      @"hkeoedmbihkkglaobeembiogeofffpop": @"iso_639_1_ti",
      @"ijgcgakmmgjaladckdppemjgdnjlcgpo": @"iso_639_1_bo",
      @"liddcpbnodlgenmbmmfchepoebgfondk": @"iso_639_1_tk",
      @"kcoilhabhhnfdakenmhddnhngngggcmp": @"iso_639_1_tl",
      @"gjinficpofcocgaaogaiimhacbfkmjmj": @"iso_639_1_tn",
      @"hhliclmbfpdlpkdhmpkleicjnemheeea": @"iso_639_1_to",
      @"kpdcfihnokkbialolpedfamclbdlgopi": @"iso_639_1_tr",
      @"nhbpjehmiofogaicflcjhcfdmmkgbohp": @"iso_639_1_ts",
      @"mmmembcojnkgfclnogjfeeiicmiijcnk": @"iso_639_1_tt",
      @"ldjaelhegioonaebajlgfnhcipdajfeo": @"iso_639_1_tw",
      @"fnokionjijmckgggmhogifnocpabdafk": @"iso_639_1_ty",
      @"ohcnbpfpchlcjchnljcdjebcjdbneecj": @"iso_639_1_ug",
      @"pbegjfplhidokoonohceljofcpbojglg": @"iso_639_1_uk",
      @"jaggpekahffhnhhchdkpnigfmdlhenpo": @"iso_639_1_ur",
      @"jephmoboccidmbemhjckbcagagijgcef": @"iso_639_1_uz",
      @"mbhiljiiffkobikkoechkpeaopagfhep": @"iso_639_1_ve",
      @"pbjakpdfjkmcajeobebemnjglbjiniln": @"iso_639_1_vi",
      @"bfljdbgfmdjgbomhiaoeoleidbfcmmpn": @"iso_639_1_vo",
      @"fmiofedgokpciaklgakbnminmmkocgnd": @"iso_639_1_wa",
      @"gpfmbdepojhpjlaelmnlbgginpgnbmfd": @"iso_639_1_cy",
      @"mhdpccgjfkfkdbbpapbgcahhknmbdnjn": @"iso_639_1_wo",
      @"eahefjeohmofagkliaddkmokbecfhclm": @"iso_639_1_fy",
      @"gjigddoamjemfcahionjikmlfijoiecf": @"iso_639_1_xh",
      @"jhnklldjooclfmgpkipaemehnngabckf": @"iso_639_1_yi",
      @"fjfbodkpnkomodlcanacakhcfmjjgkdf": @"iso_639_1_yo",
      @"bncbapkadghlbephbogcmomlecfmdhnb": @"iso_639_1_za",
      @"dhlnknppkgfgehmmipicnlplhjgpnmnh": @"iso_639_1_zu"
    };
  });

  return  _paths;
}

@end
