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
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_database.h"
#include "brave/components/brave_rewards/core/option_keys.h"
#import "brave/ios/browser/api/common/common_operations.h"
#import "brave/ios/browser/api/ledger/brave_ledger_observer.h"
#import "brave/ios/browser/api/ledger/ledger_client_bridge.h"
#import "brave/ios/browser/api/ledger/ledger_client_ios.h"
#import "brave/ios/browser/api/ledger/legacy_database/data_controller.h"
#import "brave/ios/browser/api/ledger/legacy_database/legacy_ledger_database.h"
#import "brave/ios/browser/api/ledger/promotion_solution.h"
#import "brave/ios/browser/api/ledger/rewards_core.mojom.objc+private.h"
#import "brave/ios/browser/api/ledger/rewards_core_types.mojom.objc+private.h"
#import "brave/ios/browser/api/ledger/rewards_notification.h"
#include "components/os_crypt/os_crypt.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/origin.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#define BLOG(verbose_level, format, ...)                  \
  [self log:(__FILE__)                                    \
       line:(__LINE__)verboseLevel:(verbose_level)message \
           :base::SysNSStringToUTF8(                      \
                [NSString stringWithFormat:(format), ##__VA_ARGS__])]

#define BATLedgerReadonlyBridge(__type, __objc_getter, __cpp_getter) \
  -(__type)__objc_getter {                                           \
    return ledger->__cpp_getter();                                   \
  }

#define BATLedgerBridge(__type, __objc_getter, __objc_setter, __cpp_getter, \
                        __cpp_setter)                                       \
  -(__type)__objc_getter {                                                  \
    return ledger->__cpp_getter();                                          \
  }                                                                         \
  -(void)__objc_setter : (__type)newValue {                                 \
    ledger->__cpp_setter(newValue);                                         \
  }

NSString* const BraveLedgerErrorDomain = @"BraveLedgerErrorDomain";
NSNotificationName const BraveLedgerNotificationAdded =
    @"BATBraveLedgerNotificationAdded";

BraveGeneralLedgerNotificationID const
    BATBraveGeneralLedgerNotificationIDWalletDisconnected =
        @"wallet_disconnected";

static NSString* const kMigrationSucceeded = @"BATRewardsMigrationSucceeded";

static NSString* const kContributionQueueAutoincrementID =
    @"BATContributionQueueAutoincrementID";
static NSString* const kUnblindedTokenAutoincrementID =
    @"BATUnblindedTokenAutoincrementID";

static NSString* const kExternalWalletsPrefKey = @"external_wallets";
static NSString* const kTransferFeesPrefKey = @"transfer_fees";

static const auto kOneDay =
    base::Time::kHoursPerDay * base::Time::kSecondsPerHour;

/// Ledger Prefs, keys will be defined in
/// `brave/components/brave_rewards/core/option_keys.h`
const std::map<std::string, bool> kBoolOptions = {
    {brave_rewards::core::option::kIsBitflyerRegion, false}};
const std::map<std::string, int> kIntegerOptions = {};
const std::map<std::string, double> kDoubleOptions = {};
const std::map<std::string, std::string> kStringOptions = {};
const std::map<std::string, int64_t> kInt64Options = {};
const std::map<std::string, uint64_t> kUInt64Options = {
    {brave_rewards::core::option::kPublisherListRefreshInterval,
     7 * base::Time::kHoursPerDay* base::Time::kSecondsPerHour}};
/// ---

/// When initializing the ledger, what should we do when migrating
typedef NS_ENUM(NSInteger, BATLedgerDatabaseMigrationType) {
  /// Attempt to migrate all rewards data if needed
  BATLedgerDatabaseMigrationTypeDefault = 0,
  /// Only migrate unblinded tokens if needed
  BATLedgerDatabaseMigrationTypeTokensOnly,
  /// Do not migrate any data (essentially resetting rewards activity & balance)
  BATLedgerDatabaseMigrationTypeNone
};

@interface BraveLedger () <LedgerClientBridge> {
  LedgerClientIOS* ledgerClient;
  brave_rewards::core::Ledger* ledger;
  base::SequenceBound<brave_rewards::core::LedgerDatabase> rewardsDatabase;
  scoped_refptr<base::SequencedTaskRunner> databaseQueue;
}

@property(nonatomic, copy) NSString* storagePath;
@property(nonatomic) BraveRewardsRewardsParameters* rewardsParameters;
@property(nonatomic) BraveRewardsBalance* balance;
@property(nonatomic) dispatch_queue_t fileWriteThread;
@property(nonatomic) NSMutableDictionary<NSString*, NSString*>* state;
@property(nonatomic) BraveCommonOperations* commonOps;
@property(nonatomic) NSMutableDictionary<NSString*, __kindof NSObject*>* prefs;

@property(nonatomic) NSMutableArray<BraveRewardsPromotion*>* mPendingPromotions;
@property(nonatomic)
    NSMutableArray<BraveRewardsPromotion*>* mFinishedPromotions;

@property(nonatomic) NSHashTable<BraveLedgerObserver*>* observers;

@property(nonatomic, getter=isInitialized) BOOL initialized;
@property(nonatomic) BOOL initializing;
@property(nonatomic) BOOL dataMigrationFailed;
@property(nonatomic) BraveRewardsResult initializationResult;
@property(nonatomic, getter=isLoadingPublisherList) BOOL loadingPublisherList;
@property(nonatomic, getter=isInitializingWallet) BOOL initializingWallet;
@property(nonatomic) BATLedgerDatabaseMigrationType migrationType;

/// Notifications

@property(nonatomic) NSMutableArray<RewardsNotification*>* mNotifications;
@property(nonatomic) NSTimer* notificationStartupTimer;
@property(nonatomic) NSDate* lastNotificationCheckDate;

/// Temporary blocks

@end

@implementation BraveLedger

- (instancetype)initWithStateStoragePath:(NSString*)path {
  if ((self = [super init])) {
    self.storagePath = path;
    self.commonOps = [[BraveCommonOperations alloc] initWithStoragePath:path];
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
      // Setup defaults
      self.prefs[kMigrationSucceeded] = @(NO);
      [self savePrefs];
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

    rewardsDatabase = base::SequenceBound<brave_rewards::core::LedgerDatabase>(
        databaseQueue, base::FilePath(dbPath));

    ledgerClient = new LedgerClientIOS(self);
    ledger = brave_rewards::core::Ledger::CreateInstance(ledgerClient);

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
  delete ledger;
  delete ledgerClient;
}

- (void)handleFlags:(const brave_rewards::RewardsFlags&)flags {
  if (flags.environment) {
    switch (*flags.environment) {
      case brave_rewards::RewardsFlags::Environment::kDevelopment:
        brave_rewards::core::_environment =
            brave_rewards::mojom::Environment::DEVELOPMENT;
        break;
      case brave_rewards::RewardsFlags::Environment::kStaging:
        brave_rewards::core::_environment =
            brave_rewards::mojom::Environment::STAGING;
        break;
      case brave_rewards::RewardsFlags::Environment::kProduction:
        brave_rewards::core::_environment =
            brave_rewards::mojom::Environment::PRODUCTION;
        break;
    }
  }

  if (flags.debug) {
    brave_rewards::core::is_debug = true;
  }

  if (flags.reconcile_interval) {
    brave_rewards::core::reconcile_interval = *flags.reconcile_interval;
  }

  if (flags.retry_interval) {
    brave_rewards::core::retry_interval = *flags.retry_interval;
  }
}

- (void)initializeLedgerService:(nullable void (^)())completion {
  self.migrationType = BATLedgerDatabaseMigrationTypeDefault;
  [self databaseNeedsMigration:^(BOOL needsMigration) {
    if (needsMigration) {
      [BATLedgerDatabase deleteCoreDataServerPublisherList:nil];
    }
    [self initializeLedgerService:needsMigration completion:completion];
  }];
}

- (void)initializeLedgerService:(BOOL)executeMigrateScript
                     completion:(nullable void (^)())completion {
  if (self.initialized || self.initializing) {
    return;
  }
  self.initializing = YES;

  BLOG(3, @"DB: Migrate from CoreData? %@",
       (executeMigrateScript ? @"YES" : @"NO"));
  ledger->Initialize(executeMigrateScript, ^(
                         brave_rewards::mojom::Result result) {
    self.initialized =
        (result == brave_rewards::mojom::Result::LEDGER_OK ||
         result == brave_rewards::mojom::Result::NO_LEDGER_STATE ||
         result == brave_rewards::mojom::Result::NO_PUBLISHER_STATE);
    self.initializing = NO;
    if (self.initialized) {
      self.prefs[kMigrationSucceeded] = @(YES);
      [self savePrefs];

      [self getRewardsParameters:nil];
      [self fetchBalance:nil];

      [self readNotificationsFromDisk];
    } else {
      BLOG(0, @"Ledger Initialization Failed with error: %d", result);
      if (result == brave_rewards::mojom::Result::DATABASE_INIT_FAILED) {
        // Failed to migrate data...
        switch (self.migrationType) {
          case BATLedgerDatabaseMigrationTypeDefault:
            BLOG(0,
                 @"DB: Full migration failed, attempting BAT only migration.");
            self.dataMigrationFailed = YES;
            self.migrationType = BATLedgerDatabaseMigrationTypeTokensOnly;
            [self resetRewardsDatabase];
            // attempt re-initialize without other data
            [self initializeLedgerService:YES completion:completion];
            return;
          case BATLedgerDatabaseMigrationTypeTokensOnly:
            BLOG(0, @"DB: BAT only migration failed. Initializing without "
                    @"migration.");
            self.dataMigrationFailed = YES;
            self.migrationType = BATLedgerDatabaseMigrationTypeNone;
            [self resetRewardsDatabase];
            // attempt initialize without migrating at all
            [self initializeLedgerService:NO completion:completion];
            return;
          default:
            break;
        }
      }
    }
    self.initializationResult = static_cast<BraveRewardsResult>(result);
    if (completion) {
      completion();
    }
    for (BraveLedgerObserver* observer in [self.observers copy]) {
      if (observer.walletInitalized) {
        observer.walletInitalized(self.initializationResult);
      }
    }
  });
}

- (void)databaseNeedsMigration:(void (^)(BOOL needsMigration))completion {
  // Check if we even have a DB to migrate
  if (!DataController.defaultStoreExists) {
    completion(NO);
    return;
  }
  // Have we set the pref saying ledger has alaready initialized successfully?
  if ([self.prefs[kMigrationSucceeded] boolValue]) {
    completion(NO);
    return;
  }
  // Can we even check the DB
  if (!rewardsDatabase) {
    BLOG(3, @"DB: No rewards database object");
    completion(YES);
    return;
  }
  // Check integrity of the new DB. Safe to assume if `publisher_info` table
  // exists, then all the others do as well.
  auto transaction = brave_rewards::mojom::DBTransaction::New();
  const auto command = brave_rewards::mojom::DBCommand::New();
  command->type = brave_rewards::mojom::DBCommand::Type::READ;
  command->command = "SELECT name FROM sqlite_master WHERE type = 'table' AND "
                     "name = 'publisher_info';";
  command->record_bindings = {
      brave_rewards::mojom::DBCommand::RecordBindingType::STRING_TYPE};
  transaction->commands.push_back(command->Clone());

  [self runDBTransaction:std::move(transaction)
                callback:base::BindOnce(^(
                             brave_rewards::mojom::DBCommandResponsePtr
                                 response) {
                  // Failed to even run the check, tables probably don't exist,
                  // restart from scratch
                  if (response->status !=
                      brave_rewards::mojom::DBCommandResponse::Status::
                          RESPONSE_OK) {
                    [self resetRewardsDatabase];
                    BLOG(3, @"DB: Failed to run transaction with status: %d",
                         response->status);
                    completion(YES);
                    return;
                  }

                  const auto record =
                      std::move(response->result->get_records());
                  // sqlite_master table exists, but the publisher_info table
                  // doesn't exist? Restart from scratch
                  if (record.empty() || record.front()->fields.empty()) {
                    [self resetRewardsDatabase];
                    BLOG(3, @"DB: Migrate because we couldnt find tables in "
                            @"sqlite_master");
                    completion(YES);
                    return;
                  }

                  // Tables exist so migration has happened already, but somehow
                  // the flag wasn't saved.
                  self.prefs[kMigrationSucceeded] = @(YES);
                  [self savePrefs];

                  completion(NO);
                })];
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
  rewardsDatabase = base::SequenceBound<brave_rewards::core::LedgerDatabase>(
      databaseQueue, base::FilePath(base::SysNSStringToUTF8(dbPath)));
}

- (void)getCreateScript:(brave_rewards::core::GetCreateScriptCallback)callback {
  NSString* migrationScript = @"";
  switch (self.migrationType) {
    case BATLedgerDatabaseMigrationTypeNone:
      // We shouldn't be migrating, therefore doesn't make sense that
      // `getCreateScript` was called
      BLOG(0,
           @"DB: Attempted CoreData migration with an empty migration script");
      break;
    case BATLedgerDatabaseMigrationTypeTokensOnly:
      migrationScript =
          [BATLedgerDatabase migrateCoreDataBATOnlyToSQLTransaction];
      break;
    case BATLedgerDatabaseMigrationTypeDefault:
    default:
      migrationScript = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  }
  callback(base::SysNSStringToUTF8(migrationScript), 10);
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
  ledger->CreateRewardsWallet(
      "", base::BindOnce(^(
              brave_rewards::mojom::CreateRewardsWalletResult create_result) {
        const auto strongSelf = weakSelf;
        if (!strongSelf) {
          return;
        }

        brave_rewards::mojom::Result result =
            create_result ==
                    brave_rewards::mojom::CreateRewardsWalletResult::kSuccess
                ? brave_rewards::mojom::Result::LEDGER_OK
                : brave_rewards::mojom::Result::LEDGER_ERROR;

        NSError* error = nil;
        if (result != brave_rewards::mojom::Result::LEDGER_OK) {
          std::map<brave_rewards::mojom::Result, std::string> errorDescriptions{
              {brave_rewards::mojom::Result::LEDGER_ERROR,
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
              observer.walletInitalized(
                  static_cast<BraveRewardsResult>(result));
            }
          }
        });
      }));
}

- (void)currentWalletInfo:
    (void (^)(BraveRewardsRewardsWallet* _Nullable wallet))completion {
  ledger->GetRewardsWallet(^(brave_rewards::mojom::RewardsWalletPtr wallet) {
    if (wallet.get() == nullptr) {
      completion(nil);
      return;
    }
    const auto bridgedWallet =
        [[BraveRewardsRewardsWallet alloc] initWithRewardsWallet:*wallet];
    completion(bridgedWallet);
  });
}

- (void)getRewardsParameters:
    (void (^)(BraveRewardsRewardsParameters* _Nullable))completion {
  ledger->GetRewardsParameters(base::BindOnce(^(
      brave_rewards::mojom::RewardsParametersPtr info) {
    if (info) {
      self.rewardsParameters = [[BraveRewardsRewardsParameters alloc]
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
}

- (void)fetchBalance:(void (^)(BraveRewardsBalance* _Nullable))completion {
  const auto __weak weakSelf = self;
  ledger->FetchBalance(base::BindOnce(
      ^(base::expected<brave_rewards::mojom::BalancePtr,
                       brave_rewards::mojom::FetchBalanceError> result) {
        const auto strongSelf = weakSelf;
        if (result.has_value()) {
          strongSelf.balance = [[BraveRewardsBalance alloc]
              initWithBalancePtr:std::move(result.value())];
        }
        dispatch_async(dispatch_get_main_queue(), ^{
          for (BraveLedgerObserver* observer in [self.observers copy]) {
            if (observer.fetchedBalance) {
              observer.fetchedBalance();
            }
          }
          if (completion) {
            completion(strongSelf.balance);
          }
        });
      }));
}

- (void)pendingContributionsTotal:(void (^)(double amount))completion {
  ledger->GetPendingContributionsTotal(^(double total) {
    completion(total);
  });
}

- (void)drainStatusForDrainId:(NSString*)drainId
                   completion:
                       (void (^)(BraveRewardsResult result,
                                 BraveRewardsDrainStatus status))completion {
  ledger->GetDrainStatus(base::SysNSStringToUTF8(drainId), ^(
                             brave_rewards::mojom::Result result,
                             brave_rewards::mojom::DrainStatus status) {
    completion(static_cast<BraveRewardsResult>(result),
               static_cast<BraveRewardsDrainStatus>(status));
  });
}

- (std::string)getLegacyWallet {
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
  return wallet;
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
    ledger->GetExcludedList(
        ^(std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
          const auto publishers = NSArrayFromVector(
              &list, ^BraveRewardsPublisherInfo*(
                  const brave_rewards::mojom::PublisherInfoPtr& info) {
                return [[BraveRewardsPublisherInfo alloc]
                    initWithPublisherInfo:*info];
              });
          completion(publishers);
        });
  } else {
    ledger->GetActivityInfoList(
        start, limit, std::move(cppFilter),
        ^(std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
          const auto publishers = NSArrayFromVector(
              &list, ^BraveRewardsPublisherInfo*(
                  const brave_rewards::mojom::PublisherInfoPtr& info) {
                return [[BraveRewardsPublisherInfo alloc]
                    initWithPublisherInfo:*info];
              });
          completion(publishers);
        });
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

  ledger->GetPublisherActivityFromUrl(tabId, std::move(visitData), blob);
}

- (void)updatePublisherExclusionState:(NSString*)publisherId
                                state:(BraveRewardsPublisherExclude)state {
  ledger->SetPublisherExclude(
      base::SysNSStringToUTF8(publisherId),
      (brave_rewards::mojom::PublisherExclude)state,
      base::BindOnce(^(brave_rewards::mojom::Result result) {
        if (result != brave_rewards::mojom::Result::LEDGER_OK) {
          return;
        }
        for (BraveLedgerObserver* observer in [self.observers copy]) {
          if (observer.excludedSitesChanged) {
            observer.excludedSitesChanged(publisherId, state);
          }
        }
      }));
}

- (void)restoreAllExcludedPublishers {
  ledger->RestorePublishers(
      base::BindOnce(^(brave_rewards::mojom::Result result) {
        if (result != brave_rewards::mojom::Result::LEDGER_OK) {
          return;
        }

        for (BraveLedgerObserver* observer in [self.observers copy]) {
          if (observer.excludedSitesChanged) {
            observer.excludedSitesChanged(
                @"-1", static_cast<BraveRewardsPublisherExclude>(
                           brave_rewards::mojom::PublisherExclude::ALL));
          }
        }
      }));
}

- (void)publisherBannerForId:(NSString*)publisherId
                  completion:
                      (void (^)(BraveRewardsPublisherBanner* _Nullable banner))
                          completion {
  ledger->GetPublisherBanner(
      base::SysNSStringToUTF8(publisherId),
      ^(brave_rewards::mojom::PublisherBannerPtr banner) {
        auto bridgedBanner = banner.get() != nullptr
                                 ? [[BraveRewardsPublisherBanner alloc]
                                       initWithPublisherBanner:*banner]
                                 : nil;
        // native libs prefixes the logo and background image with this URL
        // scheme
        const auto imagePrefix = @"chrome://rewards-image/";
        bridgedBanner.background = [bridgedBanner.background
            stringByReplacingOccurrencesOfString:imagePrefix
                                      withString:@""];
        bridgedBanner.logo =
            [bridgedBanner.logo stringByReplacingOccurrencesOfString:imagePrefix
                                                          withString:@""];
        completion(bridgedBanner);
      });
}

- (void)refreshPublisherWithId:(NSString*)publisherId
                    completion:(void (^)(BraveRewardsPublisherStatus status))
                                   completion {
  if (self.loadingPublisherList) {
    completion(BraveRewardsPublisherStatusNotVerified);
    return;
  }
  ledger->RefreshPublisher(base::SysNSStringToUTF8(publisherId), ^(
                               brave_rewards::mojom::PublisherStatus status) {
    completion(static_cast<BraveRewardsPublisherStatus>(status));
  });
}

#pragma mark - SKUs

- (void)processSKUItems:(NSArray<BraveRewardsSKUOrderItem*>*)items
             completion:(void (^)(BraveRewardsResult result,
                                  NSString* orderID))completion {
  ledger->ProcessSKU(VectorFromNSArray(items,
                                       ^brave_rewards::mojom::SKUOrderItem(
                                           BraveRewardsSKUOrderItem* item) {
                                         return *item.cppObjPtr;
                                       }),
                     brave_rewards::core::constant::kWalletUnBlinded,
                     ^(const brave_rewards::mojom::Result result,
                       const std::string& order_id) {
                       completion(static_cast<BraveRewardsResult>(result),
                                  base::SysUTF8ToNSString(order_id));
                     });
}

#pragma mark - Tips

- (void)listRecurringTips:
    (void (^)(NSArray<BraveRewardsPublisherInfo*>*))completion {
  ledger->GetRecurringTips(
      ^(std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
        const auto publishers = NSArrayFromVector(
            &list, ^BraveRewardsPublisherInfo*(
                const brave_rewards::mojom::PublisherInfoPtr& info) {
              return [[BraveRewardsPublisherInfo alloc]
                  initWithPublisherInfo:*info];
            });
        completion(publishers);
      });
}

- (void)addRecurringTipToPublisherWithId:(NSString*)publisherId
                                  amount:(double)amount
                              completion:(void (^)(BOOL success))completion {
  brave_rewards::mojom::RecurringTipPtr info =
      brave_rewards::mojom::RecurringTip::New();
  info->publisher_key = base::SysNSStringToUTF8(publisherId);
  info->amount = amount;
  info->created_at = [[NSDate date] timeIntervalSince1970];
  ledger->SaveRecurringTip(
      std::move(info), ^(brave_rewards::mojom::Result result) {
        const auto success =
            (result == brave_rewards::mojom::Result::LEDGER_OK);
        if (success) {
          for (BraveLedgerObserver* observer in [self.observers copy]) {
            if (observer.recurringTipAdded) {
              observer.recurringTipAdded(publisherId);
            }
          }
        }
        completion(success);
      });
}

- (void)removeRecurringTipForPublisherWithId:(NSString*)publisherId {
  ledger->RemoveRecurringTip(base::SysNSStringToUTF8(publisherId), ^(
                                 brave_rewards::mojom::Result result) {
    if (result == brave_rewards::mojom::Result::LEDGER_OK) {
      for (BraveLedgerObserver* observer in [self.observers copy]) {
        if (observer.recurringTipRemoved) {
          observer.recurringTipRemoved(publisherId);
        }
      }
    }
  });
}

- (void)listOneTimeTips:
    (void (^)(NSArray<BraveRewardsPublisherInfo*>*))completion {
  ledger->GetOneTimeTips(
      ^(std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
        const auto publishers = NSArrayFromVector(
            &list, ^BraveRewardsPublisherInfo*(
                const brave_rewards::mojom::PublisherInfoPtr& info) {
              return [[BraveRewardsPublisherInfo alloc]
                  initWithPublisherInfo:*info];
            });
        completion(publishers);
      });
}

- (void)tipPublisherDirectly:(BraveRewardsPublisherInfo*)publisher
                      amount:(double)amount
                    currency:(NSString*)currency
                  completion:(void (^)(BraveRewardsResult result))completion {
  ledger->OneTimeTip(base::SysNSStringToUTF8(publisher.id), amount,
                     ^(brave_rewards::mojom::Result result) {
                       completion(static_cast<BraveRewardsResult>(result));
                     });
}

#pragma mark - Grants

- (NSArray<BraveRewardsPromotion*>*)pendingPromotions {
  return [self.mPendingPromotions copy];
}

- (NSArray<BraveRewardsPromotion*>*)finishedPromotions {
  return [self.mFinishedPromotions copy];
}

- (NSString*)notificationIDForPromo:
    (const brave_rewards::mojom::PromotionPtr)promo {
  bool isUGP = promo->type == brave_rewards::mojom::PromotionType::UGP;
  const auto prefix = isUGP ? @"rewards_grant_" : @"rewards_grant_ads_";
  const auto promotionId = base::SysUTF8ToNSString(promo->id);
  return [NSString stringWithFormat:@"%@%@", prefix, promotionId];
}

- (void)updatePendingAndFinishedPromotions:(void (^)())completion {
  ledger->GetAllPromotions(^(
      base::flat_map<std::string, brave_rewards::mojom::PromotionPtr> map) {
    NSMutableArray* promos = [[NSMutableArray alloc] init];
    for (auto it = map.begin(); it != map.end(); ++it) {
      if (it->second.get() != nullptr) {
        [promos addObject:[[BraveRewardsPromotion alloc]
                              initWithPromotion:*it->second]];
      }
    }
    for (BraveRewardsPromotion* promo in [self.mPendingPromotions copy]) {
      [self
          clearNotificationWithID:[self
                                      notificationIDForPromo:promo.cppObjPtr]];
    }
    [self.mFinishedPromotions removeAllObjects];
    [self.mPendingPromotions removeAllObjects];
    for (BraveRewardsPromotion* promotion in promos) {
      if (promotion.status == BraveRewardsPromotionStatusFinished) {
        [self.mFinishedPromotions addObject:promotion];
      } else if (promotion.status == BraveRewardsPromotionStatusActive ||
                 promotion.status == BraveRewardsPromotionStatusAttested) {
        [self.mPendingPromotions addObject:promotion];
        bool isUGP = promotion.type == BraveRewardsPromotionTypeUgp;
        auto notificationKind = isUGP ? RewardsNotificationKindGrant
                                      : RewardsNotificationKindGrantAds;

        [self addNotificationOfKind:notificationKind
                           userInfo:nil
                     notificationID:[self notificationIDForPromo:promotion
                                                                     .cppObjPtr]
                           onlyOnce:YES];
      }
    }
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
}

- (void)fetchPromotions:
    (nullable void (^)(NSArray<BraveRewardsPromotion*>* grants))completion {
  ledger->FetchPromotions(base::BindOnce(
      ^(brave_rewards::mojom::Result result,
        std::vector<brave_rewards::mojom::PromotionPtr> promotions) {
        if (result != brave_rewards::mojom::Result::LEDGER_OK) {
          return;
        }
        [self updatePendingAndFinishedPromotions:^{
          if (completion) {
            completion(self.pendingPromotions);
          }
        }];
      }));
}

- (void)claimPromotion:(NSString*)promotionId
             publicKey:(NSString*)deviceCheckPublicKey
            completion:(void (^)(BraveRewardsResult result,
                                 NSString* _Nonnull nonce))completion {
  const auto payload = [NSDictionary dictionaryWithObject:deviceCheckPublicKey
                                                   forKey:@"publicKey"];
  const auto jsonData = [NSJSONSerialization dataWithJSONObject:payload
                                                        options:0
                                                          error:nil];
  if (!jsonData) {
    BLOG(0, @"Missing JSON payload while attempting to claim promotion");
    return;
  }
  const auto jsonString = [[NSString alloc] initWithData:jsonData
                                                encoding:NSUTF8StringEncoding];
  ledger->ClaimPromotion(
      base::SysNSStringToUTF8(promotionId), base::SysNSStringToUTF8(jsonString),
      base::BindOnce(
          ^(brave_rewards::mojom::Result result, const std::string& nonce) {
            const auto bridgedNonce = base::SysUTF8ToNSString(nonce);
            dispatch_async(dispatch_get_main_queue(), ^{
              completion(static_cast<BraveRewardsResult>(result), bridgedNonce);
            });
          }));
}

- (void)attestPromotion:(NSString*)promotionId
               solution:(PromotionSolution*)solution
             completion:(void (^)(BraveRewardsResult result,
                                  BraveRewardsPromotion* _Nullable promotion))
                            completion {
  ledger->AttestPromotion(
      base::SysNSStringToUTF8(promotionId),
      base::SysNSStringToUTF8(solution.JSONPayload),
      base::BindOnce(^(brave_rewards::mojom::Result result,
                       brave_rewards::mojom::PromotionPtr promotion) {
        if (promotion.get() == nullptr) {
          if (completion) {
            dispatch_async(dispatch_get_main_queue(), ^{
              completion(static_cast<BraveRewardsResult>(result), nil);
            });
          }
          return;
        }

        const auto bridgedPromotion =
            [[BraveRewardsPromotion alloc] initWithPromotion:*promotion];
        if (result == brave_rewards::mojom::Result::LEDGER_OK) {
          [self fetchBalance:nil];
          [self clearNotificationWithID:
                    [self notificationIDForPromo:std::move(promotion)]];
        }

        dispatch_async(dispatch_get_main_queue(), ^{
          if (completion) {
            completion(static_cast<BraveRewardsResult>(result),
                       bridgedPromotion);
          }
          if (result == brave_rewards::mojom::Result::LEDGER_OK) {
            for (BraveLedgerObserver* observer in [self.observers copy]) {
              if (observer.promotionClaimed) {
                observer.promotionClaimed(bridgedPromotion);
              }
            }
          }
        });
      }));
}

#pragma mark - History

- (void)balanceReportForMonth:(BraveRewardsActivityMonth)month
                         year:(int)year
                   completion:
                       (void (^)(BraveRewardsBalanceReportInfo* _Nullable info))
                           completion {
  ledger->GetBalanceReport(
      (brave_rewards::mojom::ActivityMonth)month, year,
      ^(const brave_rewards::mojom::Result result,
        brave_rewards::mojom::BalanceReportInfoPtr info) {
        auto bridgedInfo = info.get() != nullptr
                               ? [[BraveRewardsBalanceReportInfo alloc]
                                     initWithBalanceReportInfo:*info.get()]
                               : nil;
        completion(result == brave_rewards::mojom::Result::LEDGER_OK
                       ? bridgedInfo
                       : nil);
      });
}

- (nullable BraveRewardsAutoContributeProperties*)autoContributeProperties {
  brave_rewards::mojom::AutoContributePropertiesPtr props =
      ledger->GetAutoContributeProperties();
  if (!props) {
    return nil;
  }
  return [[BraveRewardsAutoContributeProperties alloc]
      initWithAutoContributePropertiesPtr:std::move(props)];
}

#pragma mark - Pending Contributions

- (void)pendingContributions:
    (void (^)(NSArray<BraveRewardsPendingContributionInfo*>* publishers))
        completion {
  ledger->GetPendingContributions(
      ^(std::vector<brave_rewards::mojom::PendingContributionInfoPtr> list) {
        const auto convetedList = NSArrayFromVector(
            &list, ^BraveRewardsPendingContributionInfo*(
                const brave_rewards::mojom::PendingContributionInfoPtr& info) {
              return [[BraveRewardsPendingContributionInfo alloc]
                  initWithPendingContributionInfo:*info];
            });
        completion(convetedList);
      });
}

- (void)removePendingContribution:(BraveRewardsPendingContributionInfo*)info
                       completion:
                           (void (^)(BraveRewardsResult result))completion {
  ledger->RemovePendingContribution(
      info.id, ^(const brave_rewards::mojom::Result result) {
        completion(static_cast<BraveRewardsResult>(result));
      });
}

- (void)removeAllPendingContributions:
    (void (^)(BraveRewardsResult result))completion {
  ledger->RemoveAllPendingContributions(
      ^(const brave_rewards::mojom::Result result) {
        completion(static_cast<BraveRewardsResult>(result));
      });
}

#pragma mark - Reconcile

- (void)onReconcileComplete:(brave_rewards::mojom::Result)result
               contribution:
                   (brave_rewards::mojom::ContributionInfoPtr)contribution {
  // TODO we changed from probi to amount, so from string to double
  if (result == brave_rewards::mojom::Result::LEDGER_OK) {
    if (contribution->type ==
        brave_rewards::mojom::RewardsType::RECURRING_TIP) {
      [self showTipsProcessedNotificationIfNeccessary];
    }
    [self fetchBalance:nil];
  }

  if ((result == brave_rewards::mojom::Result::LEDGER_OK &&
       contribution->type ==
           brave_rewards::mojom::RewardsType::AUTO_CONTRIBUTE) ||
      result == brave_rewards::mojom::Result::LEDGER_ERROR ||
      result == brave_rewards::mojom::Result::NOT_ENOUGH_FUNDS ||
      result == brave_rewards::mojom::Result::TIP_ERROR) {
    const auto contributionId =
        base::SysUTF8ToNSString(contribution->contribution_id);
    const auto info = @{
      @"viewingId" : contributionId,
      @"result" : @((BraveRewardsResult)result),
      @"type" : @((BraveRewardsRewardsType)contribution->type),
      @"amount" : [@(contribution->amount) stringValue]
    };

    [self addNotificationOfKind:RewardsNotificationKindAutoContribute
                       userInfo:info
                 notificationID:[NSString stringWithFormat:@"contribution_%@",
                                                           contributionId]];
  }

  for (BraveLedgerObserver* observer in [self.observers copy]) {
    if (observer.balanceReportUpdated) {
      observer.balanceReportUpdated();
    }
    if (observer.reconcileCompleted) {
      observer.reconcileCompleted(
          static_cast<BraveRewardsResult>(result),
          base::SysUTF8ToNSString(contribution->contribution_id),
          static_cast<BraveRewardsRewardsType>(contribution->type),
          [@(contribution->amount) stringValue]);
    }
  }
}

#pragma mark - Misc

- (void)rewardsInternalInfo:
    (void (^)(BraveRewardsRewardsInternalsInfo* _Nullable info))completion {
  ledger->GetRewardsInternalsInfo(
      ^(brave_rewards::mojom::RewardsInternalsInfoPtr info) {
        auto bridgedInfo = info.get() != nullptr
                               ? [[BraveRewardsRewardsInternalsInfo alloc]
                                     initWithRewardsInternalsInfo:*info.get()]
                               : nil;
        completion(bridgedInfo);
      });
}

- (void)allContributions:
    (void (^)(NSArray<BraveRewardsContributionInfo*>* contributions))
        completion {
  ledger->GetAllContributions(
      ^(std::vector<brave_rewards::mojom::ContributionInfoPtr> list) {
        const auto convetedList = NSArrayFromVector(
            &list, ^BraveRewardsContributionInfo*(
                const brave_rewards::mojom::ContributionInfoPtr& info) {
              return [[BraveRewardsContributionInfo alloc]
                  initWithContributionInfo:*info];
            });
        completion(convetedList);
      });
}

#pragma mark - Reporting

- (void)setSelectedTabId:(UInt32)selectedTabId {
  if (!self.initialized) {
    return;
  }

  if (_selectedTabId != selectedTabId) {
    ledger->OnHide(_selectedTabId, [[NSDate date] timeIntervalSince1970]);
  }
  _selectedTabId = selectedTabId;
  if (_selectedTabId > 0) {
    ledger->OnShow(_selectedTabId, [[NSDate date] timeIntervalSince1970]);
  }
}

- (void)applicationDidBecomeActive {
  if (!self.initialized) {
    return;
  }

  ledger->OnForeground(self.selectedTabId,
                       [[NSDate date] timeIntervalSince1970]);

  // Check if the last notification check was more than a day ago
  if (fabs([self.lastNotificationCheckDate timeIntervalSinceNow]) > kOneDay) {
    [self checkForNotificationsAndFetchGrants];
  }
}

- (void)applicationDidBackground {
  if (!self.initialized) {
    return;
  }

  ledger->OnBackground(self.selectedTabId,
                       [[NSDate date] timeIntervalSince1970]);
}

- (void)reportLoadedPageWithURL:(NSURL*)url tabId:(UInt32)tabId {
  if (!self.initialized) {
    return;
  }

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

  ledger->OnLoad(std::move(data), [[NSDate date] timeIntervalSince1970]);
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
  std::string fpu = firstPartyURL != nil
                        ? base::SysNSStringToUTF8(firstPartyURL.absoluteString)
                        : "";

  ledger->OnXHRLoad(tabId, base::SysNSStringToUTF8(url.absoluteString),
                    partsMap, fpu, ref, std::move(visit));
}

- (void)reportTabNavigationOrClosedWithTabId:(UInt32)tabId {
  if (!self.initialized) {
    return;
  }

  ledger->OnUnload(tabId, [[NSDate date] timeIntervalSince1970]);
}

#pragma mark - Preferences

BATLedgerBridge(int,
                minimumVisitDuration,
                setMinimumVisitDuration,
                GetPublisherMinVisitTime,
                SetPublisherMinVisitTime)

        BATLedgerBridge(int,
                        minimumNumberOfVisits,
                        setMinimumNumberOfVisits,
                        GetPublisherMinVisits,
                        SetPublisherMinVisits)

            BATLedgerBridge(BOOL,
                            allowUnverifiedPublishers,
                            setAllowUnverifiedPublishers,
                            GetPublisherAllowNonVerified,
                            SetPublisherAllowNonVerified)

                BATLedgerReadonlyBridge(double,
                                        contributionAmount,
                                        GetAutoContributionAmount)

    - (void)setContributionAmount : (double)contributionAmount {
  ledger->SetAutoContributionAmount(contributionAmount);
}

BATLedgerBridge(BOOL,
                isAutoContributeEnabled,
                setAutoContributeEnabled,
                GetAutoContributeEnabled,
                SetAutoContributeEnabled)

    - (void)setBooleanState : (const std::string&)name value : (bool)value {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithBool:value];
  [self savePrefs];
}

- (bool)getBooleanState:(const std::string&)name {
  const auto key = base::SysUTF8ToNSString(name);
  if (![self.prefs objectForKey:key]) {
    return NO;
  }

  return [self.prefs[key] boolValue];
}

- (void)setIntegerState:(const std::string&)name value:(int)value {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithInt:value];
  [self savePrefs];
}

- (int)getIntegerState:(const std::string&)name {
  const auto key = base::SysUTF8ToNSString(name);
  return [self.prefs[key] intValue];
}

- (void)setDoubleState:(const std::string&)name value:(double)value {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithDouble:value];
  [self savePrefs];
}

- (double)getDoubleState:(const std::string&)name {
  const auto key = base::SysUTF8ToNSString(name);
  return [self.prefs[key] doubleValue];
}

- (void)setStringState:(const std::string&)name
                 value:(const std::string&)value {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = base::SysUTF8ToNSString(value);
  [self savePrefs];
}

- (std::string)getStringState:(const std::string&)name {
  const auto key = base::SysUTF8ToNSString(name);
  const auto value = (NSString*)self.prefs[key];
  if (!value) {
    return "";
  }
  return base::SysNSStringToUTF8(value);
}

- (void)setInt64State:(const std::string&)name value:(int64_t)value {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithLongLong:value];
  [self savePrefs];
}

- (int64_t)getInt64State:(const std::string&)name {
  const auto key = base::SysUTF8ToNSString(name);
  return [self.prefs[key] longLongValue];
}

- (void)setUint64State:(const std::string&)name value:(uint64_t)value {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = [NSNumber numberWithUnsignedLongLong:value];
  [self savePrefs];
}

- (uint64_t)getUint64State:(const std::string&)name {
  const auto key = base::SysUTF8ToNSString(name);
  return [self.prefs[key] unsignedLongLongValue];
}

- (void)setValueState:(const std::string&)name value:(base::Value)value {
  std::string json;
  if (base::JSONWriter::Write(value, &json)) {
    const auto key = base::SysUTF8ToNSString(name);
    self.prefs[key] = base::SysUTF8ToNSString(json);
    [self savePrefs];
  }
}

- (base::Value)getValueState:(const std::string&)name {
  const auto key = base::SysUTF8ToNSString(name);
  const auto json = (NSString*)self.prefs[key];
  if (!json) {
    return base::Value();
  }

  auto value = base::JSONReader::Read(base::SysNSStringToUTF8(json));
  if (!value) {
    return base::Value();
  }

  return std::move(*value);
}

- (void)setTimeState:(const std::string&)name time:(base::Time)time {
  const auto key = base::SysUTF8ToNSString(name);
  self.prefs[key] = @(time.ToDoubleT());
  [self savePrefs];
}

- (base::Time)getTimeState:(const std::string&)name {
  const auto key = base::SysUTF8ToNSString(name);
  return base::Time::FromDoubleT([self.prefs[key] doubleValue]);
}

- (void)clearState:(const std::string&)name {
  const auto key = base::SysUTF8ToNSString(name);
  [self.prefs removeObjectForKey:key];
  [self savePrefs];
}

- (bool)getBooleanOption:(const std::string&)name {
  DCHECK(!name.empty());

  const auto it = kBoolOptions.find(name);
  DCHECK(it != kBoolOptions.end());

  return kBoolOptions.at(name);
}

- (int)getIntegerOption:(const std::string&)name {
  DCHECK(!name.empty());

  const auto it = kIntegerOptions.find(name);
  DCHECK(it != kIntegerOptions.end());

  return kIntegerOptions.at(name);
}

- (double)getDoubleOption:(const std::string&)name {
  DCHECK(!name.empty());

  const auto it = kDoubleOptions.find(name);
  DCHECK(it != kDoubleOptions.end());

  return kDoubleOptions.at(name);
}

- (std::string)getStringOption:(const std::string&)name {
  DCHECK(!name.empty());

  const auto it = kStringOptions.find(name);
  DCHECK(it != kStringOptions.end());

  return kStringOptions.at(name);
}

- (int64_t)getInt64Option:(const std::string&)name {
  DCHECK(!name.empty());

  const auto it = kInt64Options.find(name);
  DCHECK(it != kInt64Options.end());

  return kInt64Options.at(name);
}

- (uint64_t)getUint64Option:(const std::string&)name {
  DCHECK(!name.empty());

  const auto it = kUInt64Options.find(name);
  DCHECK(it != kUInt64Options.end());

  return kUInt64Options.at(name);
}

#pragma mark - Notifications

- (NSArray<RewardsNotification*>*)notifications {
  return [self.mNotifications copy];
}

- (void)clearNotificationWithID:(NSString*)notificationID {
  for (RewardsNotification* n in self.notifications) {
    if ([n.id isEqualToString:notificationID]) {
      [self clearNotification:n];
      return;
    }
  }
}

- (void)clearNotification:(RewardsNotification*)notification {
  [self.mNotifications removeObject:notification];
  [self writeNotificationsToDisk];

  for (BraveLedgerObserver* observer in [self.observers copy]) {
    if (observer.notificationsRemoved) {
      observer.notificationsRemoved(@[ notification ]);
    }
  }
}

- (void)clearAllNotifications {
  NSArray* notifications = [self.mNotifications copy];
  [self.mNotifications removeAllObjects];
  [self writeNotificationsToDisk];

  for (BraveLedgerObserver* observer in [self.observers copy]) {
    if (observer.notificationsRemoved) {
      observer.notificationsRemoved(notifications);
    }
  }
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

- (void)showTipsProcessedNotificationIfNeccessary {
  if (!self.autoContributeEnabled) {
    return;
  }
  [self addNotificationOfKind:RewardsNotificationKindTipsProcessed
                     userInfo:nil
               notificationID:@"rewards_notification_tips_processed"];
}

- (void)addNotificationOfKind:(RewardsNotificationKind)kind
                     userInfo:(nullable NSDictionary*)userInfo
               notificationID:(nullable NSString*)identifier {
  [self addNotificationOfKind:kind
                     userInfo:userInfo
               notificationID:identifier
                     onlyOnce:NO];
}

- (void)addNotificationOfKind:(RewardsNotificationKind)kind
                     userInfo:(nullable NSDictionary*)userInfo
               notificationID:(nullable NSString*)identifier
                     onlyOnce:(BOOL)onlyOnce {
  NSParameterAssert(kind != RewardsNotificationKindInvalid);
  NSString* notificationID = [identifier copy];
  if (!identifier || identifier.length == 0) {
    notificationID = [NSUUID UUID].UUIDString;
  } else if (onlyOnce) {
    const auto idx = [self.mNotifications
        indexOfObjectPassingTest:^BOOL(RewardsNotification* _Nonnull obj,
                                       NSUInteger index, BOOL* _Nonnull stop) {
          return obj.displayed && [obj.id isEqualToString:identifier];
        }];
    if (idx != NSNotFound) {
      return;
    }
  }

  const auto notification = [[RewardsNotification alloc]
      initWithID:notificationID
       dateAdded:[[NSDate date] timeIntervalSince1970]
            kind:kind
        userInfo:userInfo];
  if (onlyOnce) {
    notification.displayed = YES;
  }

  [self.mNotifications addObject:notification];

  // Post to observers
  for (BraveLedgerObserver* observer in [self.observers copy]) {
    if (observer.notificationAdded) {
      observer.notificationAdded(notification);
    }
  }

  [NSNotificationCenter.defaultCenter
      postNotificationName:BraveLedgerNotificationAdded
                    object:nil];

  [self writeNotificationsToDisk];
}

- (void)readNotificationsFromDisk {
  const auto path =
      [self.storagePath stringByAppendingPathComponent:@"notifications"];
  const auto data = [NSData dataWithContentsOfFile:path];
  if (!data) {
    // Nothing to read
    self.mNotifications = [[NSMutableArray alloc] init];
    return;
  }

  NSError* error;
  self.mNotifications = [NSKeyedUnarchiver unarchivedObjectOfClass:NSArray.self
                                                          fromData:data
                                                             error:&error];
  if (!self.mNotifications) {
    self.mNotifications = [[NSMutableArray alloc] init];
    if (error) {
      BLOG(0, @"Failed to unarchive notifications on disk: %@",
           error.debugDescription);
    }
  }
}

- (void)writeNotificationsToDisk {
  const auto path =
      [self.storagePath stringByAppendingPathComponent:@"notifications"];
  if (self.notifications.count == 0) {
    // Nothing to write, delete anything we have stored
    if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
      [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
    }
    return;
  }

  NSError* error;
  const auto data =
      [NSKeyedArchiver archivedDataWithRootObject:self.notifications
                            requiringSecureCoding:YES
                                            error:&error];
  if (!data) {
    if (error) {
      BLOG(0, @"Failed to write notifications to disk: %@",
           error.debugDescription);
    }
    return;
  }

  [data writeToURL:[NSURL fileURLWithPath:path isDirectory:NO]
           options:NSDataWritingAtomic
             error:nil];
}

#pragma mark - State

- (void)loadLedgerState:(brave_rewards::core::OnLoadCallback)callback {
  const auto contents =
      [self.commonOps loadContentsFromFileWithName:"ledger_state.json"];
  if (contents.length() > 0) {
    callback(brave_rewards::mojom::Result::LEDGER_OK, contents);
  } else {
    callback(brave_rewards::mojom::Result::NO_LEDGER_STATE, contents);
  }
  [self startNotificationTimers];
}

- (void)loadPublisherState:(brave_rewards::core::OnLoadCallback)callback {
  const auto contents =
      [self.commonOps loadContentsFromFileWithName:"publisher_state.json"];
  if (contents.length() > 0) {
    callback(brave_rewards::mojom::Result::LEDGER_OK, contents);
  } else {
    callback(brave_rewards::mojom::Result::NO_PUBLISHER_STATE, contents);
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

- (void)loadURL:(brave_rewards::mojom::UrlRequestPtr)request
       callback:(brave_rewards::core::LoadURLCallback)callback {
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
              brave_rewards::mojom::UrlResponse url_response;
              url_response.url = base::SysNSStringToUTF8(copiedURL);
              url_response.error = errorDescription;
              url_response.status_code = statusCode;
              url_response.body = response;
              url_response.headers = headers;

              if (cb) {
                std::move(*cb).Run(url_response);
              }
            }];
}

- (std::string)URIEncode:(const std::string&)value {
  const auto allowedCharacters =
      [NSMutableCharacterSet alphanumericCharacterSet];
  [allowedCharacters addCharactersInString:@"-._~"];
  const auto string = base::SysUTF8ToNSString(value);
  const auto encoded = [string
      stringByAddingPercentEncodingWithAllowedCharacters:allowedCharacters];
  return base::SysNSStringToUTF8(encoded);
}

- (void)fetchFavIcon:(const std::string&)url
          faviconKey:(const std::string&)favicon_key
            callback:(brave_rewards::core::FetchIconCallback)callback {
  const auto pageURL = [NSURL URLWithString:base::SysUTF8ToNSString(url)];
  if (!self.faviconFetcher || !pageURL) {
    dispatch_async(dispatch_get_main_queue(), ^{
      callback(NO, std::string());
    });
    return;
  }
  self.faviconFetcher(pageURL, ^(NSURL* _Nullable faviconURL) {
    dispatch_async(dispatch_get_main_queue(), ^{
      callback(faviconURL != nil,
               base::SysNSStringToUTF8(faviconURL.absoluteString));
    });
  });
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

#pragma mark - Publisher Database

- (void)handlePublisherListing:(NSArray<BraveRewardsPublisherInfo*>*)publishers
                         start:(uint32_t)start
                         limit:(uint32_t)limit
                      callback:(brave_rewards::core::PublisherInfoListCallback)
                                   callback {
  callback(VectorFromNSArray(
      publishers,
      ^brave_rewards::mojom::PublisherInfoPtr(BraveRewardsPublisherInfo* info) {
        return info.cppObjPtr;
      }));
}
- (void)publisherListNormalized:
    (std::vector<brave_rewards::mojom::PublisherInfoPtr>)list {
  const auto list_converted = NSArrayFromVector(
      &list, ^BraveRewardsPublisherInfo*(
          const brave_rewards::mojom::PublisherInfoPtr& info) {
        return [[BraveRewardsPublisherInfo alloc] initWithPublisherInfo:*info];
      });

  for (BraveLedgerObserver* observer in [self.observers copy]) {
    if (observer.publisherListNormalized) {
      observer.publisherListNormalized(list_converted);
    }
  }
}

- (void)onPanelPublisherInfo:(brave_rewards::mojom::Result)result
               publisherInfo:
                   (brave_rewards::mojom::PublisherInfoPtr)publisher_info
                    windowId:(uint64_t)windowId {
  if (publisher_info.get() == nullptr ||
      result != brave_rewards::mojom::Result::LEDGER_OK) {
    return;
  }
  auto info =
      [[BraveRewardsPublisherInfo alloc] initWithPublisherInfo:*publisher_info];
  for (BraveLedgerObserver* observer in [self.observers copy]) {
    if (observer.fetchedPanelPublisher) {
      observer.fetchedPanelPublisher(info, windowId);
    }
  }
}

- (void)onContributeUnverifiedPublishers:(brave_rewards::mojom::Result)result
                            publisherKey:(const std::string&)publisher_key
                           publisherName:(const std::string&)publisher_name {
  switch (result) {
    case brave_rewards::mojom::Result::PENDING_PUBLISHER_REMOVED: {
      const auto publisherID = base::SysUTF8ToNSString(publisher_key);
      for (BraveLedgerObserver* observer in [self.observers copy]) {
        if (observer.pendingContributionsRemoved) {
          observer.pendingContributionsRemoved(@[ publisherID ]);
        }
      }
      break;
    }
    case brave_rewards::mojom::Result::VERIFIED_PUBLISHER: {
      const auto notificationID =
          [NSString stringWithFormat:@"verified_publisher_%@",
                                     base::SysUTF8ToNSString(publisher_key)];
      const auto name = base::SysUTF8ToNSString(publisher_name);
      [self addNotificationOfKind:RewardsNotificationKindVerifiedPublisher
                         userInfo:@{@"publisher_name" : name}
                   notificationID:notificationID];
      break;
    }
    default:
      break;
  }
}

- (void)showNotification:(const std::string&)type
                    args:(const std::vector<std::string>&)args
                callback:(brave_rewards::core::LegacyResultCallback)callback {
  const auto notificationID = base::SysUTF8ToNSString(type);
  const auto info = [[NSMutableDictionary<NSNumber*, NSString*> alloc] init];
  for (NSUInteger i = 0; i < args.size(); i++) {
    info[@(i)] = base::SysUTF8ToNSString(args[i]);
  }
  [self addNotificationOfKind:RewardsNotificationKindGeneralLedger
                     userInfo:info
               notificationID:notificationID
                     onlyOnce:NO];
}
- (brave_rewards::mojom::ClientInfoPtr)getClientInfo {
  auto info = brave_rewards::mojom::ClientInfo::New();
  info->os = brave_rewards::mojom::OperatingSystem::UNDEFINED;
  info->platform = brave_rewards::mojom::Platform::IOS;
  return info;
}

- (void)unblindedTokensReady {
  [self fetchBalance:nil];
  for (BraveLedgerObserver* observer in [self.observers copy]) {
    if (observer.balanceReportUpdated) {
      observer.balanceReportUpdated();
    }
  }
}

- (void)reconcileStampReset {
  for (BraveLedgerObserver* observer in [self.observers copy]) {
    if (observer.reconcileStampReset) {
      observer.reconcileStampReset();
    }
  }
}

- (void)runDBTransaction:(brave_rewards::mojom::DBTransactionPtr)transaction
                callback:
                    (brave_rewards::core::RunDBTransactionCallback)callback {
  __weak BraveLedger* weakSelf = self;
  DCHECK(rewardsDatabase);
  rewardsDatabase
      .AsyncCall(&brave_rewards::core::LedgerDatabase::RunTransaction)
      .WithArgs(std::move(transaction))
      .Then(base::BindOnce(
          ^(brave_rewards::core::RunDBTransactionCallback completion,
            brave_rewards::mojom::DBCommandResponsePtr response) {
            if (weakSelf)
              std::move(completion).Run(std::move(response));
          },
          std::move(callback)));
}

- (void)pendingContributionSaved:(const brave_rewards::mojom::Result)result {
  for (BraveLedgerObserver* observer in [self.observers copy]) {
    if (observer.pendingContributionAdded) {
      observer.pendingContributionAdded();
    }
  }
}

- (void)walletDisconnected:(const std::string&)wallet_type {
  const auto bridgedType =
      static_cast<ExternalWalletType>(base::SysUTF8ToNSString(wallet_type));
  for (BraveLedgerObserver* observer in self.observers) {
    if (observer.externalWalletDisconnected) {
      observer.externalWalletDisconnected(bridgedType);
    }
  }
}

- (void)deleteLog:(brave_rewards::core::LegacyResultCallback)callback {
  callback(brave_rewards::mojom::Result::LEDGER_OK);
}

- (absl::optional<std::string>)encryptString:(const std::string&)value {
  std::string encrypted_value;
  if (!OSCrypt::EncryptString(value, &encrypted_value)) {
    return {};
  }
  return encrypted_value;
}

- (absl::optional<std::string>)decryptString:(const std::string&)value {
  std::string decrypted_value;
  if (!OSCrypt::DecryptString(value, &decrypted_value)) {
    return {};
  }
  return decrypted_value;
}

@end
