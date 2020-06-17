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

#include "base/message_loop/message_loop_current.h"
#include "base/task/single_thread_task_executor.h"

#import "RewardsLogging.h"

base::SingleThreadTaskExecutor* g_task_executor = nullptr;

#define BATClassAdsBridge(__type, __objc_getter, __objc_setter, __cpp_var) \
  + (__type)__objc_getter { return ads::__cpp_var; } \
  + (void)__objc_setter:(__type)newValue { ads::__cpp_var = newValue; }

static const NSInteger kDefaultNumberOfAdsPerDay = 20;
static const NSInteger kDefaultNumberOfAdsPerHour = 2;

static NSString * const kAdsEnabledPrefKey = @"BATAdsEnabled";
static NSString * const kNumberOfAdsPerDayKey = @"BATNumberOfAdsPerDay";
static NSString * const kNumberOfAdsPerHourKey = @"BATNumberOfAdsPerHour";
static NSString * const kShouldAllowAdsSubdivisionTargetingPrefKey = @"BATShouldAllowAdsSubdivisionTargetingPrefKey";
static NSString * const kAdsSubdivisionTargetingCodePrefKey = @"BATAdsSubdivisionTargetingCodePrefKey";
static NSString * const kAutomaticallyDetectedAdsSubdivisionTargetingCodePrefKey = @"BATAutomaticallyDetectedAdsSubdivisionTargetingCodePrefKey";

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
@end

@implementation BATBraveAds

- (instancetype)initWithStateStoragePath:(NSString *)path
{
  if ((self = [super init])) {
    // TODO(brave): Added task executor to ledger when ledger uses Timer/RetryTimer
    if (!base::MessageLoopCurrent::Get()) {
      g_task_executor = new base::SingleThreadTaskExecutor(base::MessagePumpType::UI);
    }

    self.storagePath = path;
    self.commonOps = [[BATCommonOperations alloc] initWithStoragePath:path];
    adsDatabase = nullptr;

    self.prefsWriteThread = dispatch_queue_create("com.rewards.ads.prefs", DISPATCH_QUEUE_SERIAL);
    self.prefs = [[NSMutableDictionary alloc] initWithContentsOfFile:[self prefsPath]];
    if (!self.prefs) {
      self.prefs = [[NSMutableDictionary alloc] init];
      self.numberOfAllowableAdsPerDay = kDefaultNumberOfAdsPerDay;
      self.numberOfAllowableAdsPerHour = kDefaultNumberOfAdsPerHour;
      self.allowSubdivisionTargeting = false;
      self.subdivisionTargetingCode = @"AUTO";
      self.automaticallyDetectedSubdivisionTargetingCode = @"";
    }

    [self setupNetworkMonitoring];

    // Add notifications for standard app foreground/background
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationDidBecomeActive) name:UIApplicationDidBecomeActiveNotification object:nil];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationDidBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];
  }
  return self;
}

- (void)dealloc
{
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

#pragma mark - Initialization / Shutdown

- (void)initializeIfAdsEnabled
{
  if (![self isAdsServiceRunning] && self.enabled) {
    self.databaseQueue = dispatch_queue_create("com.rewards.ads.db-transactions", DISPATCH_QUEUE_SERIAL);
    
    const auto* dbPath = [self adsDatabasePath].UTF8String;
    adsDatabase = new ads::Database(base::FilePath(dbPath));

    adsClient = new NativeAdsClient(self);
    ads = ads::Ads::CreateInstance(adsClient);
    ads->Initialize(^(bool) { });
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
  return (NSString *)self.prefs[kAdsSubdivisionTargetingCodePrefKey];
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
  return (NSString *)self.prefs[kAutomaticallyDetectedAdsSubdivisionTargetingCodePrefKey];
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

- (NSArray<NSString *> *)userModelLanguages
{
  return NSArrayFromVector([self getUserModelLanguages]);
}

- (void)removeAllHistory:(void (^)(BOOL))completion
{
  if (![self isAdsServiceRunning]) { return; }
  ads->RemoveAllHistory(completion);
}

#pragma mark - Confirmations

- (void)setConfirmationsIsReady:(BOOL)isReady
{
  if (![self isAdsServiceRunning]) { return; }
  ads->SetConfirmationsIsReady(isReady);
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

- (void)reportLoadedPageWithURL:(NSURL *)url innerText:(NSString *)text
{
  if (![self isAdsServiceRunning]) { return; }
  const auto urlString = base::SysNSStringToUTF8(url.absoluteString);
  ads->OnPageLoaded(urlString, base::SysNSStringToUTF8(text));
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
  ads->OnTabUpdated((int32_t)tabId, urlString, isSelected, isPrivate);
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

- (void)confirmAd:(const ads::AdInfo &)info confirmationType:(const ads::ConfirmationType)confirmationType
{
  [self.ledger confirmAd:[NSString stringWithUTF8String:info.ToJson().c_str()]
        confirmationType:[NSString stringWithUTF8String:std::string(confirmationType).c_str()]];
}

- (void)confirmAction:(const std::string &)creative_instance_id creativeSetId:(const std::string &)creative_set_id confirmationType:(const ads::ConfirmationType &)confirmationType
{
  [self.ledger confirmAction:[NSString stringWithUTF8String:creative_instance_id.c_str()]
               creativeSetID:[NSString stringWithUTF8String:creative_set_id.c_str()]
            confirmationType:[NSString stringWithUTF8String:std::string(confirmationType).c_str()]];
}

- (void)setCatalogIssuers:(std::unique_ptr<ads::IssuersInfo>)info
{
  if (![self isAdsServiceRunning]) { return; }
  [self.ledger setCatalogIssuers:[NSString stringWithUTF8String:info->ToJson().c_str()]];
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

- (void)getClientInfo:(ads::ClientInfo *)info
{
  info->platform = ads::ClientInfoPlatformType::IOS;
}

- (const std::vector<std::string>)getUserModelLanguages
{
  std::vector<std::string> languages = { "en", "fr", "de" };
  return languages;
}

- (bool)isAdsEnabled
{
  return self.enabled;
}

- (bool)isForeground
{
  return UIApplication.sharedApplication.applicationState == UIApplicationStateActive;
}

- (bool)canShowBackgroundNotifications
{
    return false;
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

- (void)URLRequest:(const std::string &)url headers:(const std::vector<std::string> &)headers content:(const std::string &)content contentType:(const std::string &)content_type method:(const ads::URLRequestMethod)method callback:(ads::URLRequestCallback)callback {
  std::map<ads::URLRequestMethod, std::string> methodMap {
    {ads::URLRequestMethod::GET, "GET"},
    {ads::URLRequestMethod::POST, "POST"},
    {ads::URLRequestMethod::PUT, "PUT"}
  };
  return [self.commonOps loadURLRequest:url headers:headers content:content content_type:content_type method:methodMap[method] callback:^(int statusCode, const std::string &response, const std::map<std::string, std::string> &headers) {
    callback(statusCode, response, headers);
  }];
}

#pragma mark - File IO

- (void)load:(const std::string &)name callback:(ads::LoadCallback)callback
{
  const auto contents = [self.commonOps loadContentsFromFileWithName:name];
  if (contents.empty()) {
    callback(ads::Result::FAILED, "");
  } else {
    callback(ads::Result::SUCCESS, contents);
  }
}

- (const std::string)loadJsonSchema:(const std::string &)name
{
  const auto bundle = [NSBundle bundleForClass:[BATBraveAds class]];
  const auto path = [bundle pathForResource:[NSString stringWithUTF8String:name.c_str()] ofType:nil];
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

- (void)loadUserModelForLanguage:(const std::string &)language callback:(ads::LoadCallback)callback
{
  const auto bundle = [NSBundle bundleForClass:[BATBraveAds class]];
  const auto languageKey = [[[NSString stringWithUTF8String:language.c_str()] substringToIndex:2] lowercaseString];
  const auto path = [[bundle pathForResource:@"user_models" ofType:nil]
                     stringByAppendingPathComponent:[NSString stringWithFormat:@"languages/%@/user_model.json", languageKey]];
  if (!path || path.length == 0) {
    callback(ads::Result::FAILED, "");
    return;
  }

  NSError *error = nil;
  const auto contents = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:&error];
  if (!contents || error) {
    callback(ads::Result::FAILED, "");
    return;
  }
  callback(ads::Result::SUCCESS, std::string(contents.UTF8String));
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

- (void)runDBTransaction:(ads::DBTransactionPtr)transaction
                callback:(ads::RunDBTransactionCallback)callback
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

@end
