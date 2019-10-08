/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

#define BATClassAdsBridge(__type, __objc_getter, __objc_setter, __cpp_var) \
  + (__type)__objc_getter { return ads::__cpp_var; } \
  + (void)__objc_setter:(__type)newValue { ads::__cpp_var = newValue; }

static const NSInteger kDefaultNumberOfAdsPerDay = 20;
static const NSInteger kDefaultNumberOfAdsPerHour = 2;

static NSString * const kAdsEnabledPrefKey = @"BATAdsEnabled";
static NSString * const kNumberOfAdsPerDayKey = @"BATNumberOfAdsPerDay";
static NSString * const kNumberOfAdsPerHourKey = @"BATNumberOfAdsPerHour";

@interface BATAdsNotification ()
- (instancetype)initWithNotificationInfo:(const ads::NotificationInfo&)info;
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
      self.numberOfAllowableAdsPerDay = kDefaultNumberOfAdsPerDay;
      self.numberOfAllowableAdsPerHour = kDefaultNumberOfAdsPerHour;
      self.enabled = YES;
    }

    [self setupNetworkMonitoring];

    adsClient = new NativeAdsClient(self);
    ads = ads::Ads::CreateInstance(adsClient);
    ads->Initialize(^(bool) { });

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
  delete ads;
  delete adsClient;
}

- (NSString *)prefsPath
{
  return [self.storagePath stringByAppendingPathComponent:@"ads_pref.plist"];
}

#pragma mark - Global

+ (BOOL)isSupportedRegion:(NSString *)region
{
  return ads::Ads::IsSupportedRegion(std::string(region.UTF8String));
}

BATClassAdsBridge(BOOL, isDebug, setDebug, _is_debug)
BATClassAdsBridge(BOOL, isTesting, setTesting, _is_testing)
BATClassAdsBridge(BOOL, isProduction, setProduction, _is_production)

#pragma mark - Configuration

- (BOOL)isEnabled
{
  return [(NSNumber *)self.prefs[kAdsEnabledPrefKey] boolValue];
}

- (void)setEnabled:(BOOL)enabled
{
  self.prefs[kAdsEnabledPrefKey] = @(enabled);
  [self savePrefs];
  if (ads != nil) {
    if (enabled) {
      ads->Initialize(^(bool) { });
    } else {
      ads->Shutdown(^(bool) { });
    }
  }
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
  ads->RemoveAllHistory(completion);
}

- (void)serveSampleAd
{
  ads->ServeSampleAd();
}

#pragma mark - Confirmations

- (void)setConfirmationsIsReady:(BOOL)isReady
{
  ads->SetConfirmationsIsReady(isReady);
}

#pragma mark - Observers

- (void)applicationDidBecomeActive
{
  ads->OnForeground();
}

- (void)applicationDidBackground
{
  ads->OnBackground();
}

#pragma mark - Reporting

- (void)reportLoadedPageWithURL:(NSURL *)url html:(NSString *)html
{
  const auto urlString = std::string(url.absoluteString.UTF8String);
  ads->OnPageLoaded(urlString, std::string(html.UTF8String));
}

- (void)reportMediaStartedWithTabId:(NSInteger)tabId
{
  ads->OnMediaPlaying((int32_t)tabId);
}

- (void)reportMediaStoppedWithTabId:(NSInteger)tabId
{
  ads->OnMediaStopped((int32_t)tabId);
}

- (void)reportTabUpdated:(NSInteger)tabId url:(NSURL *)url isSelected:(BOOL)isSelected isPrivate:(BOOL)isPrivate
{
  const auto urlString = std::string(url.absoluteString.UTF8String);
  ads->OnTabUpdated((int32_t)tabId, urlString, isSelected, isPrivate);
}

- (void)reportTabClosedWithTabId:(NSInteger)tabId
{
  ads->OnTabClosed((int32_t)tabId);
}

- (void)reportNotificationEvent:(NSString *)notificationId eventType:(BATAdsNotificationEventType)eventType
{
  ads->OnNotificationEvent(notificationId.UTF8String,
                           static_cast<ads::NotificationEventType>(eventType));
}

- (void)toggleThumbsUpForAd:(NSString *)identifier creativeSetID:(NSString *)creativeSetID
{
  ads->ToggleAdThumbUp(identifier.UTF8String,
                       creativeSetID.UTF8String,
                       ads::AdContent::LikeAction::LIKE_ACTION_THUMBS_UP);
}


- (void)toggleThumbsDownForAd:(NSString *)identifier creativeSetID:(NSString *)creativeSetID
{
  ads->ToggleAdThumbDown(identifier.UTF8String,
                         creativeSetID.UTF8String,
                         ads::AdContent::LikeAction::LIKE_ACTION_THUMBS_DOWN);
}

- (void)confirmAd:(std::unique_ptr<ads::NotificationInfo>)info
{
  [self.ledger confirmAd:[NSString stringWithUTF8String:info->ToJson().c_str()]];
}

- (void)confirmAction:(const std::string &)uuid creativeSetId:(const std::string &)creative_set_id confirmationType:(const ads::ConfirmationType &)type
{
  [self.ledger confirmAction:[NSString stringWithUTF8String:uuid.c_str()]
               creativeSetID:[NSString stringWithUTF8String:creative_set_id.c_str()]
                        type:[NSString stringWithUTF8String:std::string(type).c_str()]];
}

- (void)getAds:(const std::string &)category callback:(ads::OnGetAdsCallback)callback
{
  auto categories = bundleState->categories.find(category);
  if (categories == bundleState->categories.end()) {
    callback(ads::Result::FAILED, category, {});
    return;
  }

  callback(ads::Result::SUCCESS, category, categories->second);
}

- (void)setCatalogIssuers:(std::unique_ptr<ads::IssuersInfo>)info
{
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
      strongSelf->ads->OnTimer(timer_id);
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
  ads::NotificationInfo info;
  if (ads->GetNotificationForId(identifier.UTF8String, &info)) {
    return [[BATAdsNotification alloc] initWithNotificationInfo:info];
  }
  return nil;
}

- (bool)shouldShowNotifications
{
  return [self.notificationsHandler shouldShowNotifications];
}

- (void)showNotification:(std::unique_ptr<ads::NotificationInfo>)info
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
