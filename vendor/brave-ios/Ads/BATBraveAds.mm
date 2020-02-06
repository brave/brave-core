/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>

#import "BATBraveAds.h"
#import "BATAdsNotification.h"
#import "BATBraveLedger.h"

#import "bat/ads/ads.h"

#import "NativeAdsClient.h"
#import "NativeAdsClientBridge.h"
#import "CppTransformations.h"
#import "BATCommonOperations.h"
#import "RewardsLogStream.h"

#import <Network/Network.h>
#import <UIKit/UIKit.h>

#import "base/strings/sys_string_conversions.h"

#define BATClassAdsBridge(__type, __objc_getter, __objc_setter, __cpp_var) \
  + (__type)__objc_getter { return ads::__cpp_var; } \
  + (void)__objc_setter:(__type)newValue { ads::__cpp_var = newValue; }

static const NSInteger kDefaultAllowAdConversionTracking = YES;
static const NSInteger kDefaultNumberOfAdsPerDay = 20;
static const NSInteger kDefaultNumberOfAdsPerHour = 2;

static NSString * const kAdsEnabledPrefKey = @"BATAdsEnabled";
static NSString * const kShouldShowPublisherAdsOnParticipatingSitesPrefKey = @"BATkShouldShowPublisherAdsOnParticipatingSites";
static NSString * const kShouldAllowAdConversionTrackingPrefKey = @"BATShouldAllowAdConversionTracking";
static NSString * const kNumberOfAdsPerDayKey = @"BATNumberOfAdsPerDay";
static NSString * const kNumberOfAdsPerHourKey = @"BATNumberOfAdsPerHour";

@interface BATAdsNotification ()
- (instancetype)initWithNotificationInfo:(const ads::AdNotificationInfo&)info;
@end

@interface BATBraveAds () <NativeAdsClientBridge> {
  NativeAdsClient *adsClient;
  ads::Ads *ads;
  std::unique_ptr<ads::BundleState> bundleState;

  nw_path_monitor_t networkMonitor;
  dispatch_queue_t monitorQueue;
}
@property (nonatomic) BATCommonOperations *commonOps;
@property (nonatomic) BOOL networkConnectivityAvailable;
@property (nonatomic, copy) NSString *storagePath;
@property (nonatomic) dispatch_queue_t prefsWriteThread;
@property (nonatomic) NSMutableDictionary *prefs;
@end

@implementation BATBraveAds

- (instancetype)initWithStateStoragePath:(NSString *)path
{
  if ((self = [super init])) {
    self.storagePath = path;
    self.commonOps = [[BATCommonOperations alloc] initWithStoragePath:path];

    self.prefsWriteThread = dispatch_queue_create("com.rewards.ads.prefs", DISPATCH_QUEUE_SERIAL);
    self.prefs = [[NSMutableDictionary alloc] initWithContentsOfFile:[self prefsPath]];
    if (!self.prefs) {
      self.prefs = [[NSMutableDictionary alloc] init];
      self.allowAdConversionTracking = kDefaultAllowAdConversionTracking;
      self.numberOfAllowableAdsPerDay = kDefaultNumberOfAdsPerDay;
      self.numberOfAllowableAdsPerHour = kDefaultNumberOfAdsPerHour;
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
  return ads::Ads::IsSupportedLocale(std::string(locale.UTF8String));
}

+ (BOOL)isNewlySupportedLocale:(NSString *)locale
{
  // TODO(khickinson): Add support for last schema version, however for the MVP
  // we can safely pass 0 as all locales are newly supported
  return ads::Ads::IsNewlySupportedLocale(std::string(locale.UTF8String), 0);
}

+ (BOOL)isCurrentLocaleSupported
{
  return [self isSupportedLocale:[[NSLocale preferredLanguages] firstObject]];
}

BATClassAdsBridge(BOOL, isDebug, setDebug, _is_debug)
BATClassAdsBridge(BOOL, isTesting, setTesting, _is_testing)

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
    adsClient = new NativeAdsClient(self);
    ads = ads::Ads::CreateInstance(adsClient);
    ads->Initialize(^(bool) { });
  }
}

- (void)shutdown
{
  if ([self isAdsServiceRunning]) {
    ads->Shutdown(^(bool) {
      delete ads;
      delete adsClient;
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
  return [(NSNumber *)self.prefs[kAdsEnabledPrefKey] boolValue];
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

- (BOOL)shouldShowPublisherAdsOnParticipatingSites
{
  return [(NSNumber *)self.prefs[kShouldShowPublisherAdsOnParticipatingSitesPrefKey] boolValue];
}

- (void)setShowPublisherAdsOnParticipatingSites:(BOOL)shouldShowPublisherAdsOnParticipatingSites
{
  self.prefs[kShouldShowPublisherAdsOnParticipatingSitesPrefKey] = @(shouldShowPublisherAdsOnParticipatingSites);
  [self savePrefs];
}

- (BOOL)shouldAllowAdConversionTracking
{
  return [(NSNumber *)self.prefs[kShouldAllowAdConversionTrackingPrefKey] boolValue];
}

- (void)setAllowAdConversionTracking:(BOOL)shouldAllowAdConversionTracking
{
  self.prefs[kShouldAllowAdConversionTrackingPrefKey] = @(shouldAllowAdConversionTracking);
  [self savePrefs];
}

- (NSInteger)numberOfAllowableAdsPerDay
{
  return [(NSNumber *)self.prefs[kNumberOfAdsPerDayKey] integerValue];
}

- (void)setNumberOfAllowableAdsPerDay:(NSInteger)numberOfAllowableAdsPerDay
{
  self.prefs[kNumberOfAdsPerDayKey] = @(numberOfAllowableAdsPerDay);
  [self savePrefs];
}

- (NSInteger)numberOfAllowableAdsPerHour
{
  return [(NSNumber *)self.prefs[kNumberOfAdsPerHourKey] integerValue];
}

- (void)setNumberOfAllowableAdsPerHour:(NSInteger)numberOfAllowableAdsPerHour
{
  self.prefs[kNumberOfAdsPerHourKey] = @(numberOfAllowableAdsPerHour);
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

- (void)serveSampleAd
{
  if (![self isAdsServiceRunning]) { return; }
  ads->ServeSampleAd();
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

- (void)reportAdNotificationEvent:(NSString *)notificationId eventType:(BATAdsAdNotificationEventType)eventType
{
  if (![self isAdsServiceRunning]) { return; }
  ads->OnAdNotificationEvent(notificationId.UTF8String,
                             static_cast<ads::AdNotificationEventType>(eventType));
}

- (void)reportPublisherAdEvent:(NSString *)notificationId eventType:(BATAdsPublisherAdEventType)eventType
{
  if (![self isAdsServiceRunning]) { return; }
  ads->OnPublisherAdEvent(notificationId.UTF8String,
                          static_cast<ads::PublisherAdEventType>(eventType));
}

- (void)toggleThumbsUpForAd:(NSString *)identifier creativeSetID:(NSString *)creativeSetID
{
  if (![self isAdsServiceRunning]) { return; }
  ads->ToggleAdThumbUp(identifier.UTF8String,
                       creativeSetID.UTF8String,
                       ads::AdContent::LikeAction::LIKE_ACTION_THUMBS_UP);
}

- (void)toggleThumbsDownForAd:(NSString *)identifier creativeSetID:(NSString *)creativeSetID
{
  if (![self isAdsServiceRunning]) { return; }
  ads->ToggleAdThumbDown(identifier.UTF8String,
                         creativeSetID.UTF8String,
                         ads::AdContent::LikeAction::LIKE_ACTION_THUMBS_DOWN);
}

- (void)confirmAdNotification:(const ads::AdNotificationInfo &)info
{
  [self.ledger confirmAdNotification:[NSString stringWithUTF8String:info.ToJson().c_str()]];
}

- (void)confirmPublisherAd:(const ads::PublisherAdInfo &)info
{
  [self.ledger confirmPublisherAd:[NSString stringWithUTF8String:info.ToJson().c_str()]];
}

- (void)confirmAction:(const std::string &)uuid creativeSetId:(const std::string &)creative_set_id confirmationType:(const ads::ConfirmationType &)type
{
  [self.ledger confirmAction:[NSString stringWithUTF8String:uuid.c_str()]
               creativeSetID:[NSString stringWithUTF8String:creative_set_id.c_str()]
                        type:[NSString stringWithUTF8String:std::string(type).c_str()]];
}

- (void)getCreativeAdNotifications:(const std::vector<std::string> &)categories callback:(ads::OnGetCreativeAdNotificationsCallback)callback
{
  if (![self isAdsServiceRunning]) { return; }

  ads::CreativeAdNotifications found_ads;
  for (const auto & category : categories) {
    auto it = bundleState->categories.find(category);
    if (it == bundleState->categories.end()) {
      continue;
    }

    found_ads.insert(found_ads.end(), it->second.begin(), it->second.end());
  }

  callback(ads::Result::SUCCESS, categories, found_ads);
}

- (void)getCreativePublisherAds:(const std::string &)url categories:(const std::vector<std::string> &)categories sizes:(const std::vector<std::string> &)sizes callback:(ads::OnGetCreativePublisherAdsCallback)callback
{
  // TODO(brave): To be implemented
}

- (void)getAdConversions:(const std::string &)url callback:(ads::OnGetAdConversionsCallback)callback
{
  // TODO(khickinson): To be implemented
  if (![self isAdsServiceRunning]) { return; }

  callback(ads::Result::SUCCESS, url, {});
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

- (const std::string)getLocale
{
  return std::string([[NSLocale preferredLanguages] firstObject].UTF8String);
}

- (bool)isAdsEnabled
{
  return self.enabled;
}

- (bool)shouldShowPublisherAdsOnParticipatingSites
{
  return self.shouldShowPublisherAdsOnParticipatingSites;
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

#pragma mark - Timers

- (uint32_t)setTimer:(const uint64_t)time_offset
{
  auto __weak weakSelf = self;
  return [self.commonOps createTimerWithOffset:time_offset timerFired:^(uint32_t timer_id) {
    auto strongSelf = weakSelf;
    // If this object dies, common will get nil'd out
    if (strongSelf) {
      if ([strongSelf isAdsServiceRunning]) {
        strongSelf->ads->OnTimer(timer_id);
      }
      [strongSelf.commonOps removeTimerWithID:timer_id];
    }
  }];
}

- (void)killTimer:(uint32_t)timer_id
{
  [self.commonOps removeTimerWithID:timer_id];
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

- (void)load:(const std::string &)name callback:(ads::OnLoadCallback)callback
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

- (void)loadSampleBundle:(ads::OnLoadSampleBundleCallback)callback
{
  const auto bundle = [NSBundle bundleForClass:[BATBraveAds class]];
  const auto path = [bundle pathForResource:@"sample_bundle" ofType:@"json"];
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

- (void)loadUserModelForLanguage:(const std::string &)language callback:(ads::OnLoadCallback)callback
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

- (void)reset:(const std::string &)name callback:(ads::OnResetCallback)callback
{
  if ([self.commonOps removeFileWithName:name]) {
    callback(ads::Result::SUCCESS);
  } else {
    callback(ads::Result::FAILED);
  }
}

- (void)save:(const std::string &)name value:(const std::string &)value callback:(ads::OnSaveCallback)callback
{
  if ([self.commonOps saveContents:value name:name]) {
    callback(ads::Result::SUCCESS);
  } else {
    callback(ads::Result::FAILED);
  }
}

- (void)saveBundleState:(std::unique_ptr<ads::BundleState>)state callback:(ads::OnSaveCallback)callback
{
  if (state.get() == nullptr) {
    callback(ads::Result::FAILED);
    return;
  }
  bundleState.reset(state.release());
  if ([self.commonOps saveContents:bundleState->ToJson() name:"bundle.json"]) {
    callback(ads::Result::SUCCESS);
  } else {
    callback(ads::Result::FAILED);
  }
}

#pragma mark - Logging

- (void)eventLog:(const std::string &)json
{
  [self log:__FILE__ line:__LINE__ logLevel:ads::LogLevel::LOG_INFO]->stream() << json << std::endl;
}

- (std::unique_ptr<ads::LogStream>)log:(const char *)file line:(const int)line logLevel:(const ads::LogLevel)log_level
{
  return std::make_unique<RewardsLogStream>(file, line, log_level);
}

#pragma mark - Notifications

- (nullable BATAdsNotification *)adsNotificationForIdentifier:(NSString *)identifier
{
  if (![self isAdsServiceRunning]) { return nil; }
  ads::AdNotificationInfo info;
  if (ads->GetAdNotificationForId(identifier.UTF8String, &info)) {
    return [[BATAdsNotification alloc] initWithNotificationInfo:info];
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
  const auto notification = [[BATAdsNotification alloc] initWithNotificationInfo:*info];
  [self.notificationsHandler showNotification:notification];
}

- (void)closeNotification:(const std::string &)id
{
  const auto bridgedId = [NSString stringWithUTF8String:id.c_str()];
  [self.notificationsHandler clearNotificationWithIdentifier:bridgedId];
}

@end
