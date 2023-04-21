/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave_ledger.h"
#import <UIKit/UIKit.h>

#include "base/base64.h"
#include "base/containers/flat_map.h"
#include "base/ios/ios_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequence_bound.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/build/ios/mojom/cpp_transformations.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger_database.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#import "brave/ios/browser/api/common/common_operations.h"
#import "brave/ios/browser/api/ledger/brave_ledger_observer.h"
#import "brave/ios/browser/api/ledger/ledger.mojom.objc+private.h"
#import "brave/ios/browser/api/ledger/ledger_client_bridge.h"
#import "brave/ios/browser/api/ledger/ledger_client_ios.h"
#import "brave/ios/browser/api/ledger/ledger_types.mojom.objc+private.h"
#import "brave/ios/browser/api/ledger/promotion_solution.h"
#import "brave/ios/browser/api/ledger/rewards_notification.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/origin.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ledger {

template <typename T>
struct task_deleter {
 private:
  scoped_refptr<base::TaskRunner> task_runner;

 public:
  task_deleter()
      : task_runner(base::SequencedTaskRunner::GetCurrentDefault()) {}
  ~task_deleter() = default;

  void operator()(T* ptr) const {
    if (base::SequencedTaskRunner::GetCurrentDefault() != task_runner) {
      task_runner->PostTask(FROM_HERE,
                            base::BindOnce([](T* ptr) { delete ptr; }, ptr));
    } else {
      delete ptr;
    }
  }
};

template <typename T, typename... Args>
std::unique_ptr<T, task_deleter<T>> make_task_ptr(Args&&... args) {
  return {new T(std::forward<Args>(args)...), task_deleter<T>()};
}

}  // namespace ledger

#define LLOG(verbose_level, format, ...)                  \
  [self log:(__FILE__)                                    \
       line:(__LINE__)verboseLevel:(verbose_level)message \
           :base::SysNSStringToUTF8(                      \
                [NSString stringWithFormat:(format), ##__VA_ARGS__])]

NSString* const BraveLedgerErrorDomain = @"BraveLedgerErrorDomain";
NSNotificationName const BraveLedgerNotificationAdded =
    @"BATBraveLedgerNotificationAdded";

BraveGeneralLedgerNotificationID const
    BATBraveGeneralLedgerNotificationIDWalletDisconnected =
        @"wallet_disconnected";

static NSString* const kContributionQueueAutoincrementID =
    @"BATContributionQueueAutoincrementID";
static NSString* const kUnblindedTokenAutoincrementID =
    @"BATUnblindedTokenAutoincrementID";

static NSString* const kExternalWalletsPrefKey = @"external_wallets";
static NSString* const kTransferFeesPrefKey = @"transfer_fees";

static const auto kOneDay =
    base::Time::kHoursPerDay * base::Time::kSecondsPerHour;

/// ---

@interface BraveLedger () <LedgerClientBridge> {
  // DO NOT ACCESS DIRECTLY, use `postLedgerTask` or ensure you are accessing
  // _ledger from a task posted in `_ledgerTaskRunner`
  std::unique_ptr<ledger::LedgerImpl, ledger::task_deleter<ledger::LedgerImpl>>
      _ledger;
  std::unique_ptr<LedgerClientIOS, ledger::task_deleter<LedgerClientIOS>>
      _ledgerClient;
  base::SequenceBound<ledger::LedgerDatabase> rewardsDatabase;
  scoped_refptr<base::SequencedTaskRunner> databaseQueue;
  scoped_refptr<base::SequencedTaskRunner> _ledgerTaskRunner;
}

@property(nonatomic, copy) NSString* storagePath;
@property(nonatomic) LedgerRewardsParameters* rewardsParameters;
@property(nonatomic) LedgerBalance* balance;
@property(nonatomic) dispatch_queue_t fileWriteThread;
@property(nonatomic) NSMutableDictionary<NSString*, NSString*>* state;
@property(nonatomic) BraveCommonOperations* commonOps;
@property(nonatomic) NSMutableDictionary<NSString*, __kindof NSObject*>* prefs;

@property(nonatomic) NSMutableArray<LedgerPromotion*>* mPendingPromotions;
@property(nonatomic) NSMutableArray<LedgerPromotion*>* mFinishedPromotions;

@property(nonatomic) NSHashTable<BraveLedgerObserver*>* observers;

@property(nonatomic, getter=isInitialized) BOOL initialized;
@property(nonatomic) BOOL initializing;
@property(nonatomic) BOOL dataMigrationFailed;
@property(nonatomic) LedgerResult initializationResult;
@property(nonatomic, getter=isLoadingPublisherList) BOOL loadingPublisherList;
@property(nonatomic, getter=isInitializingWallet) BOOL initializingWallet;

/// Notifications

@property(nonatomic) NSTimer* notificationStartupTimer;
@property(nonatomic) NSDate* lastNotificationCheckDate;

/// Temporary blocks

@end

@implementation BraveLedger

- (instancetype)initWithStateStoragePath:(NSString*)path {
  if ((self = [super init])) {
    _ledgerTaskRunner = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::WithBaseSyncPrimitives(),
         base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

    self.storagePath = path;
    self.commonOps =
        [[BraveCommonOperations alloc] initWithStoragePath:path
                                                taskRunner:_ledgerTaskRunner];
    self.state = [[NSMutableDictionary alloc]
                     initWithContentsOfFile:self.randomStatePath]
                     ?: [[NSMutableDictionary alloc] init];
    self.fileWriteThread =
        dispatch_queue_create("com.rewards.file-write", DISPATCH_QUEUE_SERIAL);
    self.mPendingPromotions = [[NSMutableArray alloc] init];
    self.mFinishedPromotions = [[NSMutableArray alloc] init];
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

    [self handleFlags:brave_rewards::RewardsFlags::ForCurrentProcess()];

    databaseQueue = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

    const auto* dbPath = [self rewardsDatabasePath].UTF8String;

    rewardsDatabase = base::SequenceBound<ledger::LedgerDatabase>(
        databaseQueue, base::FilePath(dbPath));

    _ledgerTaskRunner->PostTask(
        FROM_HERE, base::BindOnce(^{
          self->_ledgerClient = ledger::make_task_ptr<LedgerClientIOS>(self);
          self->_ledger = ledger::make_task_ptr<ledger::LedgerImpl>(
              self->_ledgerClient->MakeRemote());
        }));

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
  [self.notificationStartupTimer invalidate];
}

- (void)handleFlags:(const brave_rewards::RewardsFlags&)flags {
  if (flags.environment) {
    switch (*flags.environment) {
      case brave_rewards::RewardsFlags::Environment::kDevelopment:
        ledger::_environment = ledger::mojom::Environment::DEVELOPMENT;
        break;
      case brave_rewards::RewardsFlags::Environment::kStaging:
        ledger::_environment = ledger::mojom::Environment::STAGING;
        break;
      case brave_rewards::RewardsFlags::Environment::kProduction:
        ledger::_environment = ledger::mojom::Environment::PRODUCTION;
        break;
    }
  }

  if (flags.debug) {
    ledger::is_debug = true;
  }

  if (flags.reconcile_interval) {
    ledger::reconcile_interval = *flags.reconcile_interval;
  }

  if (flags.retry_interval) {
    ledger::retry_interval = *flags.retry_interval;
  }
}

- (void)postLedgerTask:(void (^)(ledger::LedgerImpl*))task {
  _ledgerTaskRunner->PostTask(FROM_HERE, base::BindOnce(^{
                                CHECK(self->_ledger != nullptr);
                                task(self->_ledger.get());
                              }));
}

- (void)initializeLedgerService:(nullable void (^)())completion {
  if (self.initialized || self.initializing) {
    return;
  }
  self.initializing = YES;

  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->Initialize(base::BindOnce(^(ledger::mojom::Result result) {
      self.initialized = (result == ledger::mojom::Result::LEDGER_OK ||
                          result == ledger::mojom::Result::NO_LEDGER_STATE ||
                          result == ledger::mojom::Result::NO_PUBLISHER_STATE);
      self.initializing = NO;
      if (self.initialized) {
        [self getRewardsParameters:nil];
        [self fetchBalance:nil];
      } else {
        LLOG(0, @"Ledger Initialization Failed with error: %d", result);
      }
      self.initializationResult = static_cast<LedgerResult>(result);
      if (completion) {
        dispatch_async(dispatch_get_main_queue(), ^{
          completion();
        });
      }
      dispatch_async(dispatch_get_main_queue(), ^{
        for (BraveLedgerObserver* observer in [self.observers copy]) {
          if (observer.walletInitalized) {
            observer.walletInitalized(self.initializationResult);
          }
        }
      });
    }));
  }];
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
  rewardsDatabase = base::SequenceBound<ledger::LedgerDatabase>(
      databaseQueue, base::FilePath(base::SysNSStringToUTF8(dbPath)));
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

- (void)addObserver:(BraveLedgerObserver*)observer {
  [self.observers addObject:observer];
}

- (void)removeObserver:(BraveLedgerObserver*)observer {
  [self.observers removeObject:observer];
}

#pragma mark - Wallet

- (void)createWallet:(void (^)(NSError* _Nullable))completion {
  const auto __weak weakSelf = self;
  // Results that can come from CreateRewardsWallet():
  //   - LEDGER_OK: Good to go
  //   - LEDGER_ERROR: Already initialized
  //   - BAD_REGISTRATION_RESPONSE: Request credentials call failure or
  //   malformed data
  //   - REGISTRATION_VERIFICATION_FAILED: Missing master user token
  self.initializingWallet = YES;
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->CreateRewardsWallet(
        "", base::BindOnce(^(
                ledger::mojom::CreateRewardsWalletResult create_result) {
          const auto strongSelf = weakSelf;
          if (!strongSelf) {
            return;
          }

          ledger::mojom::Result result =
              create_result ==
                      ledger::mojom::CreateRewardsWalletResult::kSuccess
                  ? ledger::mojom::Result::LEDGER_OK
                  : ledger::mojom::Result::LEDGER_ERROR;

          NSError* error = nil;
          if (result != ledger::mojom::Result::LEDGER_OK) {
            std::map<ledger::mojom::Result, std::string> errorDescriptions{
                {ledger::mojom::Result::LEDGER_ERROR,
                 "The wallet was already initialized"},
                {ledger::mojom::Result::BAD_REGISTRATION_RESPONSE,
                 "Request credentials call failure or malformed data"},
                {ledger::mojom::Result::REGISTRATION_VERIFICATION_FAILED,
                 "Missing master user token from registered persona"},
            };
            NSDictionary* userInfo = @{};
            const auto description =
                errorDescriptions[static_cast<ledger::mojom::Result>(result)];
            if (description.length() > 0) {
              userInfo = @{
                NSLocalizedDescriptionKey : base::SysUTF8ToNSString(description)
              };
            }
            error = [NSError errorWithDomain:BraveLedgerErrorDomain
                                        code:static_cast<NSInteger>(result)
                                    userInfo:userInfo];
          }

          [strongSelf startNotificationTimers];
          strongSelf.initializingWallet = NO;

          dispatch_async(dispatch_get_main_queue(), ^{
            if (completion) {
              completion(error);
            }

            for (BraveLedgerObserver* observer in [strongSelf.observers copy]) {
              if (observer.walletInitalized) {
                observer.walletInitalized(static_cast<LedgerResult>(result));
              }
            }
          });
        }));
  }];
}

- (void)currentWalletInfo:
    (void (^)(LedgerRewardsWallet* _Nullable wallet))completion {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->GetRewardsWallet(
        base::BindOnce(^(ledger::mojom::RewardsWalletPtr wallet) {
          const auto bridgedWallet =
              wallet.get() != nullptr
                  ? [[LedgerRewardsWallet alloc] initWithRewardsWallet:*wallet]
                  : nil;
          dispatch_async(dispatch_get_main_queue(), ^{
            completion(bridgedWallet);
          });
        }));
  }];
}

- (void)getRewardsParameters:
    (void (^)(LedgerRewardsParameters* _Nullable))completion {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->GetRewardsParameters(
        base::BindOnce(^(ledger::mojom::RewardsParametersPtr info) {
          if (info) {
            self.rewardsParameters = [[LedgerRewardsParameters alloc]
                initWithRewardsParametersPtr:std::move(info)];
          } else {
            self.rewardsParameters = nil;
          }
          const auto __weak weakSelf = self;
          dispatch_async(dispatch_get_main_queue(), ^{
            if (completion) {
              completion(weakSelf.rewardsParameters);
            }
          });
        }));
  }];
}

- (void)fetchBalance:(void (^)(LedgerBalance* _Nullable))completion {
  const auto __weak weakSelf = self;
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->FetchBalance(base::BindOnce(
        ^(base::expected<ledger::mojom::BalancePtr,
                         ledger::mojom::FetchBalanceError> result) {
          const auto strongSelf = weakSelf;
          if (result.has_value()) {
            strongSelf.balance = [[LedgerBalance alloc]
                initWithBalancePtr:std::move(result.value())];
          }
          dispatch_async(dispatch_get_main_queue(), ^{
            if (completion) {
              completion(strongSelf.balance);
            }
          });
        }));
  }];
}

- (void)legacyWallet:
    (ledger::mojom::LedgerClient::GetLegacyWalletCallback)callback {
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
                           filter:(LedgerActivityInfoFilter*)filter
                       completion:(void (^)(NSArray<LedgerPublisherInfo*>*))
                                      completion {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    auto cppFilter =
        filter ? filter.cppObjPtr : ledger::mojom::ActivityInfoFilter::New();
    if (filter.excluded == LedgerExcludeFilterFilterExcluded) {
      ledger->GetExcludedList(base::BindOnce(^(
          std::vector<ledger::mojom::PublisherInfoPtr> list) {
        const auto publishers = NSArrayFromVector(
            &list,
            ^LedgerPublisherInfo*(const ledger::mojom::PublisherInfoPtr& info) {
              return [[LedgerPublisherInfo alloc] initWithPublisherInfo:*info];
            });
        dispatch_async(dispatch_get_main_queue(), ^{
          completion(publishers);
        });
      }));
    } else {
      ledger->GetActivityInfoList(
          start, limit, std::move(cppFilter),
          base::BindOnce(^(std::vector<ledger::mojom::PublisherInfoPtr> list) {
            const auto publishers = NSArrayFromVector(
                &list, ^LedgerPublisherInfo*(
                    const ledger::mojom::PublisherInfoPtr& info) {
                  return
                      [[LedgerPublisherInfo alloc] initWithPublisherInfo:*info];
                });
            dispatch_async(dispatch_get_main_queue(), ^{
              completion(publishers);
            });
          }));
    }
  }];
}

- (void)fetchPublisherActivityFromURL:(NSURL*)URL
                           faviconURL:(nullable NSURL*)faviconURL
                        publisherBlob:(nullable NSString*)publisherBlob
                                tabId:(uint64_t)tabId {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
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

    ledger::mojom::VisitDataPtr visitData = ledger::mojom::VisitData::New();
    visitData->domain = visitData->name = baseDomain;
    visitData->path = parsedUrl.PathForRequest();
    visitData->url = origin.Serialize();

    if (faviconURL.absoluteString) {
      visitData->favicon_url =
          base::SysNSStringToUTF8(faviconURL.absoluteString);
    }

    std::string blob = std::string();
    if (publisherBlob) {
      blob = base::SysNSStringToUTF8(publisherBlob);
    }

    ledger->GetPublisherActivityFromUrl(tabId, std::move(visitData), blob);
  }];
}

- (void)refreshPublisherWithId:(NSString*)publisherId
                    completion:
                        (void (^)(LedgerPublisherStatus status))completion {
  if (self.loadingPublisherList) {
    completion(LedgerPublisherStatusNotVerified);
    return;
  }
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->RefreshPublisher(
        base::SysNSStringToUTF8(publisherId),
        base::BindOnce(^(ledger::mojom::PublisherStatus status) {
          dispatch_async(dispatch_get_main_queue(), ^{
            completion(static_cast<LedgerPublisherStatus>(status));
          });
        }));
  }];
}

#pragma mark - Tips

- (void)listRecurringTips:(void (^)(NSArray<LedgerPublisherInfo*>*))completion {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->GetRecurringTips(base::BindOnce(^(
        std::vector<ledger::mojom::PublisherInfoPtr> list) {
      const auto publishers = NSArrayFromVector(
          &list,
          ^LedgerPublisherInfo*(const ledger::mojom::PublisherInfoPtr& info) {
            return [[LedgerPublisherInfo alloc] initWithPublisherInfo:*info];
          });
      dispatch_async(dispatch_get_main_queue(), ^{
        completion(publishers);
      });
    }));
  }];
}

- (void)removeRecurringTipForPublisherWithId:(NSString*)publisherId {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->RemoveRecurringTip(base::SysNSStringToUTF8(publisherId),
                               base::BindOnce(^(ledger::mojom::Result result){
                                   // Not Used
                               }));
  }];
}

#pragma mark - Grants

- (NSArray<LedgerPromotion*>*)pendingPromotions {
  return [self.mPendingPromotions copy];
}

- (NSArray<LedgerPromotion*>*)finishedPromotions {
  return [self.mFinishedPromotions copy];
}

- (NSString*)notificationIDForPromo:(const ledger::mojom::PromotionPtr)promo {
  bool isUGP = promo->type == ledger::mojom::PromotionType::UGP;
  const auto prefix = isUGP ? @"rewards_grant_" : @"rewards_grant_ads_";
  const auto promotionId = base::SysUTF8ToNSString(promo->id);
  return [NSString stringWithFormat:@"%@%@", prefix, promotionId];
}

- (void)updatePendingAndFinishedPromotions:(void (^)())completion {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->GetAllPromotions(base::BindOnce(
        ^(base::flat_map<std::string, ledger::mojom::PromotionPtr> map) {
          NSMutableArray* promos = [[NSMutableArray alloc] init];
          for (auto it = map.begin(); it != map.end(); ++it) {
            if (it->second.get() != nullptr) {
              [promos addObject:[[LedgerPromotion alloc]
                                    initWithPromotion:*it->second]];
            }
          }
          [self.mFinishedPromotions removeAllObjects];
          [self.mPendingPromotions removeAllObjects];
          for (LedgerPromotion* promotion in promos) {
            if (promotion.status == LedgerPromotionStatusFinished) {
              [self.mFinishedPromotions addObject:promotion];
            } else if (promotion.status == LedgerPromotionStatusActive ||
                       promotion.status == LedgerPromotionStatusAttested) {
              [self.mPendingPromotions addObject:promotion];
            }
          }
          dispatch_async(dispatch_get_main_queue(), ^{
            if (completion) {
              completion();
            }
            for (BraveLedgerObserver* observer in [self.observers copy]) {
              if (observer.promotionsAdded) {
                observer.promotionsAdded(self.pendingPromotions);
              }
              if (observer.finishedPromotionsAdded) {
                observer.finishedPromotionsAdded(self.finishedPromotions);
              }
            }
          });
        }));
  }];
}

- (void)fetchPromotions:
    (nullable void (^)(NSArray<LedgerPromotion*>* grants))completion {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->FetchPromotions(
        base::BindOnce(^(ledger::mojom::Result result,
                         std::vector<ledger::mojom::PromotionPtr> promotions) {
          if (result != ledger::mojom::Result::LEDGER_OK) {
            return;
          }
          [self updatePendingAndFinishedPromotions:^{
            if (completion) {
              dispatch_async(dispatch_get_main_queue(), ^{
                completion(self.pendingPromotions);
              });
            }
          }];
        }));
  }];
}

- (void)claimPromotion:(NSString*)promotionId
             publicKey:(NSString*)deviceCheckPublicKey
            completion:(void (^)(LedgerResult result,
                                 NSString* _Nonnull nonce))completion {
  const auto payload = [NSDictionary dictionaryWithObject:deviceCheckPublicKey
                                                   forKey:@"publicKey"];
  const auto jsonData = [NSJSONSerialization dataWithJSONObject:payload
                                                        options:0
                                                          error:nil];
  if (!jsonData) {
    LLOG(0, @"Missing JSON payload while attempting to claim promotion");
    return;
  }
  const auto jsonString = [[NSString alloc] initWithData:jsonData
                                                encoding:NSUTF8StringEncoding];
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->ClaimPromotion(
        base::SysNSStringToUTF8(promotionId),
        base::SysNSStringToUTF8(jsonString),
        base::BindOnce(
            ^(ledger::mojom::Result result, const std::string& nonce) {
              const auto bridgedNonce = base::SysUTF8ToNSString(nonce);
              dispatch_async(dispatch_get_main_queue(), ^{
                completion(static_cast<LedgerResult>(result), bridgedNonce);
              });
            }));
  }];
}

- (void)attestPromotion:(NSString*)promotionId
               solution:(PromotionSolution*)solution
             completion:
                 (void (^)(LedgerResult result,
                           LedgerPromotion* _Nullable promotion))completion {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->AttestPromotion(
        base::SysNSStringToUTF8(promotionId),
        base::SysNSStringToUTF8(solution.JSONPayload),
        base::BindOnce(^(ledger::mojom::Result result,
                         ledger::mojom::PromotionPtr promotion) {
          if (promotion.get() == nullptr) {
            if (completion) {
              dispatch_async(dispatch_get_main_queue(), ^{
                completion(static_cast<LedgerResult>(result), nil);
              });
            }
            return;
          }

          const auto bridgedPromotion =
              [[LedgerPromotion alloc] initWithPromotion:*promotion];
          if (result == ledger::mojom::Result::LEDGER_OK) {
            [self fetchBalance:nil];
          }

          dispatch_async(dispatch_get_main_queue(), ^{
            if (completion) {
              completion(static_cast<LedgerResult>(result), bridgedPromotion);
            }
            if (result == ledger::mojom::Result::LEDGER_OK) {
              for (BraveLedgerObserver* observer in [self.observers copy]) {
                if (observer.promotionClaimed) {
                  observer.promotionClaimed(bridgedPromotion);
                }
              }
            }
          });
        }));
  }];
}

#pragma mark - Pending Contributions

- (void)removeAllPendingContributions:
    (void (^)(LedgerResult result))completion {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->RemoveAllPendingContributions(
        base::BindOnce(^(const ledger::mojom::Result result) {
          dispatch_async(dispatch_get_main_queue(), ^{
            completion(static_cast<LedgerResult>(result));
          });
        }));
  }];
}

#pragma mark - Reconcile

- (void)onReconcileComplete:(ledger::mojom::Result)result
               contribution:(ledger::mojom::ContributionInfoPtr)contribution {
  // TODO we changed from probi to amount, so from string to double
  if (result == ledger::mojom::Result::LEDGER_OK) {
    [self fetchBalance:nil];
  }
}

#pragma mark - Misc

- (void)rewardsInternalInfo:
    (void (^)(LedgerRewardsInternalsInfo* _Nullable info))completion {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->GetRewardsInternalsInfo(
        base::BindOnce(^(ledger::mojom::RewardsInternalsInfoPtr info) {
          auto bridgedInfo = info.get() != nullptr
                                 ? [[LedgerRewardsInternalsInfo alloc]
                                       initWithRewardsInternalsInfo:*info.get()]
                                 : nil;
          dispatch_async(dispatch_get_main_queue(), ^{
            completion(bridgedInfo);
          });
        }));
  }];
}

- (void)allContributions:
    (void (^)(NSArray<LedgerContributionInfo*>* contributions))completion {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->GetAllContributions(
        base::BindOnce(^(std::vector<ledger::mojom::ContributionInfoPtr> list) {
          const auto convetedList = NSArrayFromVector(
              &list, ^LedgerContributionInfo*(
                  const ledger::mojom::ContributionInfoPtr& info) {
                return [[LedgerContributionInfo alloc]
                    initWithContributionInfo:*info];
              });
          dispatch_async(dispatch_get_main_queue(), ^{
            completion(convetedList);
          });
        }));
  }];
}

- (void)fetchAutoContributeProperties:
    (void (^)(LedgerAutoContributeProperties* _Nullable properties))completion {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->GetAutoContributeProperties(
        base::BindOnce(^(ledger::mojom::AutoContributePropertiesPtr props) {
          auto properties =
              props.get() != nullptr
                  ? [[LedgerAutoContributeProperties alloc]
                        initWithAutoContributePropertiesPtr:std::move(props)]
                  : nil;
          dispatch_async(dispatch_get_main_queue(), ^{
            completion(properties);
          });
        }));
  }];
}

#pragma mark - Reporting

- (void)setSelectedTabId:(UInt32)selectedTabId {
  if (!self.initialized) {
    return;
  }

  const auto time = [[NSDate date] timeIntervalSince1970];
  if (_selectedTabId != selectedTabId) {
    const auto oldTabId = _selectedTabId;
    [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
      ledger->OnHide(oldTabId, time);
    }];
  }
  _selectedTabId = selectedTabId;
  if (_selectedTabId > 0) {
    [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
      ledger->OnShow(selectedTabId, time);
    }];
  }
}

- (void)applicationDidBecomeActive {
  if (!self.initialized) {
    return;
  }

  const auto time = [[NSDate date] timeIntervalSince1970];
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->OnForeground(self.selectedTabId, time);
  }];

  // Check if the last notification check was more than a day ago
  if (fabs([self.lastNotificationCheckDate timeIntervalSinceNow]) > kOneDay) {
    [self checkForNotificationsAndFetchGrants];
  }
}

- (void)applicationDidBackground {
  if (!self.initialized) {
    return;
  }

  const auto time = [[NSDate date] timeIntervalSince1970];
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->OnBackground(self.selectedTabId, time);
  }];
}

- (void)reportLoadedPageWithURL:(NSURL*)url tabId:(UInt32)tabId {
  if (!self.initialized) {
    return;
  }

  const auto time = [[NSDate date] timeIntervalSince1970];
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    GURL parsedUrl(base::SysNSStringToUTF8(url.absoluteString));
    url::Origin origin = url::Origin::Create(parsedUrl);
    const std::string baseDomain = GetDomainAndRegistry(
        origin.host(),
        net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

    if (baseDomain == "") {
      return;
    }

    const std::string publisher_url =
        origin.scheme() + "://" + baseDomain + "/";

    ledger::mojom::VisitDataPtr data = ledger::mojom::VisitData::New();
    data->name = baseDomain;
    data->domain = origin.host();
    data->path = parsedUrl.path();
    data->tab_id = tabId;
    data->url = publisher_url;

    ledger->OnLoad(std::move(data), time);
  }];
}

- (void)reportXHRLoad:(NSURL*)url
                tabId:(UInt32)tabId
        firstPartyURL:(NSURL*)firstPartyURL
          referrerURL:(NSURL*)referrerURL {
  if (!self.initialized) {
    return;
  }

  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    base::flat_map<std::string, std::string> partsMap;
    const auto urlComponents = [[NSURLComponents alloc] initWithURL:url
                                            resolvingAgainstBaseURL:NO];
    for (NSURLQueryItem* item in urlComponents.queryItems) {
      std::string value =
          item.value != nil ? base::SysNSStringToUTF8(item.value) : "";
      partsMap[base::SysNSStringToUTF8(item.name)] = value;
    }

    auto visit = ledger::mojom::VisitData::New();
    visit->path = base::SysNSStringToUTF8(url.absoluteString);
    visit->tab_id = tabId;

    std::string ref = referrerURL != nil
                          ? base::SysNSStringToUTF8(referrerURL.absoluteString)
                          : "";
    std::string fpu =
        firstPartyURL != nil
            ? base::SysNSStringToUTF8(firstPartyURL.absoluteString)
            : "";

    ledger->OnXHRLoad(tabId, base::SysNSStringToUTF8(url.absoluteString),
                      partsMap, fpu, ref, std::move(visit));
  }];
}

- (void)reportTabNavigationOrClosedWithTabId:(UInt32)tabId {
  if (!self.initialized) {
    return;
  }

  const auto time = [[NSDate date] timeIntervalSince1970];
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->OnUnload(tabId, time);
  }];
}

#pragma mark - Preferences

- (void)setMinimumVisitDuration:(int)minimumVisitDuration {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->SetPublisherMinVisitTime(minimumVisitDuration);
  }];
}

- (void)setMinimumNumberOfVisits:(int)minimumNumberOfVisits {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->SetPublisherMinVisits(minimumNumberOfVisits);
  }];
}

- (void)setAllowUnverifiedPublishers:(bool)allowUnverifiedPublishers {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->SetPublisherAllowNonVerified(allowUnverifiedPublishers);
  }];
}

- (void)setContributionAmount:(double)contributionAmount {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->SetAutoContributionAmount(contributionAmount);
  }];
}

- (void)setAutoContributeEnabled:(bool)autoContributeEnabled {
  [self postLedgerTask:^(ledger::LedgerImpl* ledger) {
    ledger->SetAutoContributeEnabled(autoContributeEnabled);
  }];
}

- (void)setBooleanState:(const std::string&)name
                  value:(bool)value
               callback:(ledger::mojom::LedgerClient::SetBooleanStateCallback)
                            callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithBool:value];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)booleanState:(const std::string&)name
            callback:
                (ledger::mojom::LedgerClient::GetBooleanStateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  if (![self.prefs objectForKey:key]) {
    std::move(callback).Run(false);
    return;
  }
  std::move(callback).Run([self.prefs[key] boolValue]);
}

- (void)setIntegerState:(const std::string&)name
                  value:(int32_t)value
               callback:(ledger::mojom::LedgerClient::SetIntegerStateCallback)
                            callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithInt:value];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)integerState:(const std::string&)name
            callback:
                (ledger::mojom::LedgerClient::GetIntegerStateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  std::move(callback).Run([self.prefs[key] intValue]);
}

- (void)setDoubleState:(const std::string&)name
                 value:(double)value
              callback:(ledger::mojom::LedgerClient::SetDoubleStateCallback)
                           callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithDouble:value];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)doubleState:(const std::string&)name
           callback:
               (ledger::mojom::LedgerClient::GetDoubleStateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  std::move(callback).Run([self.prefs[key] doubleValue]);
}

- (void)setStringState:(const std::string&)name
                 value:(const std::string&)value
              callback:(ledger::mojom::LedgerClient::SetStringStateCallback)
                           callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = base::SysUTF8ToNSString(value);
  [self savePrefs];
  std::move(callback).Run();
}

- (void)stringState:(const std::string&)name
           callback:
               (ledger::mojom::LedgerClient::GetStringStateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  const auto value = (NSString*)self.prefs[key];
  if (!value) {
    std::move(callback).Run("");
    return;
  }
  std::move(callback).Run(base::SysNSStringToUTF8(value));
}

- (void)setInt64State:(const std::string&)name
                value:(int64_t)value
             callback:
                 (ledger::mojom::LedgerClient::SetInt64StateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithLongLong:value];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)int64State:(const std::string&)name
          callback:
              (ledger::mojom::LedgerClient::GetInt64StateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  std::move(callback).Run([self.prefs[key] longLongValue]);
}

- (void)setUint64State:(const std::string&)name
                 value:(uint64_t)value
              callback:(ledger::mojom::LedgerClient::SetUint64StateCallback)
                           callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithUnsignedLongLong:value];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)uint64State:(const std::string&)name
           callback:
               (ledger::mojom::LedgerClient::GetUint64StateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  std::move(callback).Run([self.prefs[key] unsignedLongLongValue]);
}

- (void)setValueState:(const std::string&)name
                value:(base::Value)value
             callback:
                 (ledger::mojom::LedgerClient::SetValueStateCallback)callback {
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
              (ledger::mojom::LedgerClient::GetValueStateCallback)callback {
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

- (void)setTimeState:(const std::string&)name
               value:(base::Time)value
            callback:
                (ledger::mojom::LedgerClient::SetTimeStateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = @(value.ToDoubleT());
  [self savePrefs];
  std::move(callback).Run();
}

- (void)timeState:(const std::string&)name
         callback:(ledger::mojom::LedgerClient::GetTimeStateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  std::move(callback).Run(
      base::Time::FromDoubleT([self.prefs[key] doubleValue]));
}

- (void)clearState:(const std::string&)name
          callback:(ledger::mojom::LedgerClient::ClearStateCallback)callback {
  const auto key = base::SysUTF8ToNSString(name);
  [self.prefs removeObjectForKey:key];
  [self savePrefs];
  std::move(callback).Run();
}

- (void)isBitFlyerRegion:
    (ledger::mojom::LedgerClient::IsBitFlyerRegionCallback)callback {
  std::move(callback).Run(false);
}

#pragma mark - Notifications

- (void)clearAllNotifications {
  // Not used on iOS
}

- (void)startNotificationTimers {
  dispatch_async(dispatch_get_main_queue(), ^{
    // Startup timer, begins after 30-second delay.
    self.notificationStartupTimer =
        [NSTimer scheduledTimerWithTimeInterval:30
                                         target:self
                                       selector:@selector
                                       (checkForNotificationsAndFetchGrants)
                                       userInfo:nil
                                        repeats:NO];
  });
}

- (void)checkForNotificationsAndFetchGrants {
  self.lastNotificationCheckDate = [NSDate date];

  [self fetchPromotions:nil];
}

#pragma mark - State

- (void)loadLedgerState:
    (ledger::mojom::LedgerClient::LoadLedgerStateCallback)callback {
  const auto contents =
      [self.commonOps loadContentsFromFileWithName:"ledger_state.json"];
  if (contents.length() > 0) {
    std::move(callback).Run(ledger::mojom::Result::LEDGER_OK, contents);
  } else {
    std::move(callback).Run(ledger::mojom::Result::NO_LEDGER_STATE, contents);
  }
  [self startNotificationTimers];
}

- (void)loadPublisherState:
    (ledger::mojom::LedgerClient::LoadPublisherStateCallback)callback {
  const auto contents =
      [self.commonOps loadContentsFromFileWithName:"publisher_state.json"];
  if (contents.length() > 0) {
    std::move(callback).Run(ledger::mojom::Result::LEDGER_OK, contents);
  } else {
    std::move(callback).Run(ledger::mojom::Result::NO_PUBLISHER_STATE,
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

- (void)loadUrl:(ledger::mojom::UrlRequestPtr)request
       callback:(ledger::mojom::LedgerClient::LoadURLCallback)callback {
  std::map<ledger::mojom::UrlMethod, std::string> methodMap{
      {ledger::mojom::UrlMethod::GET, "GET"},
      {ledger::mojom::UrlMethod::POST, "POST"},
      {ledger::mojom::UrlMethod::PUT, "PUT"},
      {ledger::mojom::UrlMethod::DEL, "DELETE"}};

  if (!request) {
    request = ledger::mojom::UrlRequest::New();
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
              auto url_response = ledger::mojom::UrlResponse::New();
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

- (void)fetchFavIcon:(const std::string&)url
          faviconKey:(const std::string&)faviconKey
            callback:
                (ledger::mojom::LedgerClient::FetchFavIconCallback)callback {
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
    (std::vector<ledger::mojom::PublisherInfoPtr>)list {
  // Not used on iOS
}

- (void)onPanelPublisherInfo:(ledger::mojom::Result)result
               publisherInfo:(ledger::mojom::PublisherInfoPtr)publisher_info
                    windowId:(uint64_t)windowId {
  if (publisher_info.get() == nullptr ||
      result != ledger::mojom::Result::LEDGER_OK) {
    return;
  }
  auto info =
      [[LedgerPublisherInfo alloc] initWithPublisherInfo:*publisher_info];
  for (BraveLedgerObserver* observer in [self.observers copy]) {
    if (observer.fetchedPanelPublisher) {
      observer.fetchedPanelPublisher(info, windowId);
    }
  }
}

- (void)onContributeUnverifiedPublishers:(ledger::mojom::Result)result
                            publisherKey:(const std::string&)publisher_key
                           publisherName:(const std::string&)publisher_name {
  // Not used on iOS
}

- (void)onPublisherRegistryUpdated {
  // Not used on iOS
}

- (void)onPublisherUpdated:(const std::string&)publisherId {
  // Not used on iOS
}

- (void)showNotification:(const std::string&)type
                    args:(std::vector<std::string>)args
                callback:(ledger::mojom::LedgerClient::ShowNotificationCallback)
                             callback {
  // Not used on iOS
}

- (void)clientInfo:
    (ledger::mojom::LedgerClient::GetClientInfoCallback)callback {
  auto info = ledger::mojom::ClientInfo::New();
  info->os = ledger::mojom::OperatingSystem::UNDEFINED;
  info->platform = ledger::mojom::Platform::IOS;
  std::move(callback).Run(std::move(info));
}

- (void)unblindedTokensReady {
  [self fetchBalance:nil];
}

- (void)reconcileStampReset {
  // Not used on iOS
}

- (void)runDbTransaction:(ledger::mojom::DBTransactionPtr)transaction
                callback:(ledger::mojom::LedgerClient::RunDBTransactionCallback)
                             callback {
  __weak BraveLedger* weakSelf = self;
  DCHECK(rewardsDatabase);
  rewardsDatabase.AsyncCall(&ledger::LedgerDatabase::RunTransaction)
      .WithArgs(std::move(transaction))
      .Then(base::BindOnce(
          ^(ledger::RunDBTransactionCallback completion,
            ledger::mojom::DBCommandResponsePtr response) {
            if (weakSelf)
              std::move(completion).Run(std::move(response));
          },
          std::move(callback)));
}

- (void)pendingContributionSaved:(const ledger::mojom::Result)result {
  // Not used on iOS
}

- (void)walletDisconnected:(const std::string&)wallet_type {
  // Not used on iOS
}

- (void)deleteLog:(ledger::mojom::LedgerClient::DeleteLogCallback)callback {
  std::move(callback).Run(ledger::mojom::Result::LEDGER_OK);
}

- (void)encryptString:(const std::string&)value
             callback:
                 (ledger::mojom::LedgerClient::EncryptStringCallback)callback {
  std::string encrypted_value;
  if (!OSCrypt::EncryptString(value, &encrypted_value)) {
    std::move(callback).Run(absl::nullopt);
    return;
  }
  std::move(callback).Run(absl::make_optional(encrypted_value));
}

- (void)decryptString:(const std::string&)value
             callback:
                 (ledger::mojom::LedgerClient::DecryptStringCallback)callback {
  std::string decrypted_value;
  if (!OSCrypt::DecryptString(value, &decrypted_value)) {
    std::move(callback).Run(absl::nullopt);
    return;
  }
  std::move(callback).Run(absl::make_optional(decrypted_value));
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

@end
