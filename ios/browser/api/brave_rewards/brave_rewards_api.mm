/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/brave_rewards/brave_rewards_api.h"

#import <UIKit/UIKit.h>

#include <optional>

#include "base/base64.h"
#include "base/containers/flat_map.h"
#include "base/ios/ios_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/types/cxx23_to_underlying.h"
#include "base/values.h"
#include "brave/build/ios/mojom/cpp_transformations.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"
#include "brave/components/brave_rewards/core/common/remote_worker.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_database.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#import "brave/ios/browser/api/brave_rewards/rewards.mojom.objc+private.h"
#import "brave/ios/browser/api/brave_rewards/rewards_client_bridge.h"
#import "brave/ios/browser/api/brave_rewards/rewards_client_ios.h"
#import "brave/ios/browser/api/brave_rewards/rewards_notification.h"
#import "brave/ios/browser/api/brave_rewards/rewards_observer.h"
#import "brave/ios/browser/api/common/common_operations.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/origin.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using brave_rewards::internal::RemoteWorker;

class RewardsDatabaseWorker
    : public RemoteWorker<brave_rewards::mojom::RewardsDatabase> {
 public:
  RewardsDatabaseWorker() : RemoteWorker(CreateTaskRunner()) {}
  ~RewardsDatabaseWorker() = default;

 private:
  static scoped_refptr<base::SequencedTaskRunner> CreateTaskRunner() {
    return base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  }
};

class RewardsEngineWorker
    : public RemoteWorker<brave_rewards::mojom::RewardsEngine> {
 public:
  RewardsEngineWorker() : RemoteWorker(CreateTaskRunner()) {}
  ~RewardsEngineWorker() = default;

 private:
  static scoped_refptr<base::SequencedTaskRunner> CreateTaskRunner() {
    // The `WithBaseSyncPrimitives` flag is required due to the usage of [Sync]
    // interface method calls by the Rewards engine.
    return base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::WithBaseSyncPrimitives(),
         base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  }
};

}  // namespace

#define LLOG(verbose_level, format, ...)                  \
  [self log:(__FILE__)                                    \
       line:(__LINE__)verboseLevel:(verbose_level)message \
           :base::SysNSStringToUTF8(                      \
                [NSString stringWithFormat:(format), ##__VA_ARGS__])]

NSString* const BraveRewardsErrorDomain = @"BraveRewardsErrorDomain";
NSNotificationName const BraveRewardsNotificationAdded =
    @"BATBraveRewardsNotificationAdded";

BraveGeneralRewardsNotificationID const
    BATBraveGeneralRewardsNotificationIDWalletDisconnected =
        @"wallet_disconnected";

static NSString* const kContributionQueueAutoincrementID =
    @"BATContributionQueueAutoincrementID";

static NSString* const kExternalWalletsPrefKey = @"external_wallets";
static NSString* const kTransferFeesPrefKey = @"transfer_fees";

/// ---

@interface BraveRewardsAPI () <RewardsClientBridge> {
  std::unique_ptr<RewardsClientIOS> _rewardsClient;
  RewardsEngineWorker _rewardsEngine;
  RewardsDatabaseWorker _rewardsDatabase;
}

@property(nonatomic, copy) NSString* storagePath;
@property(nonatomic) BraveRewardsRewardsParameters* rewardsParameters;
@property(nonatomic) BraveRewardsBalance* balance;
@property(nonatomic) dispatch_queue_t fileWriteThread;
@property(nonatomic) NSMutableDictionary<NSString*, NSString*>* state;
@property(nonatomic) BraveCommonOperations* commonOps;
@property(nonatomic) NSMutableDictionary<NSString*, __kindof NSObject*>* prefs;

@property(nonatomic) NSHashTable<RewardsObserver*>* observers;

@property(nonatomic, getter=isInitialized) BOOL initialized;
@property(nonatomic) BOOL initializing;
@property(nonatomic) BOOL dataMigrationFailed;
@property(nonatomic) BraveRewardsResult initializationResult;
@property(nonatomic, getter=isLoadingPublisherList) BOOL loadingPublisherList;
@property(nonatomic, getter=isInitializingWallet) BOOL initializingWallet;

/// Temporary blocks

@end

@implementation BraveRewardsAPI

- (instancetype)initWithStateStoragePath:(NSString*)path {
  if ((self = [super init])) {
    self.storagePath = path;
    self.commonOps = [[BraveCommonOperations alloc] initWithStoragePath:path];
    self.state = [[NSMutableDictionary alloc]
                     initWithContentsOfFile:self.randomStatePath]
                     ?: [[NSMutableDictionary alloc] init];
    self.fileWriteThread =
        dispatch_queue_create("com.rewards.file-write", DISPATCH_QUEUE_SERIAL);
    self.observers = [NSHashTable weakObjectsHashTable];

    self.prefs =
        [[NSMutableDictionary alloc] initWithContentsOfFile:[self prefsPath]];
    if (!self.prefs) {
      self.prefs = [[NSMutableDictionary alloc] init];
    }

    NSString* walletProviderRegionsKey = @"parameters.wallet_provider_regions";
    if (!self.prefs[walletProviderRegionsKey] ||
        ![self.prefs[walletProviderRegionsKey] isKindOfClass:NSString.class]) {
      self.prefs[walletProviderRegionsKey] = @"{}";
    }

    _rewardsClient = std::make_unique<RewardsClientIOS>(self);

    _rewardsEngine.BindRemote<brave_rewards::internal::RewardsEngine>(
        _rewardsClient->MakeRemote(),
        [self handleFlags:brave_rewards::RewardsFlags::ForCurrentProcess()]);

    _rewardsDatabase.BindRemote<brave_rewards::internal::RewardsDatabase>(
        base::FilePath([self rewardsDatabasePath].UTF8String));

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
  }
  return self;
}

- (void)dealloc {
  [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (brave_rewards::mojom::RewardsEngineOptions)handleFlags:
    (const brave_rewards::RewardsFlags&)flags {
  brave_rewards::mojom::RewardsEngineOptions options;
  if (flags.environment) {
    switch (*flags.environment) {
      case brave_rewards::RewardsFlags::Environment::kDevelopment:
        options.environment = brave_rewards::mojom::Environment::kDevelopment;
        break;
      case brave_rewards::RewardsFlags::Environment::kStaging:
        options.environment = brave_rewards::mojom::Environment::kStaging;
        break;
      case brave_rewards::RewardsFlags::Environment::kProduction:
        options.environment = brave_rewards::mojom::Environment::kProduction;
        break;
    }
  }

  if (flags.reconcile_interval) {
    options.reconcile_interval = *flags.reconcile_interval;
  }

  if (flags.retry_interval) {
    options.retry_interval = *flags.retry_interval;
  }

  return options;
}

- (void)initializeRewardsService:(nullable void (^)())completion {
  if (self.initialized || self.initializing) {
    return;
  }
  self.initializing = YES;

  _rewardsEngine->Initialize(
      base::BindOnce(^(brave_rewards::mojom::Result result) {
        self.initialized =
            (result == brave_rewards::mojom::Result::OK ||
             result == brave_rewards::mojom::Result::NO_LEGACY_STATE ||
             result == brave_rewards::mojom::Result::NO_PUBLISHER_STATE);
        self.initializing = NO;
        if (self.initialized) {
          [self getRewardsParameters:nil];
          [self fetchBalance:nil];
        } else {
          LLOG(0, @"Rewards Initialization Failed with error: %d",
               base::to_underlying(result));
        }
        self.initializationResult = static_cast<BraveRewardsResult>(result);
        if (completion) {
          completion();
        }
        for (RewardsObserver* observer in [self.observers copy]) {
          if (observer.walletInitalized) {
            observer.walletInitalized(self.initializationResult);
          }
        }
      }));
}

- (NSString*)rewardsDatabasePath {
  return [self.storagePath stringByAppendingPathComponent:@"Rewards.db"];
}

- (void)resetRewardsDatabase {
  const auto dbPath = [self rewardsDatabasePath];
  [NSFileManager.defaultManager removeItemAtPath:dbPath error:nil];
  [NSFileManager.defaultManager
      removeItemAtPath:[dbPath stringByAppendingString:@"-journal"]
                 error:nil];
  _rewardsDatabase.BindRemote<brave_rewards::internal::RewardsDatabase>(
      base::FilePath(base::SysNSStringToUTF8(dbPath)));
}

- (NSString*)randomStatePath {
  return
      [self.storagePath stringByAppendingPathComponent:@"random_state.plist"];
}

- (NSString*)prefsPath {
  return [self.storagePath stringByAppendingPathComponent:@"ledger_pref.plist"];
}

- (void)savePrefs {
  NSDictionary* prefs = [self.prefs copy];
  NSString* path = [[self prefsPath] copy];
  dispatch_async(self.fileWriteThread, ^{
    [prefs writeToURL:[NSURL fileURLWithPath:path isDirectory:NO] error:nil];
  });
}

#pragma mark - Observers

- (void)addObserver:(RewardsObserver*)observer {
  [self.observers addObject:observer];
}

- (void)removeObserver:(RewardsObserver*)observer {
  [self.observers removeObject:observer];
}

#pragma mark - Wallet

- (void)createWallet:(void (^)(NSError* _Nullable))completion {
  // Results that can come from CreateRewardsWallet():
  //   - OK: Good to go
  //   - ERROR: Already initialized
  //   - BAD_REGISTRATION_RESPONSE: Request credentials call failure or
  //   malformed data
  //   - REGISTRATION_VERIFICATION_FAILED: Missing master user token
  self.initializingWallet = YES;
  _rewardsEngine->CreateRewardsWallet(
      "", base::BindOnce(^(
              brave_rewards::mojom::CreateRewardsWalletResult create_result) {
        brave_rewards::mojom::Result result =
            create_result ==
                    brave_rewards::mojom::CreateRewardsWalletResult::kSuccess
                ? brave_rewards::mojom::Result::OK
                : brave_rewards::mojom::Result::FAILED;

        NSError* error = nil;
        if (result != brave_rewards::mojom::Result::OK) {
          std::map<brave_rewards::mojom::Result, std::string> errorDescriptions{
              {brave_rewards::mojom::Result::FAILED,
               "The wallet was already initialized"},
              {brave_rewards::mojom::Result::BAD_REGISTRATION_RESPONSE,
               "Request credentials call failure or malformed data"},
              {brave_rewards::mojom::Result::REGISTRATION_VERIFICATION_FAILED,
               "Missing master user token from registered persona"},
          };
          NSDictionary* userInfo = @{};
          const auto description =
              errorDescriptions[static_cast<brave_rewards::mojom::Result>(
                  result)];
          if (description.length() > 0) {
            userInfo = @{
              NSLocalizedDescriptionKey : base::SysUTF8ToNSString(description)
            };
          }
          error = [NSError errorWithDomain:BraveRewardsErrorDomain
                                      code:static_cast<NSInteger>(result)
                                  userInfo:userInfo];
        }

        self.initializingWallet = NO;

        if (completion) {
          completion(error);
        }

        for (RewardsObserver* observer in [self.observers copy]) {
          if (observer.walletInitalized) {
            observer.walletInitalized(static_cast<BraveRewardsResult>(result));
          }
        }
      }));
}

- (void)currentWalletInfo:
    (void (^)(BraveRewardsRewardsWallet* _Nullable wallet))completion {
  _rewardsEngine->GetRewardsWallet(
      base::BindOnce(^(brave_rewards::mojom::RewardsWalletPtr wallet) {
        const auto bridgedWallet = wallet.get() != nullptr
                                       ? [[BraveRewardsRewardsWallet alloc]
                                             initWithRewardsWallet:*wallet]
                                       : nil;
        completion(bridgedWallet);
      }));
}

- (void)getRewardsParameters:
    (void (^)(BraveRewardsRewardsParameters* _Nullable))completion {
  _rewardsEngine->GetRewardsParameters(
      base::BindOnce(^(brave_rewards::mojom::RewardsParametersPtr info) {
        if (info) {
          self.rewardsParameters = [[BraveRewardsRewardsParameters alloc]
              initWithRewardsParametersPtr:std::move(info)];
        } else {
          self.rewardsParameters = nil;
        }
        if (completion) {
          completion(self.rewardsParameters);
        }
      }));
}

- (void)fetchBalance:(void (^)(BraveRewardsBalance* _Nullable))completion {
  _rewardsEngine->FetchBalance(
      base::BindOnce(^(brave_rewards::mojom::BalancePtr balance) {
        if (balance) {
          self.balance = [[BraveRewardsBalance alloc]
              initWithBalancePtr:std::move(balance)];
        }
        if (completion) {
          completion(self.balance);
        }
      }));
}

- (void)legacyWallet:
    (brave_rewards::mojom::RewardsEngineClient::GetLegacyWalletCallback)
        callback {
  NSDictionary* externalWallets =
      self.prefs[kExternalWalletsPrefKey] ?: [[NSDictionary alloc] init];
  std::string wallet;
  NSData* data = [NSJSONSerialization dataWithJSONObject:externalWallets
                                                 options:0
                                                   error:nil];
  if (data != nil) {
    NSString* dataString = [[NSString alloc] initWithData:data
                                                 encoding:NSUTF8StringEncoding];
    if (dataString != nil) {
      wallet = base::SysNSStringToUTF8(dataString);
    }
  }
  std::move(callback).Run(wallet);
}

#pragma mark - Publishers

- (void)listActivityInfoFromStart:(unsigned int)start
                            limit:(unsigned int)limit
                           filter:(BraveRewardsActivityInfoFilter*)filter
                       completion:
                           (void (^)(NSArray<BraveRewardsPublisherInfo*>*))
                               completion {
    auto cppFilter = filter ? filter.cppObjPtr
                            : brave_rewards::mojom::ActivityInfoFilter::New();
    if (filter.excluded == BraveRewardsExcludeFilterFilterExcluded) {
      _rewardsEngine->GetExcludedList(base::BindOnce(
          ^(std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
            const auto publishers = NSArrayFromVector(
                &list, ^BraveRewardsPublisherInfo*(
                    const brave_rewards::mojom::PublisherInfoPtr& info) {
                  return [[BraveRewardsPublisherInfo alloc]
                      initWithPublisherInfo:*info];
                });
            completion(publishers);
          }));
    } else {
      _rewardsEngine->GetActivityInfoList(
          start, limit, std::move(cppFilter),
          base::BindOnce(
              ^(std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
                const auto publishers = NSArrayFromVector(
                    &list, ^BraveRewardsPublisherInfo*(
                        const brave_rewards::mojom::PublisherInfoPtr& info) {
                      return [[BraveRewardsPublisherInfo alloc]
                          initWithPublisherInfo:*info];
                    });
                completion(publishers);
              }));
    }
}

- (void)fetchPublisherActivityFromURL:(NSURL*)URL
                           faviconURL:(nullable NSURL*)faviconURL
                        publisherBlob:(nullable NSString*)publisherBlob
                                tabId:(uint64_t)tabId {
  if (!URL.absoluteString) {
    return;
  }

  GURL parsedUrl(base::SysNSStringToUTF8(URL.absoluteString));

  if (!parsedUrl.is_valid()) {
    return;
  }

  url::Origin origin = url::Origin::Create(parsedUrl);
  std::string baseDomain = GetDomainAndRegistry(
      origin.host(),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  if (baseDomain == "") {
    return;
  }

  brave_rewards::mojom::VisitDataPtr visitData =
      brave_rewards::mojom::VisitData::New();
  visitData->domain = visitData->name = baseDomain;
  visitData->path = parsedUrl.PathForRequest();
  visitData->url = origin.Serialize();

  if (faviconURL.absoluteString) {
    visitData->favicon_url = base::SysNSStringToUTF8(faviconURL.absoluteString);
  }

  std::string blob = std::string();
  if (publisherBlob) {
    blob = base::SysNSStringToUTF8(publisherBlob);
  }

  _rewardsEngine->GetPublisherActivityFromUrl(tabId, std::move(visitData),
                                              blob);
}

- (void)refreshPublisherWithId:(NSString*)publisherId
                    completion:(void (^)(BraveRewardsPublisherStatus status))
                                   completion {
  if (self.loadingPublisherList) {
    completion(BraveRewardsPublisherStatusNotVerified);
    return;
  }
  _rewardsEngine->RefreshPublisher(
      base::SysNSStringToUTF8(publisherId),
      base::BindOnce(^(brave_rewards::mojom::PublisherStatus status) {
        completion(static_cast<BraveRewardsPublisherStatus>(status));
      }));
}

#pragma mark - Tips

- (void)listRecurringTips:
    (void (^)(NSArray<BraveRewardsPublisherInfo*>*))completion {
  _rewardsEngine->GetRecurringTips(base::BindOnce(
      ^(std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
        const auto publishers = NSArrayFromVector(
            &list, ^BraveRewardsPublisherInfo*(
                const brave_rewards::mojom::PublisherInfoPtr& info) {
              return [[BraveRewardsPublisherInfo alloc]
                  initWithPublisherInfo:*info];
            });
        completion(publishers);
      }));
}

- (void)removeRecurringTipForPublisherWithId:(NSString*)publisherId {
  _rewardsEngine->RemoveRecurringTip(base::SysNSStringToUTF8(publisherId),
                                     base::DoNothing());
}

#pragma mark - Reconcile

- (void)onReconcileComplete:(brave_rewards::mojom::Result)result
               contribution:
                   (brave_rewards::mojom::ContributionInfoPtr)contribution {
  // TODO we changed from probi to amount, so from string to double
  if (result == brave_rewards::mojom::Result::OK) {
    [self fetchBalance:nil];
  }
}

#pragma mark - Misc

- (void)rewardsInternalInfo:
    (void (^)(BraveRewardsRewardsInternalsInfo* _Nullable info))completion {
  _rewardsEngine->GetRewardsInternalsInfo(
      base::BindOnce(^(brave_rewards::mojom::RewardsInternalsInfoPtr info) {
        auto bridgedInfo = info.get() != nullptr
                               ? [[BraveRewardsRewardsInternalsInfo alloc]
                                     initWithRewardsInternalsInfo:*info.get()]
                               : nil;
        completion(bridgedInfo);
      }));
}

- (void)allContributions:
    (void (^)(NSArray<BraveRewardsContributionInfo*>* contributions))
        completion {
  _rewardsEngine->GetAllContributions(base::BindOnce(
      ^(std::vector<brave_rewards::mojom::ContributionInfoPtr> list) {
        const auto convetedList = NSArrayFromVector(
            &list, ^BraveRewardsContributionInfo*(
                const brave_rewards::mojom::ContributionInfoPtr& info) {
              return [[BraveRewardsContributionInfo alloc]
                  initWithContributionInfo:*info];
            });
        completion(convetedList);
      }));
}

- (void)fetchAutoContributeProperties:
    (void (^)(BraveRewardsAutoContributeProperties* _Nullable properties))
        completion {
  _rewardsEngine->GetAutoContributeProperties(base::BindOnce(
      ^(brave_rewards::mojom::AutoContributePropertiesPtr props) {
        auto properties =
            props.get() != nullptr
                ? [[BraveRewardsAutoContributeProperties alloc]
                      initWithAutoContributePropertiesPtr:std::move(props)]
                : nil;
        completion(properties);
      }));
}

#pragma mark - Reporting

- (void)setSelectedTabId:(UInt32)selectedTabId {
  if (!self.initialized) {
    return;
  }

  const auto time = [[NSDate date] timeIntervalSince1970];
  if (_selectedTabId != selectedTabId) {
    _rewardsEngine->OnHide(_selectedTabId, time);
  }
  _selectedTabId = selectedTabId;
  if (_selectedTabId > 0) {
    _rewardsEngine->OnShow(selectedTabId, time);
  }
}

- (void)applicationDidBecomeActive {
  if (!self.initialized) {
    return;
  }

  const auto time = [[NSDate date] timeIntervalSince1970];
  _rewardsEngine->OnForeground(self.selectedTabId, time);
}

- (void)applicationDidBackground {
  if (!self.initialized) {
    return;
  }

  const auto time = [[NSDate date] timeIntervalSince1970];
  _rewardsEngine->OnBackground(self.selectedTabId, time);
}

- (void)reportLoadedPageWithURL:(NSURL*)url tabId:(UInt32)tabId {
  if (!self.initialized) {
    return;
  }

  const auto time = [[NSDate date] timeIntervalSince1970];

  GURL parsedUrl(base::SysNSStringToUTF8(url.absoluteString));
  url::Origin origin = url::Origin::Create(parsedUrl);
  const std::string baseDomain = GetDomainAndRegistry(
      origin.host(),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  if (baseDomain == "") {
    return;
  }

  const std::string publisher_url = origin.scheme() + "://" + baseDomain + "/";

  brave_rewards::mojom::VisitDataPtr data =
      brave_rewards::mojom::VisitData::New();
  data->name = baseDomain;
  data->domain = origin.host();
  data->path = parsedUrl.path();
  data->tab_id = tabId;
  data->url = publisher_url;

  _rewardsEngine->OnLoad(std::move(data), time);
}

- (void)reportXHRLoad:(NSURL*)url
                tabId:(UInt32)tabId
        firstPartyURL:(NSURL*)firstPartyURL
          referrerURL:(NSURL*)referrerURL {
  if (!self.initialized) {
    return;
  }

    base::flat_map<std::string, std::string> partsMap;
    const auto urlComponents = [[NSURLComponents alloc] initWithURL:url
                                            resolvingAgainstBaseURL:NO];
    for (NSURLQueryItem* item in urlComponents.queryItems) {
      std::string value =
          item.value != nil ? base::SysNSStringToUTF8(item.value) : "";
      partsMap[base::SysNSStringToUTF8(item.name)] = value;
    }

    auto visit = brave_rewards::mojom::VisitData::New();
    visit->path = base::SysNSStringToUTF8(url.absoluteString);
    visit->tab_id = tabId;

    std::string ref = referrerURL != nil
                          ? base::SysNSStringToUTF8(referrerURL.absoluteString)
                          : "";
    std::string fpu =
        firstPartyURL != nil
            ? base::SysNSStringToUTF8(firstPartyURL.absoluteString)
            : "";

    _rewardsEngine->OnXHRLoad(tabId,
                              base::SysNSStringToUTF8(url.absoluteString),
                              partsMap, fpu, ref, std::move(visit));
}

- (void)reportTabNavigationOrClosedWithTabId:(UInt32)tabId {
  if (!self.initialized) {
    return;
  }

  const auto time = [[NSDate date] timeIntervalSince1970];
  _rewardsEngine->OnUnload(tabId, time);
}

#pragma mark - Preferences

- (void)setMinimumVisitDuration:(int)minimumVisitDuration {
  _rewardsEngine->SetPublisherMinVisitTime(minimumVisitDuration);
}

- (void)setMinimumNumberOfVisits:(int)minimumNumberOfVisits {
  _rewardsEngine->SetPublisherMinVisits(minimumNumberOfVisits);
}

- (void)setContributionAmount:(double)contributionAmount {
  _rewardsEngine->SetAutoContributionAmount(contributionAmount);
}

- (void)setAutoContributeEnabled:(bool)autoContributeEnabled {
  _rewardsEngine->SetAutoContributeEnabled(autoContributeEnabled);
}

- (void)setBooleanState:(const std::string&)name
                  value:(bool)value
               callback:(brave_rewards::mojom::RewardsEngineClient::
                             SetBooleanStateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithBool:value];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)
    booleanState:(const std::string&)name
        callback:
            (brave_rewards::mojom::RewardsEngineClient::GetBooleanStateCallback)
                callback {
  const auto key = base::SysUTF8ToNSString(name);
  if (![self.prefs objectForKey:key]) {
    std::move(callback).Run(false);
    return;
  }
  std::move(callback).Run([self.prefs[key] boolValue]);
}

- (void)setIntegerState:(const std::string&)name
                  value:(int32_t)value
               callback:(brave_rewards::mojom::RewardsEngineClient::
                             SetIntegerStateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithInt:value];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)
    integerState:(const std::string&)name
        callback:
            (brave_rewards::mojom::RewardsEngineClient::GetIntegerStateCallback)
                callback {
  const auto key = base::SysUTF8ToNSString(name);
  std::move(callback).Run([self.prefs[key] intValue]);
}

- (void)setDoubleState:(const std::string&)name
                 value:(double)value
              callback:(brave_rewards::mojom::RewardsEngineClient::
                            SetDoubleStateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithDouble:value];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)
    doubleState:(const std::string&)name
       callback:
           (brave_rewards::mojom::RewardsEngineClient::GetDoubleStateCallback)
               callback {
  const auto key = base::SysUTF8ToNSString(name);
  std::move(callback).Run([self.prefs[key] doubleValue]);
}

- (void)setStringState:(const std::string&)name
                 value:(const std::string&)value
              callback:(brave_rewards::mojom::RewardsEngineClient::
                            SetStringStateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = base::SysUTF8ToNSString(value);
  [self savePrefs];
  std::move(callback).Run();
}

- (void)
    stringState:(const std::string&)name
       callback:
           (brave_rewards::mojom::RewardsEngineClient::GetStringStateCallback)
               callback {
  const auto key = base::SysUTF8ToNSString(name);
  const auto value = (NSString*)self.prefs[key];
  if (!value) {
    std::move(callback).Run("");
    return;
  }
  std::move(callback).Run(base::SysNSStringToUTF8(value));
}

- (void)
    setInt64State:(const std::string&)name
            value:(int64_t)value
         callback:
             (brave_rewards::mojom::RewardsEngineClient::SetInt64StateCallback)
                 callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithLongLong:value];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)int64State:(const std::string&)name
          callback:
              (brave_rewards::mojom::RewardsEngineClient::GetInt64StateCallback)
                  callback {
  const auto key = base::SysUTF8ToNSString(name);
  std::move(callback).Run([self.prefs[key] longLongValue]);
}

- (void)setUint64State:(const std::string&)name
                 value:(uint64_t)value
              callback:(brave_rewards::mojom::RewardsEngineClient::
                            SetUint64StateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithUnsignedLongLong:value];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)
    uint64State:(const std::string&)name
       callback:
           (brave_rewards::mojom::RewardsEngineClient::GetUint64StateCallback)
               callback {
  const auto key = base::SysUTF8ToNSString(name);
  std::move(callback).Run([self.prefs[key] unsignedLongLongValue]);
}

- (void)
    setValueState:(const std::string&)name
            value:(base::Value)value
         callback:
             (brave_rewards::mojom::RewardsEngineClient::SetValueStateCallback)
                 callback {
  std::string json;
  if (base::JSONWriter::Write(value, &json)) {
    const auto key = base::SysUTF8ToNSString(name);
    self.prefs[key] = base::SysUTF8ToNSString(json);
    [self savePrefs];
  }
  std::move(callback).Run();
}

- (void)valueState:(const std::string&)name
          callback:
              (brave_rewards::mojom::RewardsEngineClient::GetValueStateCallback)
                  callback {
  const auto key = base::SysUTF8ToNSString(name);
  const auto json = (NSString*)self.prefs[key];
  if (!json) {
    std::move(callback).Run(base::Value());
    return;
  }

  auto value = base::JSONReader::Read(base::SysNSStringToUTF8(json));
  if (!value) {
    std::move(callback).Run(base::Value());
    return;
  }

  std::move(callback).Run(std::move(*value));
}

- (void)
    setTimeState:(const std::string&)name
           value:(base::Time)value
        callback:
            (brave_rewards::mojom::RewardsEngineClient::SetTimeStateCallback)
                callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = @(value.InSecondsFSinceUnixEpoch());
  [self savePrefs];
  std::move(callback).Run();
}

- (void)timeState:(const std::string&)name
         callback:
             (brave_rewards::mojom::RewardsEngineClient::GetTimeStateCallback)
                 callback {
  const auto key = base::SysUTF8ToNSString(name);
  std::move(callback).Run(
      base::Time::FromSecondsSinceUnixEpoch([self.prefs[key] doubleValue]));
}

- (void)clearState:(const std::string&)name
          callback:
              (brave_rewards::mojom::RewardsEngineClient::ClearStateCallback)
                  callback {
  const auto key = base::SysUTF8ToNSString(name);
  [self.prefs removeObjectForKey:key];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)getClientCountryCode:
    (brave_rewards::mojom::RewardsEngineClient::GetClientCountryCodeCallback)
        callback {
  std::move(callback).Run("");
}

- (void)isAutoContributeSupportedForClient:
    (brave_rewards::mojom::RewardsEngineClient::
         IsAutoContributeSupportedForClientCallback)callback {
  std::move(callback).Run(true);
}

#pragma mark - Notifications

- (void)clearAllNotifications {
  // Not used on iOS
}

#pragma mark - State

- (void)loadLegacyState:
    (brave_rewards::mojom::RewardsEngineClient::LoadLegacyStateCallback)
        callback {
  const auto contents =
      [self.commonOps loadContentsFromFileWithName:"ledger_state.json"];
  if (contents.length() > 0) {
    std::move(callback).Run(brave_rewards::mojom::Result::OK, contents);
  } else {
    std::move(callback).Run(brave_rewards::mojom::Result::NO_LEGACY_STATE,
                            contents);
  }
}

- (void)loadPublisherState:
    (brave_rewards::mojom::RewardsEngineClient::LoadPublisherStateCallback)
        callback {
  const auto contents =
      [self.commonOps loadContentsFromFileWithName:"publisher_state.json"];
  if (contents.length() > 0) {
    std::move(callback).Run(brave_rewards::mojom::Result::OK, contents);
  } else {
    std::move(callback).Run(brave_rewards::mojom::Result::NO_PUBLISHER_STATE,
                            contents);
  }
}

#pragma mark - Network

- (NSString*)customUserAgent {
  return self.commonOps.customUserAgent;
}

- (void)setCustomUserAgent:(NSString*)customUserAgent {
  self.commonOps.customUserAgent = [customUserAgent
      stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
}

- (void)loadUrl:(brave_rewards::mojom::UrlRequestPtr)request
       callback:(brave_rewards::mojom::RewardsEngineClient::LoadURLCallback)
                    callback {
  std::map<brave_rewards::mojom::UrlMethod, std::string> methodMap{
      {brave_rewards::mojom::UrlMethod::GET, "GET"},
      {brave_rewards::mojom::UrlMethod::POST, "POST"},
      {brave_rewards::mojom::UrlMethod::PUT, "PUT"},
      {brave_rewards::mojom::UrlMethod::DEL, "DELETE"}};

  if (!request) {
    request = brave_rewards::mojom::UrlRequest::New();
  }

  const auto copiedURL = base::SysUTF8ToNSString(request->url);

  auto cb = std::make_shared<decltype(callback)>(std::move(callback));
  return [self.commonOps
      loadURLRequest:request->url
             headers:request->headers
             content:request->content
        content_type:request->content_type
              method:methodMap[request->method]
            callback:^(
                const std::string& errorDescription, int statusCode,
                const std::string& response,
                const base::flat_map<std::string, std::string>& headers) {
              auto url_response = brave_rewards::mojom::UrlResponse::New();
              url_response->url = base::SysNSStringToUTF8(copiedURL);
              url_response->error = errorDescription;
              url_response->status_code = statusCode;
              url_response->body = response;
              url_response->headers = headers;

              if (cb) {
                std::move(*cb).Run(std::move(url_response));
              }
            }];
}

- (void)
    fetchFavIcon:(const std::string&)url
      faviconKey:(const std::string&)faviconKey
        callback:
            (brave_rewards::mojom::RewardsEngineClient::FetchFavIconCallback)
                callback {
  std::move(callback).Run(NO, std::string());
}

#pragma mark - Logging

- (void)log:(const std::string&)file
            line:(int32_t)line
    verboseLevel:(int32_t)verboseLevel
         message:(const std::string&)message {
  const int vlog_level =
      logging::GetVlogLevelHelper(file.c_str(), file.length());
  if (verboseLevel <= vlog_level) {
    logging::LogMessage(file.c_str(), line, -verboseLevel).stream() << message;
  }
}

#pragma mark - Publisher Database

- (void)publisherListNormalized:
    (std::vector<brave_rewards::mojom::PublisherInfoPtr>)list {
  // Not used on iOS
}

- (void)onPanelPublisherInfo:(brave_rewards::mojom::Result)result
               publisherInfo:
                   (brave_rewards::mojom::PublisherInfoPtr)publisher_info
                    windowId:(uint64_t)windowId {
  if (publisher_info.get() == nullptr ||
      result != brave_rewards::mojom::Result::OK) {
    return;
  }
  auto info =
      [[BraveRewardsPublisherInfo alloc] initWithPublisherInfo:*publisher_info];
  for (RewardsObserver* observer in [self.observers copy]) {
    if (observer.fetchedPanelPublisher) {
      observer.fetchedPanelPublisher(info, windowId);
    }
  }
}

- (void)onPublisherRegistryUpdated {
  // Not used on iOS
}

- (void)onPublisherUpdated:(const std::string&)publisherId {
  // Not used on iOS
}

- (void)showNotification:(const std::string&)type
                    args:(std::vector<std::string>)args
                callback:(brave_rewards::mojom::RewardsEngineClient::
                              ShowNotificationCallback)callback {
  // Not used on iOS
}

- (void)clientInfo:
    (brave_rewards::mojom::RewardsEngineClient::GetClientInfoCallback)callback {
  auto info = brave_rewards::mojom::ClientInfo::New();
  info->os = brave_rewards::mojom::OperatingSystem::UNDEFINED;
  info->platform = brave_rewards::mojom::Platform::IOS;
  std::move(callback).Run(std::move(info));
}

- (void)reconcileStampReset {
  // Not used on iOS
}

- (void)runDbTransaction:(brave_rewards::mojom::DBTransactionPtr)transaction
                callback:(brave_rewards::mojom::RewardsEngineClient::
                              RunDBTransactionCallback)callback {
  _rewardsDatabase->RunTransaction(std::move(transaction), std::move(callback));
}

- (void)walletDisconnected:(const std::string&)wallet_type {
  // Not used on iOS
}

- (void)deleteLog:
    (brave_rewards::mojom::RewardsEngineClient::DeleteLogCallback)callback {
  std::move(callback).Run(brave_rewards::mojom::Result::OK);
}

- (void)
    encryptString:(const std::string&)value
         callback:
             (brave_rewards::mojom::RewardsEngineClient::EncryptStringCallback)
                 callback {
  std::string encrypted_value;
  if (!OSCrypt::EncryptString(value, &encrypted_value)) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  std::move(callback).Run(std::make_optional(encrypted_value));
}

- (void)
    decryptString:(const std::string&)value
         callback:
             (brave_rewards::mojom::RewardsEngineClient::DecryptStringCallback)
                 callback {
  std::string decrypted_value;
  if (!OSCrypt::DecryptString(value, &decrypted_value)) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  std::move(callback).Run(std::make_optional(decrypted_value));
}

- (void)externalWalletConnected {
  // Not used on iOS
}

- (void)externalWalletLoggedOut {
  // Not used on iOS
}

- (void)externalWalletReconnected {
  // Not used on iOS
}

- (void)externalWalletDisconnected {
  // Not used on iOS
}

@end
