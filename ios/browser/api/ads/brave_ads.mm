/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <Network/Network.h>
#import <UIKit/UIKit.h>

#include "base/base64.h"
#include "base/containers/flat_map.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/values.h"
#import "brave/build/ios/mojom/cpp_transformations.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_util.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/public/database/database.h"
#include "brave/components/brave_ads/core/public/flags/flags_util.h"
#include "brave/components/brave_ads/core/public/history/ad_content_info.h"
#include "brave/components/brave_ads/core/public/history/ad_content_value_util.h"
#include "brave/components/brave_ads/core/public/history/history_filter_types.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "brave/components/brave_ads/core/public/history/history_sort_types.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/user/user_interaction/ad_events/ad_event_cache.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/l10n/common/prefs.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#import "brave/ios/browser/api/ads/ads_client_bridge.h"
#import "brave/ios/browser/api/ads/ads_client_ios.h"
#import "brave/ios/browser/api/ads/brave_ads.h"
#import "brave/ios/browser/api/ads/brave_ads.mojom.objc+private.h"
#import "brave/ios/browser/api/ads/inline_content_ad_ios.h"
#import "brave/ios/browser/api/ads/notification_ad_ios.h"
#import "brave/ios/browser/api/common/common_operations.h"
#include "build/build_config.h"
#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#define BLOG(verbose_level, format, ...)                  \
  [self log:(__FILE__)                                    \
       line:(__LINE__)verboseLevel:(verbose_level)message \
           :base::SysNSStringToUTF8(                      \
                [NSString stringWithFormat:(format), ##__VA_ARGS__])]

#define BATClassAdsBridge(__type, __objc_getter, __objc_setter, __cpp_var) \
  +(__type)__objc_getter {                                                 \
    return brave_ads::__cpp_var;                                           \
  }                                                                        \
  +(void)__objc_setter : (__type)newValue {                                \
    brave_ads::__cpp_var = newValue;                                       \
  }

static const NSInteger kDefaultNumberOfAdsPerHour = 2;

static const int kCurrentAdsResourceManifestSchemaVersion = 1;

static NSString* const kLegacyOptedInToNotificationAdsPrefKey =
    @"BATAdsEnabled";
static NSString* const kLegacyNumberOfAdsPerHourKey = @"BATNumberOfAdsPerHour";
static NSString* const kLegacyShouldAllowAdsSubdivisionTargetingPrefKey =
    @"BATShouldAllowAdsSubdivisionTargetingPrefKey";
static NSString* const kLegacyAdsSubdivisionTargetingCodePrefKey =
    @"BATAdsSubdivisionTargetingCodePrefKey";
static NSString* const kLegacyAutoDetectedAdsSubdivisionTargetingCodePrefKey =
    @"BATAutoDetectedAdsSubdivisionTargetingCodePrefKey";

static NSString* const kOptedInToNotificationAdsPrefKey =
    base::SysUTF8ToNSString(brave_ads::prefs::kOptedInToNotificationAds);
static NSString* const kRewardsEnabledPrefKey =
    base::SysUTF8ToNSString(brave_rewards::prefs::kEnabled);
static NSString* const kMaximumNotificationAdsPerHourPrefKey =
    base::SysUTF8ToNSString(brave_ads::prefs::kMaximumNotificationAdsPerHour);
static NSString* const kShouldAllowSubdivisionTargetingPrefKey =
    base::SysUTF8ToNSString(brave_ads::prefs::kShouldAllowSubdivisionTargeting);
static NSString* const kSubdivisionTargetingSubdivisionPrefKey =
    base::SysUTF8ToNSString(brave_ads::prefs::kSubdivisionTargetingSubdivision);
static NSString* const kSubdivisionTargetingAutoDetectedSubdivisionPrefKey =
    base::SysUTF8ToNSString(
        brave_ads::prefs::kSubdivisionTargetingAutoDetectedSubdivision);
static NSString* const kAdsResourceMetadataPrefKey = @"BATAdsResourceMetadata";

namespace {

brave_ads::mojom::DBCommandResponseInfoPtr RunDBTransactionOnTaskRunner(
    brave_ads::mojom::DBTransactionInfoPtr transaction,
    brave_ads::Database* database) {
  auto response = brave_ads::mojom::DBCommandResponseInfo::New();
  if (!database) {
    response->status =
        brave_ads::mojom::DBCommandResponseInfo::StatusType::RESPONSE_ERROR;
  } else {
    database->RunTransaction(std::move(transaction), response.get());
  }

  return response;
}

}  // namespace

@interface NotificationAdIOS ()
- (instancetype)initWithNotificationInfo:
    (const brave_ads::NotificationAdInfo&)info;
@end

@interface InlineContentAdIOS ()
- (instancetype)initWithInlineContentAdInfo:
    (const brave_ads::InlineContentAdInfo&)info;
@end

@interface BraveAds () <AdsClientBridge> {
  AdsClientIOS* adsClient;
  brave_ads::AdsClientNotifier* adsClientNotifier;
  brave_ads::Ads* ads;
  brave_ads::Database* adsDatabase;
  brave_ads::AdEventCache* adEventCache;
  scoped_refptr<base::SequencedTaskRunner> databaseQueue;

  nw_path_monitor_t networkMonitor;
  dispatch_queue_t monitorQueue;
}
@property(nonatomic) BraveCommonOperations* commonOps;
@property(nonatomic) BOOL networkConnectivityAvailable;
@property(nonatomic, copy) NSString* storagePath;
@property(nonatomic) dispatch_group_t prefsWriteGroup;
@property(nonatomic) dispatch_queue_t prefsWriteThread;
@property(nonatomic) NSMutableDictionary* prefs;
@property(nonatomic, copy) NSDictionary* adsResourceMetadata;
@property(nonatomic) NSTimer* updateAdsResourceTimer;
@property(nonatomic) int64_t adsResourceRetryCount;
@property(nonatomic, readonly) NSDictionary* componentPaths;
@end

@implementation BraveAds

- (instancetype)initWithStateStoragePath:(NSString*)path {
  if ((self = [super init])) {
    self.storagePath = path;
    self.commonOps = [[BraveCommonOperations alloc] initWithStoragePath:path];
    adsDatabase = nullptr;
    adEventCache = nullptr;

    self.prefsWriteThread =
        dispatch_queue_create("com.rewards.ads.prefs", DISPATCH_QUEUE_SERIAL);
    self.prefsWriteGroup = dispatch_group_create();
    self.prefs =
        [[NSMutableDictionary alloc] initWithContentsOfFile:[self prefsPath]];
    if (!self.prefs) {
      self.prefs = [[NSMutableDictionary alloc] init];
      self.numberOfAllowableAdsPerHour = kDefaultNumberOfAdsPerHour;
    } else {
      [self migratePrefs];
    }

    [self setupNetworkMonitoring];

    if (self.adsResourceMetadata == nil) {
      self.adsResourceMetadata = [[NSDictionary alloc] init];
    }

    self.adsResourceRetryCount = 1;

    databaseQueue = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

    // Add notifications for standard app foreground/background
    [NSNotificationCenter.defaultCenter
        addObserver:self
           selector:@selector(applicationDidBecomeActive)
               name:UIApplicationDidBecomeActiveNotification
             object:nil];
    [NSNotificationCenter.defaultCenter
        addObserver:self
           selector:@selector(applicationDidBackground)
               name:UIApplicationDidEnterBackgroundNotification
             object:nil];

    const auto dbPath = base::SysNSStringToUTF8([self adsDatabasePath]);
    adsDatabase = new brave_ads::Database(base::FilePath(dbPath));

    adEventCache = new brave_ads::AdEventCache();

    adsClient = new AdsClientIOS(self);
  }
  return self;
}

- (void)dealloc {
  [self.updateAdsResourceTimer invalidate];
  self.updateAdsResourceTimer = nil;

  [NSNotificationCenter.defaultCenter removeObserver:self];
  if (networkMonitor) {
    nw_path_monitor_cancel(networkMonitor);
  }

  if (adsDatabase) {
    databaseQueue->DeleteSoon(FROM_HERE, adsDatabase);
  }

  if (ads != nil) {
    delete ads;
  }
  if (adsClientNotifier != nil) {
    delete adsClientNotifier;
  }
  if (adsClient != nil) {
    delete adsClient;
  }
  if (adEventCache != nil) {
    delete adEventCache;
  }
  ads = nil;
  adsClientNotifier = nil;
  adsClient = nil;
  adEventCache = nil;
}

- (NSString*)prefsPath {
  return [self.storagePath stringByAppendingPathComponent:@"ads_pref.plist"];
}

#pragma mark - Global

+ (BOOL)isSupportedRegion {
  return brave_ads::IsSupportedRegion();
}

#pragma mark - Initialization / Shutdown

- (void)initializeWithSysInfo:(BraveAdsSysInfo*)sysInfo
             buildChannelInfo:(BraveAdsBuildChannelInfo*)buildChannelInfo
                   walletInfo:(nullable BraveAdsWalletInfo*)walletInfo
                   completion:(void (^)(bool))completion {
  if ([self isAdsServiceRunning]) {
    completion(false);
    return;
  }

  adsClientNotifier = new brave_ads::AdsClientNotifier();
  ads = brave_ads::Ads::CreateInstance(adsClient);

  auto cppSysInfo =
      sysInfo ? sysInfo.cppObjPtr : brave_ads::mojom::SysInfo::New();
  ads->SetSysInfo(std::move(cppSysInfo));
  auto cppBuildChannelInfo = buildChannelInfo
                                 ? buildChannelInfo.cppObjPtr
                                 : brave_ads::mojom::BuildChannelInfo::New();
  ads->SetBuildChannel(std::move(cppBuildChannelInfo));
  ads->SetFlags(brave_ads::BuildFlags());

  auto cppWalletInfo =
      walletInfo ? walletInfo.cppObjPtr : brave_ads::mojom::WalletInfoPtr();
  ads->Initialize(std::move(cppWalletInfo),
                  base::BindOnce(^(const bool success) {
                    [self periodicallyCheckForAdsResourceUpdates];
                    [self registerAdsResources];
                    if (success && self->adsClientNotifier != nil) {
                      self->adsClientNotifier->NotifyDidInitializeAds();
                    }
                    completion(success);
                    if (!success) {
                      if (self->ads != nil) {
                        delete self->ads;
                      }
                      if (self->adsClientNotifier != nil) {
                        delete self->adsClientNotifier;
                      }
                      self->ads = nil;
                      self->adsClientNotifier = nil;
                    }
                  }));
}

- (void)updateWalletInfo:(NSString*)paymentId base64Seed:(NSString*)base64Seed {
  if (![self isAdsServiceRunning]) {
    return;
  }
  adsClientNotifier->NotifyRewardsWalletDidUpdate(
      base::SysNSStringToUTF8(paymentId), base::SysNSStringToUTF8(base64Seed));
}

- (NSString*)adsDatabasePath {
  return [self.storagePath stringByAppendingPathComponent:@"Ads.db"];
}

- (void)resetAdsDatabase {
  delete adsDatabase;
  const auto dbPath = [self adsDatabasePath];
  [NSFileManager.defaultManager removeItemAtPath:dbPath error:nil];
  [NSFileManager.defaultManager
      removeItemAtPath:[dbPath stringByAppendingString:@"-journal"]
                 error:nil];
  adsDatabase =
      new brave_ads::Database(base::FilePath(base::SysNSStringToUTF8(dbPath)));
}

- (void)shutdown:(nullable void (^)())completion {
  if ([self isAdsServiceRunning]) {
    dispatch_group_notify(self.prefsWriteGroup, dispatch_get_main_queue(), ^{
      self->ads->Shutdown(base::BindOnce(^(bool) {
        if (self->ads != nil) {
          delete self->ads;
        }
        if (self->adsClientNotifier != nil) {
          delete self->adsClientNotifier;
        }
        if (self->adsClient != nil) {
          delete self->adsClient;
        }
        if (self->adsDatabase != nil) {
          self->databaseQueue->PostTask(
              FROM_HERE,
              base::BindOnce(
                  [](brave_ads::Database* database) { delete database; },
                  self->adsDatabase));
        }
        if (self->adEventCache != nil) {
          delete self->adEventCache;
        }
        self->ads = nil;
        self->adsClientNotifier = nil;
        self->adsClient = nil;
        self->adsDatabase = nil;
        self->adEventCache = nil;
        if (completion) {
          completion();
        }
      }));
    });
  } else {
    if (completion) {
      completion();
    }
  }
}

- (BOOL)isAdsServiceRunning {
  return ads != nil && adsClientNotifier != nil;
}

#pragma mark - Configuration

- (BOOL)isEnabled {
  return [self.prefs[kRewardsEnabledPrefKey] boolValue];
}

- (void)setEnabled:(BOOL)enabled {
  self.prefs[kRewardsEnabledPrefKey] = @(enabled);
  [self savePref:kRewardsEnabledPrefKey];
  self.prefs[kOptedInToNotificationAdsPrefKey] = @(enabled);
  [self savePref:kOptedInToNotificationAdsPrefKey];
}

- (NSInteger)numberOfAllowableAdsPerHour {
  return [self.prefs[kMaximumNotificationAdsPerHourPrefKey] integerValue];
}

- (void)setNumberOfAllowableAdsPerHour:(NSInteger)numberOfAllowableAdsPerHour {
  self.prefs[kMaximumNotificationAdsPerHourPrefKey] =
      @(numberOfAllowableAdsPerHour);
  [self savePref:kMaximumNotificationAdsPerHourPrefKey];
}

- (BOOL)shouldAllowSubdivisionTargeting {
  return [self.prefs[kShouldAllowSubdivisionTargetingPrefKey] boolValue];
}

- (void)setAllowSubdivisionTargeting:(BOOL)allowAdsSubdivisionTargeting {
  self.prefs[kShouldAllowSubdivisionTargetingPrefKey] =
      @(allowAdsSubdivisionTargeting);
  [self savePref:kShouldAllowSubdivisionTargetingPrefKey];
}

- (NSString*)subdivisionTargetingCode {
  return (NSString*)self.prefs[kSubdivisionTargetingSubdivisionPrefKey]
             ?: @"AUTO";
}

- (void)setSubdivisionTargetingCode:(NSString*)subdivisionTargetingCode {
  self.prefs[kSubdivisionTargetingSubdivisionPrefKey] =
      subdivisionTargetingCode;
  [self savePref:kSubdivisionTargetingSubdivisionPrefKey];
}

- (NSString*)autoDetectedSubdivisionTargetingCode {
  return (NSString*)
                 self.prefs[kSubdivisionTargetingAutoDetectedSubdivisionPrefKey]
             ?: @"";
}

- (void)setAutoDetectedSubdivisionTargetingCode:
    (NSString*)autoDetectedSubdivisionTargetingCode {
  self.prefs[kSubdivisionTargetingAutoDetectedSubdivisionPrefKey] =
      autoDetectedSubdivisionTargetingCode;
  [self savePref:kSubdivisionTargetingAutoDetectedSubdivisionPrefKey];
}

- (void)savePref:(NSString*)name {
  [self savePrefs];

  if ([self isAdsServiceRunning]) {
    adsClientNotifier->NotifyPrefDidChange(base::SysNSStringToUTF8(name));
  }
}

- (void)savePrefs {
  NSDictionary* prefs = [self.prefs copy];
  NSString* path = [[self prefsPath] copy];
  dispatch_group_enter(self.prefsWriteGroup);
  dispatch_async(self.prefsWriteThread, ^{
    [prefs writeToURL:[NSURL fileURLWithPath:path isDirectory:NO] error:nil];
    dispatch_group_leave(self.prefsWriteGroup);
  });
}

#pragma mark -

- (void)migratePrefs {
  if ([self.prefs objectForKey:kLegacyOptedInToNotificationAdsPrefKey]) {
    self.prefs[kOptedInToNotificationAdsPrefKey] =
        self.prefs[kLegacyOptedInToNotificationAdsPrefKey];
    [self.prefs removeObjectForKey:kLegacyOptedInToNotificationAdsPrefKey];
  }

  if (![self.prefs objectForKey:kRewardsEnabledPrefKey] &&
      [self.prefs objectForKey:kOptedInToNotificationAdsPrefKey]) {
    self.prefs[kRewardsEnabledPrefKey] =
        self.prefs[kOptedInToNotificationAdsPrefKey];
  }

  if ([self.prefs objectForKey:kLegacyNumberOfAdsPerHourKey]) {
    self.prefs[kMaximumNotificationAdsPerHourPrefKey] =
        self.prefs[kLegacyNumberOfAdsPerHourKey];
    [self.prefs removeObjectForKey:kLegacyNumberOfAdsPerHourKey];
  }

  if ([self.prefs
          objectForKey:kLegacyShouldAllowAdsSubdivisionTargetingPrefKey]) {
    self.prefs[kShouldAllowSubdivisionTargetingPrefKey] =
        self.prefs[kLegacyShouldAllowAdsSubdivisionTargetingPrefKey];
    [self.prefs
        removeObjectForKey:kLegacyShouldAllowAdsSubdivisionTargetingPrefKey];
  }

  if ([self.prefs objectForKey:kLegacyAdsSubdivisionTargetingCodePrefKey]) {
    self.prefs[kSubdivisionTargetingSubdivisionPrefKey] =
        self.prefs[kLegacyAdsSubdivisionTargetingCodePrefKey];
    [self.prefs removeObjectForKey:kLegacyAdsSubdivisionTargetingCodePrefKey];
  }

  if ([self.prefs
          objectForKey:kLegacyAutoDetectedAdsSubdivisionTargetingCodePrefKey]) {
    self.prefs[kSubdivisionTargetingAutoDetectedSubdivisionPrefKey] =
        self.prefs[kLegacyAutoDetectedAdsSubdivisionTargetingCodePrefKey];
    [self.prefs removeObjectForKey:
                    kLegacyAutoDetectedAdsSubdivisionTargetingCodePrefKey];
  }

  [self savePrefs];
}

- (void)setupNetworkMonitoring {
  auto const __weak weakSelf = self;

  monitorQueue = dispatch_queue_create("bat.nw.monitor", DISPATCH_QUEUE_SERIAL);
  networkMonitor = nw_path_monitor_create();
  nw_path_monitor_set_queue(networkMonitor, monitorQueue);
  nw_path_monitor_set_update_handler(
      networkMonitor, ^(nw_path_t _Nonnull path) {
        const auto strongSelf = weakSelf;
        if (!strongSelf) {
          return;
        }
        strongSelf.networkConnectivityAvailable =
            (nw_path_get_status(path) == nw_path_status_satisfied ||
             nw_path_get_status(path) == nw_path_status_satisfiable);
      });
  nw_path_monitor_start(networkMonitor);
}

#pragma mark - Observers

- (void)addObserver:(brave_ads::AdsClientNotifierObserver*)observer {
  if (adsClientNotifier != nil) {
    adsClientNotifier->AddObserver(observer);
  }
}

- (void)removeObserver:(brave_ads::AdsClientNotifierObserver*)observer {
  if (adsClientNotifier != nil) {
    adsClientNotifier->RemoveObserver(observer);
  }
}

- (void)notifyPendingObservers {
  if (adsClientNotifier != nil) {
    adsClientNotifier->NotifyPendingObservers();
  }
}

- (void)applicationDidBecomeActive {
  if (![self isAdsServiceRunning]) {
    return;
  }
  adsClientNotifier->NotifyBrowserDidEnterForeground();
  adsClientNotifier->NotifyBrowserDidBecomeActive();
}

- (void)applicationDidBackground {
  if (![self isAdsServiceRunning]) {
    return;
  }
  adsClientNotifier->NotifyBrowserDidResignActive();
  adsClientNotifier->NotifyBrowserDidEnterBackground();
}

#pragma mark - History

- (NSArray<NSDate*>*)getAdsHistoryDates {
  if (![self isAdsServiceRunning]) {
    return @[];
  }

  const auto history_items = ads->GetHistory(
      brave_ads::HistoryFilterType::kNone, brave_ads::HistorySortType::kNone,
      base::Time::Min(), base::Time::Max());

  const auto dates = [[NSMutableArray<NSDate*> alloc] init];
  for (const auto& history_item : history_items) {
    const auto date = [NSDate
        dateWithTimeIntervalSince1970:history_item.created_at.ToDoubleT()];
    [dates addObject:date];
  }

  return dates;
}

- (BOOL)hasViewedAdsInPreviousCycle {
  const auto calendar =
      [NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian];
  const auto now = NSDate.date;
  const auto previousCycleDate = [calendar dateByAddingUnit:NSCalendarUnitMonth
                                                      value:-1
                                                     toDate:now
                                                    options:0];
  const auto previousCycleMonth = [calendar component:NSCalendarUnitMonth
                                             fromDate:previousCycleDate];
  const auto previousCycleYear = [calendar component:NSCalendarUnitYear
                                            fromDate:previousCycleDate];
  const auto viewedDates = [self getAdsHistoryDates];
  for (NSDate* date in viewedDates) {
    const auto components =
        [calendar components:NSCalendarUnitMonth | NSCalendarUnitYear
                    fromDate:date];
    if (components.month == previousCycleMonth &&
        components.year == previousCycleYear) {
      // Was from previous cycle
      return YES;
    }
  }
  return NO;
}

#pragma mark - Reporting

- (void)reportLoadedPageWithURL:(NSURL*)url
             redirectedFromURLs:(NSArray<NSURL*>*)redirectionURLs
                           html:(NSString*)html
                      innerText:(NSString*)text
                          tabId:(NSInteger)tabId {
  if (![self isAdsServiceRunning]) {
    return;
  }
  std::vector<GURL> urls;
  for (NSURL* redirectURL in redirectionURLs) {
    urls.push_back(net::GURLWithNSURL(redirectURL));
  }
  urls.push_back(net::GURLWithNSURL(url));
  adsClientNotifier->NotifyTabTextContentDidChange(
      (int32_t)tabId, urls, base::SysNSStringToUTF8(text));
  adsClientNotifier->NotifyTabHtmlContentDidChange(
      (int32_t)tabId, urls, base::SysNSStringToUTF8(html));
}

- (void)reportMediaStartedWithTabId:(NSInteger)tabId {
  if (![self isAdsServiceRunning]) {
    return;
  }
  adsClientNotifier->NotifyTabDidStartPlayingMedia((int32_t)tabId);
}

- (void)reportMediaStoppedWithTabId:(NSInteger)tabId {
  if (![self isAdsServiceRunning]) {
    return;
  }
  adsClientNotifier->NotifyTabDidStopPlayingMedia((int32_t)tabId);
}

- (void)reportTabUpdated:(NSInteger)tabId
                     url:(NSURL*)url
      redirectedFromURLs:(NSArray<NSURL*>*)redirectionURLs
              isSelected:(BOOL)isSelected {
  if (![self isAdsServiceRunning]) {
    return;
  }
  std::vector<GURL> urls;
  for (NSURL* redirectURL in redirectionURLs) {
    urls.push_back(net::GURLWithNSURL(redirectURL));
  }
  urls.push_back(net::GURLWithNSURL(url));
  const bool isVisible = isSelected && [self isBrowserActive];
  adsClientNotifier->NotifyTabDidChange((int32_t)tabId, urls, isVisible);
}

- (void)reportTabClosedWithTabId:(NSInteger)tabId {
  if (![self isAdsServiceRunning]) {
    return;
  }
  adsClientNotifier->NotifyDidCloseTab((int32_t)tabId);
}

- (void)reportNotificationAdEvent:(NSString*)placementId
                        eventType:(BraveAdsNotificationAdEventType)eventType
                       completion:(void (^)(BOOL success))completion {
  if (![self isAdsServiceRunning]) {
    return;
  }
  ads->TriggerNotificationAdEvent(
      base::SysNSStringToUTF8(placementId),
      static_cast<brave_ads::mojom::NotificationAdEventType>(eventType),
      base::BindOnce(^(const bool success) {
        completion(success);
      }));
}

- (void)reportNewTabPageAdEvent:(NSString*)wallpaperId
             creativeInstanceId:(NSString*)creativeInstanceId
                      eventType:(BraveAdsNewTabPageAdEventType)eventType
                     completion:(void (^)(BOOL success))completion {
  if (![self isAdsServiceRunning]) {
    return;
  }
  ads->TriggerNewTabPageAdEvent(
      base::SysNSStringToUTF8(wallpaperId),
      base::SysNSStringToUTF8(creativeInstanceId),
      static_cast<brave_ads::mojom::NewTabPageAdEventType>(eventType),
      base::BindOnce(^(const bool success) {
        completion(success);
      }));
}

- (void)inlineContentAdsWithDimensions:(NSString*)dimensionsArg
                            completion:
                                (void (^)(NSString* dimensions,
                                          InlineContentAdIOS* ad))completion {
  if (![self isAdsServiceRunning]) {
    return;
  }
  ads->MaybeServeInlineContentAd(
      base::SysNSStringToUTF8(dimensionsArg),
      base::BindOnce(
          ^(const std::string& dimensions,
            const absl::optional<brave_ads::InlineContentAdInfo>& ad) {
            if (!ad) {
              completion(base::SysUTF8ToNSString(dimensions), nil);
              return;
            }

            const auto inline_content_ad =
                [[InlineContentAdIOS alloc] initWithInlineContentAdInfo:*ad];
            completion(base::SysUTF8ToNSString(dimensions), inline_content_ad);
          }));
}

- (void)reportInlineContentAdEvent:(NSString*)placementId
                creativeInstanceId:(NSString*)creativeInstanceId
                         eventType:(BraveAdsInlineContentAdEventType)eventType
                        completion:(void (^)(BOOL success))completion {
  if (![self isAdsServiceRunning]) {
    return;
  }
  ads->TriggerInlineContentAdEvent(
      base::SysNSStringToUTF8(placementId),
      base::SysNSStringToUTF8(creativeInstanceId),
      static_cast<brave_ads::mojom::InlineContentAdEventType>(eventType),
      base::BindOnce(^(const bool success) {
        completion(success);
      }));
}

- (void)reportPromotedContentAdEvent:(NSString*)placementId
                  creativeInstanceId:(NSString*)creativeInstanceId
                           eventType:
                               (BraveAdsPromotedContentAdEventType)eventType
                          completion:(void (^)(BOOL success))completion {
  if (![self isAdsServiceRunning]) {
    return;
  }
  ads->TriggerPromotedContentAdEvent(
      base::SysNSStringToUTF8(placementId),
      base::SysNSStringToUTF8(creativeInstanceId),
      static_cast<brave_ads::mojom::PromotedContentAdEventType>(eventType),
      base::BindOnce(^(const bool success) {
        completion(success);
      }));
}

- (void)purgeOrphanedAdEvents:(BraveAdsAdType)adType
                   completion:(void (^)(BOOL success))completion {
  if (![self isAdsServiceRunning]) {
    return;
  }
  ads->PurgeOrphanedAdEventsForType(
      static_cast<brave_ads::mojom::AdType>(adType),
      base::BindOnce(^(const bool success) {
        completion(success);
      }));
}

- (void)detailsForCurrentCycle:(void (^)(NSInteger adsReceived,
                                         double estimatedEarnings,
                                         NSDate* nextPaymentDate))completion {
  if (![self isAdsServiceRunning]) {
    return;
  }
  ads->GetStatementOfAccounts(
      base::BindOnce(^(brave_ads::mojom::StatementInfoPtr statement) {
        if (!statement) {
          completion(0, 0, nil);
          return;
        }

        NSDate* nextPaymentDate = nil;
        if (!statement->next_payment_date.is_null()) {
          nextPaymentDate =
              [NSDate dateWithTimeIntervalSince1970:statement->next_payment_date
                                                        .ToDoubleT()];
        }
        completion(statement->ads_received_this_month,
                   statement->max_earnings_this_month, nextPaymentDate);
      }));
}

- (void)toggleThumbsUpForAd:(NSString*)creativeInstanceId
               advertiserId:(NSString*)advertiserId
                    segment:(NSString*)segment {
  if (![self isAdsServiceRunning]) {
    return;
  }
  brave_ads::AdContentInfo ad_content;
  ad_content.type = brave_ads::AdType::kNotificationAd;
  ad_content.creative_instance_id = base::SysNSStringToUTF8(creativeInstanceId);
  ad_content.advertiser_id = base::SysNSStringToUTF8(advertiserId);
  ad_content.segment = base::SysNSStringToUTF8(segment);
  ads->ToggleLikeAd(brave_ads::AdContentToValue(ad_content));
}

- (void)toggleThumbsDownForAd:(NSString*)creativeInstanceId
                 advertiserId:(NSString*)advertiserId
                      segment:(NSString*)segment {
  if (![self isAdsServiceRunning]) {
    return;
  }
  brave_ads::AdContentInfo ad_content;
  ad_content.type = brave_ads::AdType::kNotificationAd;
  ad_content.creative_instance_id = base::SysNSStringToUTF8(creativeInstanceId);
  ad_content.advertiser_id = base::SysNSStringToUTF8(advertiserId);
  ad_content.segment = base::SysNSStringToUTF8(segment);
  ads->ToggleDislikeAd(brave_ads::AdContentToValue(ad_content));
}

#pragma mark - Configuration

- (uint64_t)getNotificationAdsPerHour {
  return self.numberOfAllowableAdsPerHour;
}

- (bool)isAdsEnabled {
  return self.enabled;
}

- (bool)isBrowserActive {
  return UIApplication.sharedApplication.applicationState ==
         UIApplicationStateActive;
}

- (bool)isBrowserInFullScreenMode {
  return true;
}

- (bool)canShowNotificationAdsWhileBrowserIsBackgrounded {
  return false;
}

- (bool)isNetworkConnectionAvailable {
  return self.networkConnectivityAvailable;
}

- (void)setIdleThreshold:(const int)threshold {
  // Not needed on mobile
}

#pragma mark - Network

- (void)UrlRequest:(brave_ads::mojom::UrlRequestInfoPtr)url_request
          callback:(brave_ads::UrlRequestCallback)callback {
  std::map<brave_ads::mojom::UrlRequestMethodType, std::string> methodMap{
      {brave_ads::mojom::UrlRequestMethodType::kGet, "GET"},
      {brave_ads::mojom::UrlRequestMethodType::kPost, "POST"},
      {brave_ads::mojom::UrlRequestMethodType::kPut, "PUT"}};

  const auto copiedURL = url_request->url;

  auto cb = std::make_shared<decltype(callback)>(std::move(callback));
  const auto __weak weakSelf = self;
  return [self.commonOps
      loadURLRequest:url_request->url.spec()
             headers:url_request->headers
             content:url_request->content
        content_type:url_request->content_type
              method:methodMap[url_request->method]
            callback:^(
                const std::string& errorDescription, int statusCode,
                const std::string& response,
                const base::flat_map<std::string, std::string>& headers) {
              const auto strongSelf = weakSelf;
              if (!strongSelf || ![strongSelf isAdsServiceRunning]) {
                return;
              }
              brave_ads::mojom::UrlResponseInfo url_response;
              url_response.url = copiedURL;
              url_response.status_code = statusCode;
              url_response.body = response;
              url_response.headers = headers;
              if (cb) {
                std::move(*cb).Run(url_response);
              }
            }];
}

#pragma mark - File IO

- (NSDictionary*)adsResourceMetadata {
  return (NSDictionary*)self.prefs[kAdsResourceMetadataPrefKey];
}

- (void)setAdsResourceMetadata:(NSDictionary*)adsResourceMetadata {
  self.prefs[kAdsResourceMetadataPrefKey] = adsResourceMetadata;
  [self savePref:kAdsResourceMetadataPrefKey];
}

- (BOOL)registerAdsResourcesForLanguageCode:(NSString*)languageCode {
  if (!languageCode) {
    return NO;
  }

  NSString* isoLanguageCode =
      [@"iso_639_1_" stringByAppendingString:[languageCode lowercaseString]];

  NSArray* languageCodeAdsResourceIds =
      [self.componentPaths allKeysForObject:isoLanguageCode];
  if ([languageCodeAdsResourceIds count] == 0) {
    return NO;
  }

  NSString* languageCodeAdsResourceId =
      [languageCodeAdsResourceIds firstObject];
  BLOG(1, @"Registering Brave Ads Resources Installer (%@) with id %@",
       languageCode, languageCodeAdsResourceId);

  BLOG(1, @"Notifying ads resource observers");

  const auto __weak weakSelf = self;
  [self downloadAdsResource:isoLanguageCode
                 completion:^(BOOL success) {
                   const auto strongSelf = weakSelf;
                   if (!strongSelf) {
                     return;
                   }
                   if (success) {
                     const std::string bridged_language_code_adsResource_idkey =
                         base::SysNSStringToUTF8(languageCodeAdsResourceId);
                     strongSelf->adsClientNotifier
                         ->NotifyDidUpdateResourceComponent(
                             "1", bridged_language_code_adsResource_idkey);
                   }
                 }];

  return YES;
}

- (BOOL)registerAdsResourcesForCountryCode:(NSString*)countryCode {
  if (!countryCode) {
    return NO;
  }

  NSString* isoCountryCode =
      [@"iso_3166_1_" stringByAppendingString:[countryCode lowercaseString]];

  NSArray* countryCodeAdsResourceIds =
      [self.componentPaths allKeysForObject:isoCountryCode];
  if ([countryCodeAdsResourceIds count] == 0) {
    return NO;
  }

  NSString* countryCodeAdsResourceId = [countryCodeAdsResourceIds firstObject];
  BLOG(1, @"Registering Brave Ads Resources Installer (%@) with id %@",
       countryCode, countryCodeAdsResourceId);

  BLOG(1, @"Notifying ads resource observers");
  const auto __weak weakSelf = self;
  [self downloadAdsResource:isoCountryCode
                 completion:^(BOOL success) {
                   const auto strongSelf = weakSelf;
                   if (!strongSelf) {
                     return;
                   }
                   if (success) {
                     const std::string bridged_country_code_adsResource_idkey =
                         base::SysNSStringToUTF8(countryCodeAdsResourceId);

                     strongSelf->adsClientNotifier
                         ->NotifyDidUpdateResourceComponent(
                             "1", bridged_country_code_adsResource_idkey);
                   }
                 }];

  return YES;
}

- (void)registerAdsResources {
  const auto currentLocale = [NSLocale currentLocale];

  if (![self registerAdsResourcesForLanguageCode:currentLocale.languageCode]) {
    BLOG(1, @"%@ not supported for ads resource installer",
         currentLocale.languageCode);
  }

  if (![self registerAdsResourcesForCountryCode:currentLocale.countryCode]) {
    BLOG(1, @"%@ not supported for ads resource installer",
         currentLocale.countryCode);
  }
}

- (void)periodicallyCheckForAdsResourceUpdates {
  const uint64_t time_offset = 6 * 60 * 60;  // every 6 hours

  const auto __weak weakSelf = self;
  self.updateAdsResourceTimer = [NSTimer
      scheduledTimerWithTimeInterval:time_offset
                             repeats:YES
                               block:^(NSTimer* _Nonnull timer) {
                                 const auto strongSelf = weakSelf;
                                 if (!strongSelf) {
                                   return;
                                 }

                                 [strongSelf updateAdsResources];

                                 NSDateFormatter* formatter =
                                     [[NSDateFormatter alloc] init];
                                 formatter.dateStyle = NSDateFormatterFullStyle;
                                 formatter.timeStyle = NSDateFormatterFullStyle;
                                 BLOG(1, @"Update ads resources on %@",
                                      [formatter
                                          stringFromDate:
                                              [[NSDate date]
                                                  dateByAddingTimeInterval:
                                                      time_offset]]);
                               }];
}

- (void)updateAdsResources {
  BLOG(1, @"Updating ads resources");

  const auto currentLocale = [NSLocale currentLocale];
  NSString* isoLanguageCode = [@"iso_639_1_"
      stringByAppendingString:[currentLocale.languageCode lowercaseString]];
  NSString* isoCountryCode = [@"iso_3166_1_"
      stringByAppendingString:[currentLocale.countryCode lowercaseString]];

  for (NSString* key in @[ isoLanguageCode, isoCountryCode ]) {
    BLOG(1, @"Checking %@ ads resource for updates", key);

    const auto __weak weakSelf = self;
    [self downloadAdsResource:key
                   completion:^(BOOL success) {
                     const auto strongSelf = weakSelf;
                     if (!strongSelf) {
                       return;
                     }

                     if (!success) {
                       BLOG(1, @"Failed to update ads resources");
                       return;
                     }

                     BLOG(1, @"Notifying ads resource observers");
                     strongSelf->adsClientNotifier
                         ->NotifyDidUpdateResourceComponent(
                             "1", base::SysNSStringToUTF8(key));
                   }];
  }
}

- (void)downloadAdsResource:(NSString*)folderName
                 completion:(void (^)(BOOL success))completion {
  BLOG(1, @"Downloading %@ ads resource manifest", folderName);

  const auto __weak weakSelf = self;

  void (^handleRetry)() = ^{
    const auto strongSelf = weakSelf;
    const int64_t backoff = 1 * 60;
    int64_t delay = backoff << strongSelf.adsResourceRetryCount;
    if (delay >= 60 * 60) {
      delay = 60 * 60;
    } else {
      strongSelf.adsResourceRetryCount++;
    }

    NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
    formatter.dateStyle = NSDateFormatterFullStyle;
    formatter.timeStyle = NSDateFormatterFullStyle;
    BLOG(1, @"Retry loading %@ ads resource on %@", folderName,
         [formatter
             stringFromDate:[[NSDate date] dateByAddingTimeInterval:delay]]);

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, delay * NSEC_PER_SEC),
                   dispatch_get_main_queue(), ^{
                     const auto strongSelf2 = weakSelf;
                     if (!strongSelf2 || ![strongSelf2 isAdsServiceRunning]) {
                       return;
                     }
                     [strongSelf2 downloadAdsResource:folderName
                                           completion:completion];
                   });
  };

  NSString* baseUrl = @"https://brave-user-model-installer-input.s3.brave.com";
  const auto flags = brave_rewards::RewardsFlags::ForCurrentProcess();
  if (flags.environment) {
    switch (*flags.environment) {
      case brave_rewards::RewardsFlags::Environment::kDevelopment:
      case brave_rewards::RewardsFlags::Environment::kStaging:
        baseUrl = @"https://"
                  @"brave-user-model-installer-input-dev.s3.bravesoftware.com";
        break;
      case brave_rewards::RewardsFlags::Environment::kProduction:
        break;
    }
  }
  baseUrl = [baseUrl stringByAppendingPathComponent:folderName];

  NSString* manifestUrl =
      [baseUrl stringByAppendingPathComponent:@"resources.json"];
  [self.commonOps loadURLRequest:base::SysNSStringToUTF8(manifestUrl)
      headers:{}
      content:""
      content_type:""
      method:"GET"
      callback:^(const std::string& errorDescriptionArg, int statusCodeArg,
                 const std::string& responseStr,
                 const base::flat_map<std::string, std::string>& headersArg) {
        const auto strongSelf = weakSelf;
        if (!strongSelf || ![strongSelf isAdsServiceRunning]) {
          return;
        }

        if (statusCodeArg == 404) {
          BLOG(1, @"%@ ads resource manifest not found", folderName);
          completion(NO);
          return;
        }

        if (statusCodeArg != 200) {
          BLOG(1, @"Failed to download %@ ads resource manifest", folderName);
          handleRetry();
          return;
        }

        NSData* data = [base::SysUTF8ToNSString(responseStr)
            dataUsingEncoding:NSUTF8StringEncoding];
        NSDictionary* dict = [NSJSONSerialization JSONObjectWithData:data
                                                             options:0
                                                               error:nil];
        if (!dict) {
          handleRetry();
          return;
        }

        NSNumber* schemaVersion = dict[@"schemaVersion"];
        if ([schemaVersion intValue] !=
            kCurrentAdsResourceManifestSchemaVersion) {
          BLOG(1, @"Invalid schema version for ads resource manifest %@ (%d)",
               folderName, [schemaVersion intValue]);
          handleRetry();
          return;
        }

        NSArray* adsResources = dict[@"resources"];

        const auto group = dispatch_group_create();
        BOOL __block allSuccessful = YES;
        BOOL __block shouldRetry = NO;

        for (NSDictionary* adsResource in adsResources) {
          NSString* adsResourceStatus = adsResource[@"status"];
          if (!adsResourceStatus ||
              ![adsResourceStatus isEqualToString:@"stable"]) {
            continue;
          }

          NSString* adsResourceId = adsResource[@"id"];
          if (!adsResourceId) {
            continue;
          }

          NSString* filename = adsResource[@"filename"];
          if (!filename) {
            continue;
          }

          NSNumber* version = adsResource[@"version"];
          if (!version) {
            continue;
          }

          NSDictionary* adsResourceMetadataDict =
              [strongSelf adsResourceMetadata];
          if (version <= adsResourceMetadataDict[adsResourceId]) {
            BLOG(1, @"%@ ads resource is up to date on version %@",
                 adsResourceId, version);
            continue;
          }

          NSString* adsResourceUrl =
              [baseUrl stringByAppendingPathComponent:filename];

          BLOG(1, @"Downloading %@ ads resource version %@", adsResourceId,
               version);

          dispatch_group_enter(group);
          [strongSelf.commonOps
              loadURLRequest:base::SysNSStringToUTF8(adsResourceUrl)
              headers:{}
              content:""
              content_type:""
              method:"GET"
              callback:^(
                  const std::string& errorDescription, int statusCode,
                  const std::string& response,
                  const base::flat_map<std::string, std::string>& headers) {
                if (!strongSelf || ![strongSelf isAdsServiceRunning]) {
                  return;
                }

                if (statusCode == 404) {
                  BLOG(1, @"%@ ads resource not found", folderName);
                  allSuccessful = NO;
                  return;
                }

                if (statusCode != 200) {
                  BLOG(1, @"Failed to download %@ ads resource version %@",
                       adsResourceId, version);
                  allSuccessful = NO;
                  shouldRetry = YES;
                  return;
                }

                [strongSelf.commonOps saveContents:response
                                              name:adsResourceId.UTF8String];
                BLOG(1, @"Cached %@ ads resource version %@", adsResourceId,
                     version);

                NSMutableDictionary* dictionary =
                    [[strongSelf adsResourceMetadata] mutableCopy];
                dictionary[adsResourceId] = version;
                [strongSelf setAdsResourceMetadata:dictionary];

                BLOG(1, @"%@ ads resource updated to version %@", adsResourceId,
                     version);
                dispatch_group_leave(group);
              }];
        }

        dispatch_group_notify(group, dispatch_get_main_queue(), ^{
          if (shouldRetry) {
            handleRetry();
          } else {
            completion(allSuccessful);
          }
        });
      }];
}

- (void)getBrowsingHistory:(const int)max_count
                   forDays:(const int)days_ago
                  callback:(brave_ads::GetBrowsingHistoryCallback)callback {
  // To be implemented https://github.com/brave/brave-ios/issues/3499
  std::move(callback).Run({});
}

- (void)loadFileResource:(const std::string&)id
                 version:(const int)version
                callback:(brave_ads::LoadFileCallback)callback {
  NSString* bridgedId = base::SysUTF8ToNSString(id);
  NSString* nsFilePath = [self.commonOps dataPathForFilename:bridgedId];

  BLOG(1, @"Loading %@ ads resource descriptor", nsFilePath);

  base::FilePath file_path(nsFilePath.UTF8String);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()}, base::BindOnce(^base::File {
        return base::File(file_path,
                          base::File::FLAG_OPEN | base::File::FLAG_READ);
      }),
      base::BindOnce(std::move(callback)));
}

- (void)getScheduledCaptcha:(const std::string&)payment_id
                   callback:(brave_ads::GetScheduledCaptchaCallback)callback {
  // Adaptive captcha not supported on iOS
  std::move(callback).Run("");
}

- (void)showScheduledCaptchaNotification:(const std::string&)payment_id
                               captchaId:(const std::string&)captcha_id {
  [self.captchaHandler
      handleAdaptiveCaptchaForPaymentId:base::SysUTF8ToNSString(payment_id)
                              captchaId:base::SysUTF8ToNSString(captcha_id)];
}

- (void)load:(const std::string&)name
    callback:(brave_ads::LoadCallback)callback {
  const auto contents = [self.commonOps loadContentsFromFileWithName:name];
  if (contents.empty()) {
    std::move(callback).Run(/*value*/ absl::nullopt);
  } else {
    std::move(callback).Run(contents);
  }
}

- (const std::string)loadDataResource:(const std::string&)name {
  const auto bundle = [NSBundle bundleForClass:[BraveAds class]];
  const auto path = [bundle pathForResource:base::SysUTF8ToNSString(name)
                                     ofType:nil];
  if (!path || path.length == 0) {
    return "";
  }
  NSError* error = nil;
  const auto contents = [NSString stringWithContentsOfFile:path
                                                  encoding:NSUTF8StringEncoding
                                                     error:&error];
  if (!contents || error) {
    return "";
  }
  return std::string(contents.UTF8String);
}

- (void)save:(const std::string&)name
       value:(const std::string&)value
    callback:(brave_ads::SaveCallback)callback {
  if ([self.commonOps saveContents:value name:name]) {
    std::move(callback).Run(/*success*/ true);
  } else {
    std::move(callback).Run(/*success*/ false);
  }
}

#pragma mark - Logging

- (void)log:(const char*)file
            line:(const int)line
    verboseLevel:(const int)verbose_level
         message:(const std::string&)message {
  const int vlog_level = logging::GetVlogLevelHelper(file, strlen(file));
  if (verbose_level <= vlog_level) {
    logging::LogMessage(file, line, -verbose_level).stream() << message;
  }
}

#pragma mark - Notifications

- (nullable NotificationAdIOS*)notificationAdForIdentifier:
    (NSString*)identifier {
  if (![self isAdsServiceRunning]) {
    return nil;
  }

  const absl::optional<brave_ads::NotificationAdInfo> ad =
      ads->MaybeGetNotificationAd(identifier.UTF8String);
  if (!ad) {
    return nil;
  }

  return [[NotificationAdIOS alloc] initWithNotificationInfo:*ad];
}

- (bool)canShowNotificationAds {
  return [self.notificationsHandler shouldShowNotifications];
}

- (void)showNotificationAd:(const brave_ads::NotificationAdInfo&)info {
  const auto notification =
      [[NotificationAdIOS alloc] initWithNotificationInfo:info];
  [self.notificationsHandler showNotification:notification];
}

- (void)closeNotificationAd:(const std::string&)placement_id {
  const auto bridgedPlacementId = base::SysUTF8ToNSString(placement_id);
  [self.notificationsHandler
      clearNotificationWithIdentifier:bridgedPlacementId];
}

- (void)showReminder:(const brave_ads::mojom::ReminderType)type {
  // Not needed on iOS
}

- (void)cacheAdEventForInstanceId:(const std::string&)id
                           adType:(const std::string&)ad_type
                 confirmationType:(const std::string&)confirmation_type
                             time:(const base::Time)time {
  if (!adEventCache) {
    return;
  }

  adEventCache->AddEntryForInstanceId(id, ad_type, confirmation_type, time);
}

- (std::vector<base::Time>)getCachedAdEvents:(const std::string&)ad_type
                            confirmationType:
                                (const std::string&)confirmation_type {
  if (!adEventCache) {
    return {};
  }

  return adEventCache->Get(ad_type, confirmation_type);
}

- (void)resetAdEventCacheForInstanceId:(const std::string&)id {
  if (!adEventCache) {
    return;
  }

  return adEventCache->ResetForInstanceId(id);
}

- (bool)shouldAllowAdsSubdivisionTargeting {
  return self.shouldAllowSubdivisionTargeting;
}

- (void)setAllowAdsSubdivisionTargeting:(const bool)should_allow {
  self.allowSubdivisionTargeting = should_allow;
}

- (std::string)adsSubdivisionTargetingCode {
  return base::SysNSStringToUTF8(self.subdivisionTargetingCode);
}

- (void)setAdsSubdivisionTargetingCode:
    (const std::string&)subdivision_targeting_code {
  self.subdivisionTargetingCode =
      [NSString stringWithCString:subdivision_targeting_code.c_str()
                         encoding:[NSString defaultCStringEncoding]];
}

- (std::string)autoDetectedAdsSubdivisionTargetingCode {
  return base::SysNSStringToUTF8(self.autoDetectedSubdivisionTargetingCode);
}

- (void)setAutoDetectedAdsSubdivisionTargetingCode:
    (const std::string&)subdivision_targeting_code {
  self.autoDetectedSubdivisionTargetingCode =
      [NSString stringWithCString:subdivision_targeting_code.c_str()
                         encoding:[NSString defaultCStringEncoding]];
}

- (void)runDBTransaction:(brave_ads::mojom::DBTransactionInfoPtr)transaction
                callback:(brave_ads::RunDBTransactionCallback)completion {
  __weak BraveAds* weakSelf = self;
  databaseQueue->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&RunDBTransactionOnTaskRunner, std::move(transaction),
                     adsDatabase),
      base::BindOnce(
          ^(brave_ads::RunDBTransactionCallback callback,
            brave_ads::mojom::DBCommandResponseInfoPtr response) {
            const auto strongSelf = weakSelf;
            if (!strongSelf || ![strongSelf isAdsServiceRunning]) {
              return;
            }
            std::move(callback).Run(std::move(response));
          },
          std::move(completion)));
}

- (void)updateAdRewards {
  // Not needed on iOS because ads do not show unless you are viewing a tab
}

- (void)setBooleanPref:(const std::string&)path value:(const bool)value {
  const auto key = base::SysUTF8ToNSString(path);
  self.prefs[key] = @(value);
  [self savePref:key];
}

- (bool)getBooleanPref:(const std::string&)path {
  // TODO(https://github.com/brave/brave-browser/issues/32112): Remove the
  // code that permanently sets values for preferences when the issue is
  // resolved.
  if (path == brave_news::prefs::kBraveNewsOptedIn ||
      path == brave_news::prefs::kNewTabPageShowToday ||
      path == ntp_background_images::prefs::kNewTabPageShowBackgroundImage ||
      path == ntp_background_images::prefs::
                  kNewTabPageShowSponsoredImagesBackgroundImage) {
    return true;
  }

  const auto key = base::SysUTF8ToNSString(path);
  return [self.prefs[key] boolValue];
}

- (void)setIntegerPref:(const std::string&)path value:(const int)value {
  const auto key = base::SysUTF8ToNSString(path);
  self.prefs[key] = @(value);
  [self savePref:key];
}

- (int)getIntegerPref:(const std::string&)path {
  // TODO(https://github.com/brave/brave-browser/issues/32112): Remove the
  // code that permanently sets values for preferences when the issue is
  // resolved.
  if (path == brave_ads::prefs::kIssuerPing && ![self hasPrefPath:path]) {
    return 7'200'000;
  }

  const auto key = base::SysUTF8ToNSString(path);
  return [self.prefs[key] intValue];
}

- (void)setDoublePref:(const std::string&)path value:(const double)value {
  const auto key = base::SysUTF8ToNSString(path);
  self.prefs[key] = @(value);
  [self savePref:key];
}

- (double)getDoublePref:(const std::string&)path {
  const auto key = base::SysUTF8ToNSString(path);
  return [self.prefs[key] doubleValue];
}

- (void)setStringPref:(const std::string&)path value:(const std::string&)value {
  const auto key = base::SysUTF8ToNSString(path);
  self.prefs[key] = base::SysUTF8ToNSString(value);
  [self savePref:key];
}

- (std::string)getStringPref:(const std::string&)path {
  // TODO(https://github.com/brave/brave-browser/issues/32112): Remove the
  // code that permanently sets values for preferences when the issue is
  // resolved.
  if (path == brave_ads::prefs::kSubdivisionTargetingSubdivision) {
    return "AUTO";
  }

  const auto key = base::SysUTF8ToNSString(path);
  const auto value = (NSString*)self.prefs[key];
  if (!value) {
    return "";
  }
  return value.UTF8String;
}

- (void)setInt64Pref:(const std::string&)path value:(const int64_t)value {
  const auto key = base::SysUTF8ToNSString(path);
  self.prefs[key] = @(value);
  [self savePref:key];
}

- (int64_t)getInt64Pref:(const std::string&)path {
  const auto key = base::SysUTF8ToNSString(path);
  return [self.prefs[key] longLongValue];
}

- (void)setUint64Pref:(const std::string&)path value:(const uint64_t)value {
  const auto key = base::SysUTF8ToNSString(path);
  self.prefs[key] = @(value);
  [self savePref:key];
}

- (uint64_t)getUint64Pref:(const std::string&)path {
  const auto key = base::SysUTF8ToNSString(path);
  return [self.prefs[key] unsignedLongLongValue];
}

- (void)setTimePref:(const std::string&)path value:(const base::Time)value {
  const auto key = base::SysUTF8ToNSString(path);
  self.prefs[key] = @(value.ToDoubleT());
  [self savePref:key];
}

- (base::Time)getTimePref:(const std::string&)path {
  const auto key = base::SysUTF8ToNSString(path);
  return base::Time::FromDoubleT([self.prefs[key] doubleValue]);
}

- (void)setDictPref:(const std::string&)path value:(base::Value::Dict)value {
  std::string json;
  if (base::JSONWriter::Write(value, &json)) {
    const auto key = base::SysUTF8ToNSString(path);
    self.prefs[key] = base::SysUTF8ToNSString(json);
    [self savePref:key];
  }
}

- (absl::optional<base::Value::Dict>)getDictPref:(const std::string&)path {
  const auto key = base::SysUTF8ToNSString(path);
  const auto json = (NSString*)self.prefs[key];
  if (!json) {
    return absl::nullopt;
  }

  absl::optional<base::Value> value =
      base::JSONReader::Read(base::SysNSStringToUTF8(json));
  if (!value || !value->is_dict()) {
    return absl::nullopt;
  }

  return value->GetDict().Clone();
}

- (void)setListPref:(const std::string&)path value:(base::Value::List)value {
  std::string json;
  if (base::JSONWriter::Write(value, &json)) {
    const auto key = base::SysUTF8ToNSString(path);
    self.prefs[key] = base::SysUTF8ToNSString(json);
    [self savePref:key];
  }
}

- (absl::optional<base::Value::List>)getListPref:(const std::string&)path {
  const auto key = base::SysUTF8ToNSString(path);
  const auto json = (NSString*)self.prefs[key];
  if (!json) {
    return absl::nullopt;
  }

  absl::optional<base::Value> value =
      base::JSONReader::Read(base::SysNSStringToUTF8(json));
  if (!value || !value->is_list()) {
    return absl::nullopt;
  }

  return value->GetList().Clone();
}

- (void)clearPref:(const std::string&)path {
  const auto key = base::SysUTF8ToNSString(path);
  [self.prefs removeObjectForKey:key];
  [self savePref:key];
}

- (bool)hasPrefPath:(const std::string&)path {
  const auto key = base::SysUTF8ToNSString(path);
  return [self.prefs objectForKey:key] != nil;
}

- (void)setLocalStatePref:(const std::string&)path value:(base::Value)value {
  // Not needed on iOS
}

- (absl::optional<base::Value>)getLocalStatePref:(const std::string&)path {
  if (path == brave_l10n::prefs::kCountryCode) {
    return base::Value(brave_l10n::GetDefaultISOCountryCodeString());
  }

  return absl::nullopt;
}

#pragma mark - Ads Resources Paths

- (NSDictionary*)componentPaths {
  static NSDictionary* _paths = nil;

  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _paths = @{
      @"jememeholcpjpoahinnlafoiaknnmfgl" : @"iso_3166_1_af",
      @"hfonhokmgmjionconfpknjfphfahdklo" : @"iso_3166_1_ax",
      @"anlkmbkbgleadcacchhgdoecllpllknb" : @"iso_3166_1_al",
      @"imoolhehjnpebcjecoinphmohihmbccj" : @"iso_3166_1_dz",
      @"kgnhcdjacgcanjnbdcmngdeoncckfmfh" : @"iso_3166_1_as",
      @"pmlmnjficamnkblapnohndlnhkkoaoco" : @"iso_3166_1_ad",
      @"majdffglhcddbbanjnhfocagmmhobghd" : @"iso_3166_1_ao",
      @"bhdlolcjjefaidodgffjhpbeeapabpgn" : @"iso_3166_1_ai",
      @"pbanoihfljabneihobeplbciopfilajn" : @"iso_3166_1_aq",
      @"cbdjliajiakicmdohhbjbgggacbjdnmn" : @"iso_3166_1_ag",
      @"enadnicbppimlbpilkeaogmmfpennphn" : @"iso_3166_1_ar",
      @"cpnhinhihfnhhmoknpbkcifgadjbcjnf" : @"iso_3166_1_am",
      @"ocegkjjbmlnibhfjmeiaidplhcbdhomd" : @"iso_3166_1_aw",
      @"kklfafolbojbonkjgifmmkdmaaimminj" : @"iso_3166_1_au",
      @"jmneklmcodckmpipiekkfaokobhkflep" : @"iso_3166_1_at",
      @"llmikniomoddmmghodjfbncbidjlhhid" : @"iso_3166_1_az",
      @"aaoipmkakdldlippoaocoegnnfnpcokj" : @"iso_3166_1_bs",
      @"megoieebjempmobckocciojgfhfmiejb" : @"iso_3166_1_bh",
      @"ppkgobeickbpfmmkbhfgmgiloepdiagn" : @"iso_3166_1_bd",
      @"ndfnmlonkimafoabeafnaignianecdhf" : @"iso_3166_1_bb",
      @"apndmjdcfbhgfccccdmmeofpdgnlbbif" : @"iso_3166_1_by",
      @"lnbdfmpjjckjhnmahgdojnfnmdmpebfn" : @"iso_3166_1_be",
      @"demegfhfekncneocdlflandgegpcoofj" : @"iso_3166_1_bz",
      @"jiodmgmkikfbkchgenlamoabbfnobnfh" : @"iso_3166_1_bj",
      @"aeiffmlccgeaacefkfknodmnebanekbo" : @"iso_3166_1_bm",
      @"hemccombjdaepjnhhdplhiaedaackooa" : @"iso_3166_1_bt",
      @"dggplmjbjalpdgndkigojpikblaiflke" : @"iso_3166_1_bo",
      @"jbibpedjodeigdoimlgpikphaabhdbie" : @"iso_3166_1_bq",
      @"iefeaaegnnpiadjfoeifapjmflnbjehg" : @"iso_3166_1_ba",
      @"bfdagcnfmfaidkkjcmfjmogiefoiglem" : @"iso_3166_1_bw",
      @"kfimhlhdlhimaheficomcahhbaicoele" : @"iso_3166_1_bv",
      @"fbpmbjccnaaeogogeldlomcmlhllgaje" : @"iso_3166_1_br",
      @"cpbmgmnfoakodmmpppabghhckchppneg" : @"iso_3166_1_io",
      @"gcikmigghagkligpileoekfjmohokjhm" : @"iso_3166_1_bn",
      @"ahhbponhjohgifhjbjggafboffbimmmg" : @"iso_3166_1_bg",
      @"fjgjapaemndekhnfeopeoeajfpmlgemo" : @"iso_3166_1_bf",
      @"cfbbahiimladdkhblpkkokkmemlmkbhe" : @"iso_3166_1_bi",
      @"oeneodeckioghmhokkmcbijfanjbanop" : @"iso_3166_1_cv",
      @"cmknopomfihgdpjlnjhjkmogaooeoeic" : @"iso_3166_1_kh",
      @"mmiflfidlgoehkhhkeodfdjpbkkjadgi" : @"iso_3166_1_cm",
      @"lgejdiamednlaeiknhnnjnkofmapfbbf" : @"iso_3166_1_ca",
      @"efpgpbmpbkhadlnpdnjigldeebgacopp" : @"iso_3166_1_ky",
      @"ljfeoddejgcdofgnpgljaeiaemfimgej" : @"iso_3166_1_cf",
      @"oahnhemdhgogkhgljdkhbficecbplmdf" : @"iso_3166_1_td",
      @"gbbfjnnpelockljikcmjkeodlaebjokm" : @"iso_3166_1_cl",
      @"gfccfpdijajblfnkmddbflphiiibifik" : @"iso_3166_1_cn",
      @"mjennfbimaafgcoekloojmbhnkophgni" : @"iso_3166_1_cx",
      @"pnfogoijegjkepejdabehdfadbkpgoed" : @"iso_3166_1_cc",
      @"cegiaceckhbagmmfcoeebeghiddghbkk" : @"iso_3166_1_co",
      @"efcmpmpbmaehimomnmonodlpnghelnfi" : @"iso_3166_1_km",
      @"hkgnnbjmfcelmehmphhbjigedknjobaa" : @"iso_3166_1_cg",
      @"kignebofcmcgmjfiapilgdfkbekmkdmg" : @"iso_3166_1_cd",
      @"clcghlkineckendfhkgdpkplofknofjo" : @"iso_3166_1_ck",
      @"hmmoibmjgckbeejmcfflnngeacaklchb" : @"iso_3166_1_cr",
      @"nopcbglolocphcdeikfkoeppkhijacij" : @"iso_3166_1_ci",
      @"mjhnpmgafkmildljebajibghemlbffni" : @"iso_3166_1_hr",
      @"mdopdmalncfakkimlojioichjbebcaip" : @"iso_3166_1_cu",
      @"boeecnnjahpahhockgdcbdlaimpcflig" : @"iso_3166_1_cw",
      @"hknminnkgcjafipipbbalkakppehkpjn" : @"iso_3166_1_cy",
      @"iejekkikpddbbockoldagmfcdbffomfc" : @"iso_3166_1_cz",
      @"kmfkbonhconlbieplamnikedgfbggail" : @"iso_3166_1_dk",
      @"phihhhnelfclomhodhgalooldjgejgfl" : @"iso_3166_1_dj",
      @"obihnhimgbeenjgfnlcdbfdgkkgeogdp" : @"iso_3166_1_dm",
      @"gciaobanmdlfkegiikhgdoogegeghhch" : @"iso_3166_1_do",
      @"imnpbmpnmlmkjpgfnfeplikingjklpej" : @"iso_3166_1_ec",
      @"ojfkdbfibflfejobeodhoepghgoghkii" : @"iso_3166_1_eg",
      @"adnhangbagjlobdeicimblgcnafegpfb" : @"iso_3166_1_sv",
      @"gncihmgakhljdlkadibldhdfnocfplci" : @"iso_3166_1_gq",
      @"diacfpapelanfbkmehdpaaohmnkkngge" : @"iso_3166_1_er",
      @"jdigggiplmjlocdopbdmjibckpamigij" : @"iso_3166_1_ee",
      @"npacefioaofgbofilfnhliofkefklipp" : @"iso_3166_1_sz",
      @"lbiagcddmfapjfbebccolcahfppaimmo" : @"iso_3166_1_et",
      @"aogeahmaehgnkobkmnmkhkimdjpgfgen" : @"iso_3166_1_fk",
      @"molpidmcmbmhbicckmbopbmiojddebke" : @"iso_3166_1_fo",
      @"biobhkionbllnfljaapocfpdmhamedkf" : @"iso_3166_1_fj",
      @"ecneelfoifpgnignhipdebhbkgcphmic" : @"iso_3166_1_fi",
      @"bgifagoclclhhoflocdefiklgodpihog" : @"iso_3166_1_fr",
      @"mhmfclafeemiphfebhnobinkplbmpocm" : @"iso_3166_1_gf",
      @"mjaianjdadeiocokpoanbgjhegficcce" : @"iso_3166_1_pf",
      @"jbjodokafbafhemocebljdblgnfajabi" : @"iso_3166_1_tf",
      @"nchncleokkkkdfgbgmenkpkmhnlbibmg" : @"iso_3166_1_ga",
      @"alebifccfdpcgpandngmalclpbjpaiak" : @"iso_3166_1_gm",
      @"kaikhlldfkdjgddjdkangjobahokefeb" : @"iso_3166_1_ge",
      @"jcncoheihebhhiemmbmpfhkceomfipbj" : @"iso_3166_1_de",
      @"panlkcipneniikplpjnoddnhonjljbdp" : @"iso_3166_1_gh",
      @"pibapallcmncjajdoamgdnmgcbefekgn" : @"iso_3166_1_gi",
      @"ochemplgmlglilaflfjnmngpfmpmjgnb" : @"iso_3166_1_gr",
      @"gjknekmnjalchmfjbecifkoohcmlolhp" : @"iso_3166_1_gl",
      @"kbllnlfaaoonbliendlhmoleflgdekki" : @"iso_3166_1_gd",
      @"keggdlgkcfmmopgnogknokiocjjieapm" : @"iso_3166_1_gp",
      @"mfnbeofelbcabhloibhpklffiifjcdfl" : @"iso_3166_1_gu",
      @"jdhabeecpolnjdiplbcpjlohmlgdjgjh" : @"iso_3166_1_gt",
      @"ncldbgolkmlgfkoignkdjehiadnfmlid" : @"iso_3166_1_gg",
      @"gcfgdkmegcjaceeofigbabmjjiojdgnb" : @"iso_3166_1_gn",
      @"kheejcjggceghjdinbcklehfkobloboc" : @"iso_3166_1_gw",
      @"fehpbhdbjpnaibhncpeedfkogiiajcne" : @"iso_3166_1_gy",
      @"pkpmecljhbjbiicbnfcfgpgmpehldemm" : @"iso_3166_1_ht",
      @"kfjeoeekjccibpockdjcdjbgagaaopdj" : @"iso_3166_1_hm",
      @"ljhceaiogeejahjjblnfaaocgogchpkb" : @"iso_3166_1_va",
      @"llmmfofcojgcignfnaodhdhdhphhmfmf" : @"iso_3166_1_hn",
      @"plpcpclbpkilccbegfbpediidmejaahc" : @"iso_3166_1_hk",
      @"pofhnfhkonpjephlcjlmbjmlikiaddoc" : @"iso_3166_1_hu",
      @"cljplhmgochlncaelcabfgeefebhmfnk" : @"iso_3166_1_is",
      @"kdkakecfnmlfifkhlekmfkmmkpgeckcl" : @"iso_3166_1_in",
      @"lanimmipljlbdnajkabaoiklnpcaoakp" : @"iso_3166_1_id",
      @"mhiehehcmljjmpibmpiadoeefnchmbdm" : @"iso_3166_1_ir",
      @"oejhcomgcgcofdfchkdhkjohnioijofn" : @"iso_3166_1_iq",
      @"fbmfelhaipnlicodibhjkmeafbcgpfnm" : @"iso_3166_1_ie",
      @"blofecpleppfodonanffdbepbiijmklm" : @"iso_3166_1_im",
      @"fiodhmddlgkgajbhohfdmkliikidmaom" : @"iso_3166_1_il",
      @"gjkhegliajlngffafldbadcnpfegmkmb" : @"iso_3166_1_it",
      @"ncfbonfnhophngmkkihoieoclepddfhm" : @"iso_3166_1_jm",
      @"ikolbkmkinegpoedjeklhfnaidmloifj" : @"iso_3166_1_jp",
      @"lfgnglkpngeffaijiomlppnobpchhcgf" : @"iso_3166_1_je",
      @"gnkmfghehkoegoabhndflbdmfnlgeind" : @"iso_3166_1_jo",
      @"jadlfgggcfdhicaoacokfpmccbmedkim" : @"iso_3166_1_kz",
      @"bfhpiebgnciehokapeppcinalnibbnan" : @"iso_3166_1_ke",
      @"dkghhhflbpfknidjbhlophapheggpahk" : @"iso_3166_1_ki",
      @"pnokpaenadbgpjpmlnoamnmpjbjlfoaf" : @"iso_3166_1_kp",
      @"clgbjhhcdihjgbomhpmfdjdiagejadja" : @"iso_3166_1_kr",
      @"ehkeinmpkojiiacjohbalbnhloiaifig" : @"iso_3166_1_kw",
      @"hehalbiboicjbbcfhckdfeijjjppdhij" : @"iso_3166_1_kg",
      @"lhjcndbhldpnapjddfgohdcdmfibgpon" : @"iso_3166_1_la",
      @"pooflbdadogbmjmnnppfmklfjbmoblfa" : @"iso_3166_1_lv",
      @"hkengofpokonjepdafjdeclejledncdj" : @"iso_3166_1_lb",
      @"mdojkinfephdfhbfadcnnfcjfniefael" : @"iso_3166_1_ls",
      @"alenpijagefjamgompebcjhbfhepnphh" : @"iso_3166_1_lr",
      @"mnhglgpnnohpipdeinibpbnlnpledicf" : @"iso_3166_1_ly",
      @"onhaidkdpiboaolhnaddeddfaabomchd" : @"iso_3166_1_li",
      @"aokfbnlokidoepkhilbmfdkdhajkpbli" : @"iso_3166_1_lt",
      @"gnmaofjfninpeccldcmlkbinhhohmbck" : @"iso_3166_1_lu",
      @"ncmdondkaofghlnhiabnfilafhmabong" : @"iso_3166_1_mo",
      @"lapgbedoccnchodbgfmafpkkhlfmcehe" : @"iso_3166_1_mg",
      @"dhmcaoadkmoejegjpjgkjhibioemkfni" : @"iso_3166_1_mw",
      @"dadpenhnclbbkjfbkgkgecknfjggpbmm" : @"iso_3166_1_my",
      @"ggclalmmmmgjcoleeficgnnjkpgeinfd" : @"iso_3166_1_mv",
      @"flocoipmnbpcodjfhmkjecjpbkcmkecp" : @"iso_3166_1_ml",
      @"emckddclmcjoilbadmdjdakabpnkdkhk" : @"iso_3166_1_mt",
      @"cpjafhooepmhnflmjabfeaiopfbljhpo" : @"iso_3166_1_mh",
      @"chbeaiccoofemohdajloflfkblbgdiih" : @"iso_3166_1_mq",
      @"dfmnoondmnbngeilibiicikjenjjeigi" : @"iso_3166_1_mr",
      @"iobofpagkcicpcijjfmnghgppbghnpdo" : @"iso_3166_1_mu",
      @"lcnaekpkllhpljanlibochejknjflodn" : @"iso_3166_1_yt",
      @"dclpeadnefbjogjcamdglgmmbbgnjcob" : @"iso_3166_1_mx",
      @"pjiglaefpchnekpbkbfngjnfhlcmhgbk" : @"iso_3166_1_fm",
      @"paiickhoniddnnlhhdmhjcfemgkgfohn" : @"iso_3166_1_md",
      @"iloglofhibeghkfbocphifnfpccmplgd" : @"iso_3166_1_mc",
      @"pclbpikpdcdondhappcgloeohjgammia" : @"iso_3166_1_mn",
      @"dkjadbekoidbnlmaomlcjjgkofkechlo" : @"iso_3166_1_me",
      @"mknfcplgmgbfkklaiimdakgjbeonapeh" : @"iso_3166_1_ms",
      @"pmbhpljpfciommdigfblnenpdiapdafl" : @"iso_3166_1_ma",
      @"gilieeicpdnkcjbohfhjhpmpjocapbko" : @"iso_3166_1_mz",
      @"bbeoopklmfincipdlffbbphpjefmimmp" : @"iso_3166_1_mm",
      @"paoffgbbehbibcihhmboiaebgojdnibj" : @"iso_3166_1_na",
      @"jpejbbflggaiaflclgomjcolpomjmhlh" : @"iso_3166_1_nr",
      @"ohodaiianeochclnnobadfikohciggno" : @"iso_3166_1_np",
      @"choggjlbfndjppfiidbhmefapnlhcdhe" : @"iso_3166_1_nl",
      @"apmipakgigaapfahiojgjgkfgcdekbpp" : @"iso_3166_1_nc",
      @"dlbokjgcdlhkgfeklggoncjhihaebnai" : @"iso_3166_1_nz",
      @"jajkonoepahongnlnfbfmlnpnkjkchof" : @"iso_3166_1_ni",
      @"mmhmpjfgnhibhfccegfohnibkpooppkn" : @"iso_3166_1_ne",
      @"bhkddokohamnindobkmifljnpgijdjdh" : @"iso_3166_1_ng",
      @"celbcocehclbnblfndjfjleagcbbpooc" : @"iso_3166_1_nu",
      @"bcnnffpigdndbdohgifflckehcoofigc" : @"iso_3166_1_nf",
      @"njlgnoebifbjpafbmnkkchknkinmeljm" : @"iso_3166_1_mk",
      @"cpjjnbhhjohkkmkkplcfkobjfbjlildd" : @"iso_3166_1_mp",
      @"ciibjdmjfejjghmnlonlihnjodfckfbo" : @"iso_3166_1_no",
      @"cobdmgempkofdfhgmbhfckemppmlbjob" : @"iso_3166_1_om",
      @"aiaabcbklimkipbpalfoaehfdebbainb" : @"iso_3166_1_pk",
      @"ejlnmikcbnjpaaolkheodefhahiabjga" : @"iso_3166_1_pw",
      @"iienfoenehmoepjgljgjdkenjedjfjol" : @"iso_3166_1_ps",
      @"aafjalakdldginkbeobaiamnfphcdmko" : @"iso_3166_1_pa",
      @"monkjbjmhjepdcaedlejhmjjjcjpiiaa" : @"iso_3166_1_pg",
      @"aoidaoefdchfhdjfdffjnnlbfepfkadj" : @"iso_3166_1_py",
      @"pmbmbglgbofljclfopjkkompfgedgjhi" : @"iso_3166_1_pe",
      @"ocmnmegmbhbfmdnjoppmlbhfcpmedacn" : @"iso_3166_1_ph",
      @"ccpkbhegiebckfidhnoihgdmddhnmdfh" : @"iso_3166_1_pn",
      @"feeklcgpaolphdiamjaolkkcpbeihkbh" : @"iso_3166_1_pl",
      @"gchnahcajhccobheiedkgdpfboljkhge" : @"iso_3166_1_pt",
      @"bpjdfagamlhoojajkeifbendedaikinl" : @"iso_3166_1_pr",
      @"jicllaljbaldhopinkkegkfpmmnnhmbc" : @"iso_3166_1_qa",
      @"aeglmpapdhfhdicbifhnmaoehffffmie" : @"iso_3166_1_re",
      @"jpapeieehcilkcfpljhopohigdhbnjck" : @"iso_3166_1_ro",
      @"nfcegbjbohhjljcdogkmookngaiplbna" : @"iso_3166_1_ru",
      @"djjoaejcadmjbgadeijpbokipgmolnih" : @"iso_3166_1_rw",
      @"fjefhohmfmokjmnibamjnpiafikmmlef" : @"iso_3166_1_bl",
      @"dpodaelfodkebmgmmdoecleopjggboln" : @"iso_3166_1_sh",
      @"idmipdncpnfbfonogngaimigpbpnenpb" : @"iso_3166_1_kn",
      @"lhlajkngiihbjjaakfgkencpppeahhfi" : @"iso_3166_1_lc",
      @"hihpbgpfcelklhigbkfnbdgjmpbnabmo" : @"iso_3166_1_mf",
      @"cpkbkgenaaododkijfnfmgmpekbcfjcg" : @"iso_3166_1_pm",
      @"bnonnlpingklaggdalihppicgpaghpop" : @"iso_3166_1_vc",
      @"jfckclnlfaekpfklphjagmjiphjcchmj" : @"iso_3166_1_ws",
      @"lneikknijgnijfnpoahmfkefboallgin" : @"iso_3166_1_sm",
      @"djlblammehomffbplemhekjeghekglpc" : @"iso_3166_1_st",
      @"gmhojjgbbfachddbgojljenplnhialka" : @"iso_3166_1_sa",
      @"haalbaecaigldhgnjfmjbedegjipkdfb" : @"iso_3166_1_sn",
      @"dlfdepidpnibdoblimabdmgpobophapn" : @"iso_3166_1_rs",
      @"dmdapbmagehdijbdhbdbfjijgmcppjml" : @"iso_3166_1_sc",
      @"piajfdpbabffhdlmpkaejndbdnohljfn" : @"iso_3166_1_sl",
      @"jilipkheolgjanjhhhdmbaleiiblnepe" : @"iso_3166_1_sg",
      @"igdomgnppdmcglgohoamnpegjelohlkj" : @"iso_3166_1_sx",
      @"obponfmfefkaeehakbehbnnlcbebebhd" : @"iso_3166_1_sk",
      @"dckjbnoilglapbgmniiagempimbaicmn" : @"iso_3166_1_si",
      @"mlbgbnccloeobccglpaachnaabgegcni" : @"iso_3166_1_sb",
      @"hnfmhdkkmcgeppiiohpondjgibepgfeb" : @"iso_3166_1_so",
      @"jadaiaonajphgnbamppeenldepoajgbf" : @"iso_3166_1_za",
      @"ghclfflogdfhnjciileceoofmhkneggp" : @"iso_3166_1_gs",
      @"kkfngpdjfcddimfgkgibaccaoehjplkn" : @"iso_3166_1_ss",
      @"ganmbmiebelpdlnohnabgkkocholelbp" : @"iso_3166_1_es",
      @"gmahgggkpliaoidcaknflpbgpehcjmhc" : @"iso_3166_1_lk",
      @"dhcfofldcefkohnjcnfodlbiakgedidd" : @"iso_3166_1_sd",
      @"khgbibnjdanhnoebnfjgpnfbkohdngph" : @"iso_3166_1_sr",
      @"kchkidfjkghdocdicfpmbckmjfgnlndb" : @"iso_3166_1_sj",
      @"clncjboijmbkcjgkechfhalginbnplpp" : @"iso_3166_1_se",
      @"gnamhdlealpfbanappoephfdjeoehggd" : @"iso_3166_1_ch",
      @"hnhakbhiamjojdoajhebemlajheokinm" : @"iso_3166_1_sy",
      @"jejmkjlhckkijknapfhfoogakgoelhen" : @"iso_3166_1_tw",
      @"nfpgpnagpefhcijfnabpdejiiejplonp" : @"iso_3166_1_tj",
      @"jnlkpmlmfdipllbnjmjomkddplafclch" : @"iso_3166_1_tz",
      @"mjkmkfbpiegjkbeolgpomaloeiiffodm" : @"iso_3166_1_th",
      @"kmdanbbapegbkpjkfdldmekconhnmmmo" : @"iso_3166_1_tl",
      @"alinepjaedjagibhfjcejemabijbohmi" : @"iso_3166_1_tg",
      @"bbobjkhpggphapdpcchkbklglkindkcc" : @"iso_3166_1_tk",
      @"jdkdhebphdakbabdbgefjkdbdoagmdec" : @"iso_3166_1_to",
      @"nbmopmgpfmalleghhbkablkoamofibpk" : @"iso_3166_1_tt",
      @"hgmkfpcpppjheoknjjfpajfmibkndjdf" : @"iso_3166_1_tn",
      @"fahflofbglhemnakgdmillobeencekne" : @"iso_3166_1_tr",
      @"fhbmmefncojhnjhbckninoliclloeeac" : @"iso_3166_1_tm",
      @"hbiblijpgfgphhfoajjmcgdbhmobbfba" : @"iso_3166_1_tc",
      @"kennokhomgefcfjjjhckbiidlhmkinca" : @"iso_3166_1_tv",
      @"bolcbpmofcabjoflcmljongimpbpeagb" : @"iso_3166_1_ug",
      @"enkpeojckjlmehbniegocfffdkanjhef" : @"iso_3166_1_ua",
      @"ajdiilmgienedlgohldjicpcofnckdmn" : @"iso_3166_1_ae",
      @"cmdlemldhabgmejfognbhdejendfeikd" : @"iso_3166_1_gb",
      @"iblokdlgekdjophgeonmanpnjihcjkjj" : @"iso_3166_1_us",
      @"ocikkcmgfagemkpbbkjlngjomkdobgpp" : @"iso_3166_1_um",
      @"cejbfkalcdepkoekifpidloabepihogd" : @"iso_3166_1_uy",
      @"chpbioaimigllimaalbibcmjjcfnbpid" : @"iso_3166_1_uz",
      @"ogbkgicanbpgkemjooahemnoihlihonp" : @"iso_3166_1_vu",
      @"okopabpainkagabcmkfnnchaakhimaoe" : @"iso_3166_1_ve",
      @"jcffalbkohmmfjmgkdcphlimplejkmon" : @"iso_3166_1_vn",
      @"jlfjphoakpnmhpldmdkdhekopbjmkljn" : @"iso_3166_1_vg",
      @"infpagefbmdijbaigeagmldkjnjdhhfa" : @"iso_3166_1_vi",
      @"hefgpgfflbaepfgbafaaadffchekggfg" : @"iso_3166_1_wf",
      @"fjhkoeiglahhpcmgfpalgckcaoaifjkn" : @"iso_3166_1_eh",
      @"cijopjkddpagbkjpnnbjcecfamjbkakp" : @"iso_3166_1_ye",
      @"inmfjchmafaondfnpgffefgbdmmfgenb" : @"iso_3166_1_zm",
      @"fmobbdfaoifmdjonfklmapdliabjdmjp" : @"iso_3166_1_zw",
      @"ijmgabghpbflfadffhpmjklamndnonha" : @"iso_639_1_ab",
      @"hddanjaanmjbdklklpldpgpmbdmaiihb" : @"iso_639_1_aa",
      @"blcjdmhlkddhompnlbjlpccpelipihem" : @"iso_639_1_af",
      @"pecokcgeeiabdlkfkfjpmfldfhhjlmom" : @"iso_639_1_ak",
      @"pgkommhmfkkkfbbcnnfhlkagbodoimjm" : @"iso_639_1_sq",
      @"emopjfcnkbjfedjbfgajdajnkkfekcbl" : @"iso_639_1_am",
      @"knjanagkmnjgjjiekhmhclcbcdbjajmk" : @"iso_639_1_ar",
      @"onjbjcnjheabeghbflckfekjnnjgfabn" : @"iso_639_1_an",
      @"ghgfdmhmhifphaokjfplffppmlfchofm" : @"iso_639_1_hy",
      @"mbcmenffnlanegglgpdgbmmldfpclnkm" : @"iso_639_1_as",
      @"clemenahenllbgeilmkllcpfcfhloebp" : @"iso_639_1_av",
      @"cmggjadifnmfdfeohgomnaggonihmbli" : @"iso_639_1_ae",
      @"fabdaiigiipinlpgkkakcffbhkmfnfek" : @"iso_639_1_ay",
      @"cgpdnipmfmbfceokoadgbliclnoeddef" : @"iso_639_1_az",
      @"hjfgajachhnlgjfjgbjbehgcoecfigap" : @"iso_639_1_bm",
      @"keghklelbpkaiogcinbjnnjideedfmdd" : @"iso_639_1_ba",
      @"lldcgicjgpomllohjncpcjkijjklooji" : @"iso_639_1_eu",
      @"ogmkkmobemflinkielcngdanaojkamcc" : @"iso_639_1_be",
      @"ligbalgipjnajannojamijenlackcioc" : @"iso_639_1_bn",
      @"bkicjkclpdihdjbpajnegckopabcebff" : @"iso_639_1_bh",
      @"dafhhkffcifanfgpjlgejcahkidpbnfj" : @"iso_639_1_bi",
      @"ngnfehahlmgkoclalhldboigojplccbl" : @"iso_639_1_bs",
      @"hkfnnljkbmadknefddjfligobbiagmea" : @"iso_639_1_br",
      @"igdpeoeohlgknkdnjhbijpfeenfiepdc" : @"iso_639_1_bg",
      @"mdacdgffhlidpdmhckokeoajhojeibmp" : @"iso_639_1_my",
      @"keahfifnfebhoaaghffigjnhgbkkaibd" : @"iso_639_1_ca",
      @"fllhkadnpidionapphjicdkfdiloghad" : @"iso_639_1_ch",
      @"eakppbikiomjdnligoccikcdipojiljk" : @"iso_639_1_ce",
      @"ddekfkhnjcpbankekbelkeekibbhmgnh" : @"iso_639_1_ny",
      @"clegognodnfcpbmhpfgbpckebppbaebp" : @"iso_639_1_zh",
      @"obdagonejiaflgbifdloghkllgdjpcdj" : @"iso_639_1_cv",
      @"apmkijnjnhdabkckmkkejgdnbgdglocb" : @"iso_639_1_kw",
      @"gdmbpbmoiogajaogpajfhonmlepcdokn" : @"iso_639_1_co",
      @"amedpgedagedjlkgjajebgecfkldmdfa" : @"iso_639_1_cr",
      @"ncjeinjdknabbgmaijmnipndbggmchff" : @"iso_639_1_hr",
      @"nllaodpgkekajbabhjphhekenlnlpdmd" : @"iso_639_1_cs",
      @"klifniioldbebiedppmbobpdiombacge" : @"iso_639_1_da",
      @"aoljgchlinejchjbbkicamhfdapghahp" : @"iso_639_1_dv",
      @"neglbnegiidighiifljiphcldmgibifn" : @"iso_639_1_nl",
      @"jginkacioplimdjobccplmgiphpjjigl" : @"iso_639_1_dz",
      @"ocilmpijebaopmdifcomolmpigakocmo" : @"iso_639_1_en",
      @"halbpcgpgcafebkldcfhllomekophick" : @"iso_639_1_eo",
      @"onmakacikbbnhmanodpjhijljadlpbda" : @"iso_639_1_et",
      @"bjhkalknmdllcnkcjdjncplkbbeigklb" : @"iso_639_1_ee",
      @"jamflccjbegjmghgaljipcjjbipgojbn" : @"iso_639_1_fo",
      @"gfoibbmiaikelajlipoffiklghlbapjf" : @"iso_639_1_fj",
      @"lbbgedbjaoehfaoklcebbepkbmljanhc" : @"iso_639_1_fi",
      @"ijgkfgmfiinppefbonemjidmkhgbonei" : @"iso_639_1_fr",
      @"anhpkncejedojfopbigplmbfmbhkagao" : @"iso_639_1_ff",
      @"ejioajnkmjfjfbbanmgfbagjbdlfodge" : @"iso_639_1_gl",
      @"hlipecdabdcghhdkhfmhiclaobjjmhbo" : @"iso_639_1_ka",
      @"bbefpembgddgdihpkcidgdgiojjlchji" : @"iso_639_1_de",
      @"hgcnnimnfelflnbdfbdngikednoidhmg" : @"iso_639_1_el",
      @"ebgmbleidclecpicaccgpjdhndholiel" : @"iso_639_1_gn",
      @"mdpcebhdjplkegddkiodmbjcmlnhbpog" : @"iso_639_1_gu",
      @"hpkelamfnimiapfbeeelpfkhkgfoejil" : @"iso_639_1_ht",
      @"khgpojhhoikmhodflkppdcakhbkaojpi" : @"iso_639_1_ha",
      @"biklnlngpkiolabpfdiokhnokoomfldp" : @"iso_639_1_he",
      @"pkmkhkpglegjekkfabaelkbgkfegnmde" : @"iso_639_1_hz",
      @"acdmbpdfmekdichgebponnjloihkdejf" : @"iso_639_1_hi",
      @"cmkgcbedakcdopgjdhbbpjjaodjcdbdp" : @"iso_639_1_ho",
      @"ifbdofecjcadpnkokmdfahhjadcppmko" : @"iso_639_1_hu",
      @"hoeihggfhgnfpdnaeocfpmoelcenhfla" : @"iso_639_1_ia",
      @"gbmolmcnbhegkhjhbhjehcpbjonjlgfg" : @"iso_639_1_id",
      @"fioklomfllflofcdiklpgabemclgkkdh" : @"iso_639_1_ie",
      @"oiihbhoknlbjonghmcplpbpkcdeinlfg" : @"iso_639_1_ga",
      @"nchaailfhkbnlnaobgjmoamdfnclhdoo" : @"iso_639_1_ig",
      @"fbdjobfnceggaokdnlebbaplnhednlhl" : @"iso_639_1_ik",
      @"nkajjfpapgfhlcpmmoipafbfnnmojaep" : @"iso_639_1_io",
      @"dhhkjdedjghadoekfklpheblplmlpdec" : @"iso_639_1_is",
      @"apaklaabmoggbjglopdnboibkipdindg" : @"iso_639_1_it",
      @"eociikjclgmjinkgeoehleofopogehja" : @"iso_639_1_iu",
      @"anbffbdnbfabjafoemkhoaelpodojknn" : @"iso_639_1_ja",
      @"jdfafcdnmjeadcohbmjeeijgheobldng" : @"iso_639_1_jv",
      @"jknflnmanienedkoeoginjmbpmkpclki" : @"iso_639_1_kl",
      @"nggdckgfnooehkpnmjdodbknekmhcdeg" : @"iso_639_1_kn",
      @"mhobnohaonkecggnifnffofnihbakjic" : @"iso_639_1_kr",
      @"hhejjhncnckfmpkpkhkbknekhkepcill" : @"iso_639_1_ks",
      @"ljllkgialkdmamlacenmjkhjmimndfil" : @"iso_639_1_kk",
      @"dhigelnhdnmhffomicjdmdhefhedgijm" : @"iso_639_1_km",
      @"jcfelbpkigpapilbogpidbfdffgdfafe" : @"iso_639_1_ki",
      @"ookcfboifajbeojnnebiaoplifgiinof" : @"iso_639_1_rw",
      @"njimokpabbaelbbpohoflcbjhdgonbmf" : @"iso_639_1_ky",
      @"danmahhfdmncbiipbefmjdkembceajdk" : @"iso_639_1_kv",
      @"lcahllbcfbhghpjjocdhmilokfbpbekn" : @"iso_639_1_kg",
      @"deadocmlegcgnokbhhpgblpofkpkeocg" : @"iso_639_1_ko",
      @"hfboaaehpnfpnpompagbamoknlnladfn" : @"iso_639_1_ku",
      @"cppbcboljlmfdgeehadijemhkifhcpnl" : @"iso_639_1_kj",
      @"knnpciecjoakhokllbgioaceglldlgan" : @"iso_639_1_la",
      @"chnbfebpflegknnjiikofmnebcbphead" : @"iso_639_1_lb",
      @"hkfkdejbdionnjdhgfhpkcmknmamddde" : @"iso_639_1_lg",
      @"nnbaaidlgckbmfdlnioepikbcmjmbadb" : @"iso_639_1_li",
      @"dedchpogdooaakplmpjobpkiggalnlif" : @"iso_639_1_ln",
      @"alaghdpgppakjfjcipokbaempndnglfk" : @"iso_639_1_lo",
      @"copfpkggfedomijbmceepmahananponb" : @"iso_639_1_lt",
      @"ljambfnmibabkhcpgppfblodipceahab" : @"iso_639_1_lu",
      @"lflklgkjbemnncejeanindnikiaicpod" : @"iso_639_1_lv",
      @"lkcfaihllkinjhmdjgaccjhgdobabpbj" : @"iso_639_1_gv",
      @"anikcbbbhcobgockdcemaboadbdcnhlg" : @"iso_639_1_mk",
      @"gaompbafbaolhddgoafjhkgmnjpjpbip" : @"iso_639_1_mg",
      @"pppjaeenalohmnhjpemhdkkkghfddbfp" : @"iso_639_1_ms",
      @"papnfbjjblebaaeenodbmllepiijfmhn" : @"iso_639_1_ml",
      @"jmakhmnfhlhioagnmgnakhigadgkpkcl" : @"iso_639_1_mt",
      @"gakleannelcniohpkolmbgfpnkjlbnll" : @"iso_639_1_mi",
      @"lmpaafjmphnclpkfcejgjbnieahkgemg" : @"iso_639_1_mr",
      @"fehmnfinbijjdacjgeofhojfdjhbehic" : @"iso_639_1_mh",
      @"aadlcogkfhaficfijoolhlajkngeecea" : @"iso_639_1_mn",
      @"edcpinkoiknmjafcdpolkkeiieddmbab" : @"iso_639_1_na",
      @"ooncphbdmekmhldbojgoboebnongkpic" : @"iso_639_1_nv",
      @"kemfolmmdooeepfhbpiemnnekfjlbnnd" : @"iso_639_1_nd",
      @"mihcpmkclenpfgielcipmdbcfpncfojc" : @"iso_639_1_ne",
      @"jpleaamlgnfhfemdcfmbknnhcbfnglkh" : @"iso_639_1_ng",
      @"gbigbbblbdmfhbpobcliehihdedicjfl" : @"iso_639_1_nb",
      @"fnmjhgcaloniglpngailbaillhbenela" : @"iso_639_1_nn",
      @"fgicknpghikljlipmfibgghcdikjfjfj" : @"iso_639_1_no",
      @"nodfcenkjehpafmbmgnapoieilnoijap" : @"iso_639_1_ii",
      @"dghnlalnaoekcligakadcmmioaieangj" : @"iso_639_1_nr",
      @"fbfeebiglbpbmgbefgmijdbcchmdfdhm" : @"iso_639_1_oc",
      @"gdkeabmpgllapbjgifgfmlfelpdlkapj" : @"iso_639_1_oj",
      @"fnhldinjahkdbngcnjfcmidhpjedinbg" : @"iso_639_1_cu",
      @"aegokocmijocdgiddgjbjkdfiheijfpl" : @"iso_639_1_om",
      @"amkpggbpieoafkbmkijjnefikhjjfogn" : @"iso_639_1_or",
      @"adccmiokahgjhdbmhldbnkkplgcgfkpp" : @"iso_639_1_os",
      @"ghikcfknmlkdjiiljfpgpmcfjinpollk" : @"iso_639_1_pa",
      @"hinecgnhkigghpncjnflaokadaclcfpm" : @"iso_639_1_pi",
      @"blaocfojieebnkolagngecdhnipocicj" : @"iso_639_1_fa",
      @"fojhemdeemkcacelmecilmibcjallejo" : @"iso_639_1_pl",
      @"fikmpfipjepnnhiolongfjimedlhaemk" : @"iso_639_1_ps",
      @"fimpfhgllgkaekhbpkakjchdogecjflf" : @"iso_639_1_pt",
      @"ndlciiakjgfcefimfjlfcjgohlgidpnc" : @"iso_639_1_qu",
      @"nlabdknfkecjaechkekdlkgnapljpfla" : @"iso_639_1_rm",
      @"piebpodmljdloocefikhekfjajloneij" : @"iso_639_1_rn",
      @"ohhkknigpeehdnkeccopplflnodppkme" : @"iso_639_1_ro",
      @"jajlkohoekhghdbclekclenlahcjplec" : @"iso_639_1_ru",
      @"inkmdnaeojfdngbdkbinoinflfahcjfc" : @"iso_639_1_sa",
      @"golaphmkhjkdmcakpigbjhneagiapkfh" : @"iso_639_1_sc",
      @"kcmiboiihhehobbffjebhgadbalknboh" : @"iso_639_1_sd",
      @"cmomlghkjichjgbkakaoenfenincefnj" : @"iso_639_1_se",
      @"mfaajikelgfodgcgnldapbgjdbncmibc" : @"iso_639_1_sm",
      @"gndfhmmkadfdhmchhljmcdhlicdmmlbn" : @"iso_639_1_sg",
      @"pdgppejghdoknllcnfikoaloobopajjo" : @"iso_639_1_sr",
      @"djmefhmnkffabdodgcfjmgffpindaaak" : @"iso_639_1_gd",
      @"bdepmnbdekgdgjimffimkfeoggmnlbbf" : @"iso_639_1_sn",
      @"mogllbjhcnfhcejalaikleeogjmmfkdm" : @"iso_639_1_si",
      @"gnhdcgmlejfbcccdjdhjalacjcimlkjh" : @"iso_639_1_sk",
      @"jifgjineejhedlmjnkcijoincbhelicp" : @"iso_639_1_sl",
      @"doclofgiadjildnifgkajdlpngijbpop" : @"iso_639_1_so",
      @"mgdaglmilmjenimbkdmneckfbphfllic" : @"iso_639_1_st",
      @"elecgkckipdmnkkgndidemmdhdcdfhnp" : @"iso_639_1_es",
      @"aondicpcneldjpbfemaimbpomgaepjhg" : @"iso_639_1_su",
      @"ccmmjlklhnoojaganaecljeecenhafok" : @"iso_639_1_sw",
      @"khbhchcpljcejlmijghlabpcmlkkfnid" : @"iso_639_1_ss",
      @"lnhckckgfdgjgkoelimnmpbnnognpmfb" : @"iso_639_1_sv",
      @"nbmbpelgpalcgdghkeafabljjbhmalhf" : @"iso_639_1_ta",
      @"nonmahhknjgpnoamcdihefcbpdioimbh" : @"iso_639_1_te",
      @"olopfkdcklijkianjbegdegilmhdgpcj" : @"iso_639_1_tg",
      @"jllmphacilbjnfngcojfgmiimipclfbm" : @"iso_639_1_th",
      @"hkeoedmbihkkglaobeembiogeofffpop" : @"iso_639_1_ti",
      @"ijgcgakmmgjaladckdppemjgdnjlcgpo" : @"iso_639_1_bo",
      @"liddcpbnodlgenmbmmfchepoebgfondk" : @"iso_639_1_tk",
      @"mocihammffaleonaomjleikagemilaoj" : @"iso_639_1_tl",
      @"gjinficpofcocgaaogaiimhacbfkmjmj" : @"iso_639_1_tn",
      @"hhliclmbfpdlpkdhmpkleicjnemheeea" : @"iso_639_1_to",
      @"edjnpechdkjgcfjepfnnabdkcfcfllpd" : @"iso_639_1_tr",
      @"nhbpjehmiofogaicflcjhcfdmmkgbohp" : @"iso_639_1_ts",
      @"mmmembcojnkgfclnogjfeeiicmiijcnk" : @"iso_639_1_tt",
      @"ldjaelhegioonaebajlgfnhcipdajfeo" : @"iso_639_1_tw",
      @"fnokionjijmckgggmhogifnocpabdafk" : @"iso_639_1_ty",
      @"ohcnbpfpchlcjchnljcdjebcjdbneecj" : @"iso_639_1_ug",
      @"pbegjfplhidokoonohceljofcpbojglg" : @"iso_639_1_uk",
      @"jaggpekahffhnhhchdkpnigfmdlhenpo" : @"iso_639_1_ur",
      @"jephmoboccidmbemhjckbcagagijgcef" : @"iso_639_1_uz",
      @"mbhiljiiffkobikkoechkpeaopagfhep" : @"iso_639_1_ve",
      @"pbjakpdfjkmcajeobebemnjglbjiniln" : @"iso_639_1_vi",
      @"bfljdbgfmdjgbomhiaoeoleidbfcmmpn" : @"iso_639_1_vo",
      @"fmiofedgokpciaklgakbnminmmkocgnd" : @"iso_639_1_wa",
      @"gpfmbdepojhpjlaelmnlbgginpgnbmfd" : @"iso_639_1_cy",
      @"mhdpccgjfkfkdbbpapbgcahhknmbdnjn" : @"iso_639_1_wo",
      @"eahefjeohmofagkliaddkmokbecfhclm" : @"iso_639_1_fy",
      @"gjigddoamjemfcahionjikmlfijoiecf" : @"iso_639_1_xh",
      @"jhnklldjooclfmgpkipaemehnngabckf" : @"iso_639_1_yi",
      @"fjfbodkpnkomodlcanacakhcfmjjgkdf" : @"iso_639_1_yo",
      @"bncbapkadghlbephbogcmomlecfmdhnb" : @"iso_639_1_za",
      @"dhlnknppkgfgehmmipicnlplhjgpnmnh" : @"iso_639_1_zu"
    };
  });

  return _paths;
}

- (void)recordP2AEvents:(const std::vector<std::string>&)events {
  // Not needed on iOS
}

- (void)addTrainingSample:
    (std::vector<brave_federated::mojom::CovariateInfoPtr>)training_sample {
  // Not needed on iOS
}

@end
