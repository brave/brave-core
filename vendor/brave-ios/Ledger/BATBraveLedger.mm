/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <UIKit/UIKit.h>
#import "bat/ledger/ledger.h"
#import "bat/ledger/ledger_database.h"
#import "bat/ledger/global_constants.h"
#import "bat/ledger/option_keys.h"

#import "ledger.mojom.objc+private.h"

#import "BATBraveLedger.h"
#import "BATBraveAds.h"
#import "BATBraveAds+Private.h"
#import "BATCommonOperations.h"
#import "NSURL+Extensions.h"

#import "NativeLedgerClient.h"
#import "NativeLedgerClientBridge.h"
#import "CppTransformations.h"

#import <objc/runtime.h>

#import "BATLedgerDatabase.h"
#import "DataController.h"

#import "base/containers/flat_map.h"
#import "base/time/time.h"
#import "url/gurl.h"
#import "net/base/registry_controlled_domains/registry_controlled_domain.h"
#import "base/strings/sys_string_conversions.h"
#import "components/os_crypt/os_crypt.h"

#import "base/base64.h"
#import "base/command_line.h"
#import "base/i18n/icu_util.h"
#import "base/ios/ios_util.h"
#include "base/sequenced_task_runner.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/task_runner_util.h"

#import "RewardsLogging.h"

#define BATLedgerReadonlyBridge(__type, __objc_getter, __cpp_getter) \
- (__type)__objc_getter { return ledger->__cpp_getter(); }

#define BATLedgerBridge(__type, __objc_getter, __objc_setter, __cpp_getter, __cpp_setter) \
- (__type)__objc_getter { return ledger->__cpp_getter(); } \
- (void)__objc_setter:(__type)newValue { ledger->__cpp_setter(newValue); }

#define BATClassLedgerBridge(__type, __objc_getter, __objc_setter, __cpp_var) \
+ (__type)__objc_getter { return ledger::__cpp_var; } \
+ (void)__objc_setter:(__type)newValue { ledger::__cpp_var = newValue; }

NSString * const BATBraveLedgerErrorDomain = @"BATBraveLedgerErrorDomain";
NSNotificationName const BATBraveLedgerNotificationAdded = @"BATBraveLedgerNotificationAdded";

BATBraveGeneralLedgerNotificationID const BATBraveGeneralLedgerNotificationIDWalletNowVerified = @"wallet_new_verified";
BATBraveGeneralLedgerNotificationID const BATBraveGeneralLedgerNotificationIDWalletDisconnected = @"wallet_disconnected";

static NSString * const kNextAddFundsDateNotificationKey = @"BATNextAddFundsDateNotification";
static NSString * const kBackupNotificationIntervalKey = @"BATBackupNotificationInterval";
static NSString * const kBackupNotificationFrequencyKey = @"BATBackupNotificationFrequency";
static NSString * const kUserHasFundedKey = @"BATRewardsUserHasFunded";
static NSString * const kBackupSucceededKey = @"BATRewardsBackupSucceeded";
static NSString * const kMigrationSucceeded = @"BATRewardsMigrationSucceeded";

static NSString * const kContributionQueueAutoincrementID = @"BATContributionQueueAutoincrementID";
static NSString * const kUnblindedTokenAutoincrementID = @"BATUnblindedTokenAutoincrementID";

static NSString * const kExternalWalletsPrefKey = @"external_wallets";
static NSString * const kTransferFeesPrefKey = @"transfer_fees";

static const auto kOneDay = base::Time::kHoursPerDay * base::Time::kSecondsPerHour;

/// Ledger Prefs, keys will be defined in `bat/ledger/option_keys.h`
const std::map<std::string, bool> kBoolOptions = {
    {ledger::option::kClaimUGP, true},
    {ledger::option::kContributionsDisabledForBAPMigration, false},
    {ledger::option::kIsBitflyerRegion, false}};
const std::map<std::string, int> kIntegerOptions = {};
const std::map<std::string, double> kDoubleOptions = {};
const std::map<std::string, std::string> kStringOptions = {};
const std::map<std::string, int64_t> kInt64Options = {};
const std::map<std::string, uint64_t> kUInt64Options = {
  {ledger::option::kPublisherListRefreshInterval,
    7 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour}
};
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

namespace {

ledger::type::DBCommandResponsePtr RunDBTransactionOnTaskRunner(
    ledger::type::DBTransactionPtr transaction,
    ledger::LedgerDatabase* database) {
  auto response = ledger::type::DBCommandResponse::New();
  if (!database) {
    response->status = ledger::type::DBCommandResponse::Status::RESPONSE_ERROR;
  } else {
    database->RunTransaction(std::move(transaction), response.get());
  }

  return response;
}

}  // namespace

@interface BATBraveLedger () <NativeLedgerClientBridge> {
  NativeLedgerClient *ledgerClient;
  ledger::Ledger *ledger;
  ledger::LedgerDatabase *rewardsDatabase;
  scoped_refptr<base::SequencedTaskRunner> databaseQueue;
}

@property (nonatomic, copy) NSString *storagePath;
@property (nonatomic) BATRewardsParameters *rewardsParameters;
@property (nonatomic) BATBalance *balance;
@property(nonatomic) BATExternalWallet* upholdWallet;
@property (nonatomic) dispatch_queue_t fileWriteThread;
@property (nonatomic) NSMutableDictionary<NSString *, NSString *> *state;
@property (nonatomic) BATCommonOperations *commonOps;
@property (nonatomic) NSMutableDictionary<NSString *, __kindof NSObject *> *prefs;

@property (nonatomic) NSMutableArray<BATPromotion *> *mPendingPromotions;
@property (nonatomic) NSMutableArray<BATPromotion *> *mFinishedPromotions;

@property (nonatomic) NSHashTable<BATBraveLedgerObserver *> *observers;

@property (nonatomic, getter=isInitialized) BOOL initialized;
@property (nonatomic) BOOL initializing;
@property (nonatomic) BOOL dataMigrationFailed;
@property (nonatomic) BATResult initializationResult;
@property (nonatomic, getter=isLoadingPublisherList) BOOL loadingPublisherList;
@property (nonatomic, getter=isInitializingWallet) BOOL initializingWallet;
@property (nonatomic) BATLedgerDatabaseMigrationType migrationType;

/// Notifications

@property (nonatomic) NSMutableArray<BATRewardsNotification *> *mNotifications;
@property (nonatomic) NSTimer *notificationStartupTimer;
@property (nonatomic) NSDate *lastNotificationCheckDate;

/// Temporary blocks

@end

@implementation BATBraveLedger

- (instancetype)initWithStateStoragePath:(NSString *)path
{
  if ((self = [super init])) {
    self.storagePath = path;
    self.commonOps = [[BATCommonOperations alloc] initWithStoragePath:path];
    self.state = [[NSMutableDictionary alloc] initWithContentsOfFile:self.randomStatePath] ?: [[NSMutableDictionary alloc] init];
    self.fileWriteThread = dispatch_queue_create("com.rewards.file-write", DISPATCH_QUEUE_SERIAL);
    self.mPendingPromotions = [[NSMutableArray alloc] init];
    self.mFinishedPromotions = [[NSMutableArray alloc] init];
    self.observers = [NSHashTable weakObjectsHashTable];
    rewardsDatabase = nullptr;

    self.prefs = [[NSMutableDictionary alloc] initWithContentsOfFile:[self prefsPath]];
    if (!self.prefs) {
      self.prefs = [[NSMutableDictionary alloc] init];
      // Setup defaults
      self.prefs[kNextAddFundsDateNotificationKey] = @([[NSDate date] timeIntervalSince1970]);
      self.prefs[kBackupNotificationFrequencyKey] = @(7 * kOneDay); // 7 days
      self.prefs[kBackupNotificationIntervalKey] = @(7 * kOneDay); // 7 days
      self.prefs[kBackupSucceededKey] = @(NO);
      self.prefs[kUserHasFundedKey] = @(NO);
      self.prefs[kMigrationSucceeded] = @(NO);
      [self savePrefs];
    }

    const auto args = [NSProcessInfo processInfo].arguments;
    const char *argv[args.count];
    for (NSInteger i = 0; i < args.count; i++) {
      argv[i] = args[i].UTF8String;
    }

    databaseQueue = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

    const auto* dbPath = [self rewardsDatabasePath].UTF8String;
    rewardsDatabase = ledger::LedgerDatabase::CreateInstance(base::FilePath(dbPath));

    ledgerClient = new NativeLedgerClient(self);
    ledger = ledger::Ledger::CreateInstance(ledgerClient);
    
    // Add notifications for standard app foreground/background
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationDidBecomeActive) name:UIApplicationDidBecomeActiveNotification object:nil];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationDidBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];
  }
  return self;
}

- (void)dealloc
{
  [NSNotificationCenter.defaultCenter removeObserver:self];
  [self.notificationStartupTimer invalidate];

  if (rewardsDatabase) {
    databaseQueue->DeleteSoon(FROM_HERE, rewardsDatabase);
  }
  delete ledger;
  delete ledgerClient;
}

- (void)initializeLedgerService:(nullable void (^)())completion
{
  self.migrationType = BATLedgerDatabaseMigrationTypeDefault;
  [self databaseNeedsMigration:^(BOOL needsMigration) {
    if (needsMigration) {
      [BATLedgerDatabase deleteCoreDataServerPublisherList:nil];
    }
    [self initializeLedgerService:needsMigration completion:completion];
  }];
}

- (void)initializeLedgerService:(BOOL)executeMigrateScript completion:(nullable void (^)())completion
{
  if (self.initialized || self.initializing) {
    return;
  }
  self.initializing = YES;

  BLOG(3, @"DB: Migrate from CoreData? %@", (executeMigrateScript ? @"YES" : @"NO"));
  ledger->Initialize(executeMigrateScript, ^(ledger::type::Result result){
    self.initialized = (result == ledger::type::Result::LEDGER_OK ||
                        result == ledger::type::Result::NO_LEDGER_STATE ||
                        result == ledger::type::Result::NO_PUBLISHER_STATE);
    self.initializing = NO;
    if (self.initialized) {
      self.prefs[kMigrationSucceeded] = @(YES);
      [self savePrefs];

      [self getRewardsParameters:nil];
      [self fetchBalance:nil];
      [self fetchUpholdWallet:nil];
      
      [self readNotificationsFromDisk];
      
      [self.ads initializeIfAdsEnabled];
    } else {
      BLOG(0, @"Ledger Initialization Failed with error: %d", result);
      if (result == ledger::type::Result::DATABASE_INIT_FAILED) {
        // Failed to migrate data...
        switch (self.migrationType) {
          case BATLedgerDatabaseMigrationTypeDefault:
            BLOG(0, @"DB: Full migration failed, attempting BAT only migration.");
            self.dataMigrationFailed = YES;
            self.migrationType = BATLedgerDatabaseMigrationTypeTokensOnly;
            [self resetRewardsDatabase];
            // attempt re-initialize without other data
            [self initializeLedgerService:YES completion:completion];
            return;
          case BATLedgerDatabaseMigrationTypeTokensOnly:
            BLOG(0, @"DB: BAT only migration failed. Initializing without migration.");
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
    self.initializationResult = static_cast<BATResult>(result);
    if (completion) {
      completion();
    }
    for (BATBraveLedgerObserver *observer in [self.observers copy]) {
      if (observer.walletInitalized) {
        observer.walletInitalized(self.initializationResult);
      }
    }
  });
}

- (void)databaseNeedsMigration:(void (^)(BOOL needsMigration))completion
{
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
  auto transaction = ledger::type::DBTransaction::New();
  const auto command = ledger::type::DBCommand::New();
  command->type = ledger::type::DBCommand::Type::READ;
  command->command = "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'publisher_info';";
  command->record_bindings = { ledger::type::DBCommand::RecordBindingType::STRING_TYPE };
  transaction->commands.push_back(command->Clone());

  [self runDBTransaction:std::move(transaction) callback:^(ledger::type::DBCommandResponsePtr response){
    // Failed to even run the check, tables probably don't exist,
    // restart from scratch
    if (response->status != ledger::type::DBCommandResponse::Status::RESPONSE_OK) {
      [self resetRewardsDatabase];
      BLOG(3, @"DB: Failed to run transaction with status: %d", response->status);
      completion(YES);
      return;
    }

    const auto record = std::move(response->result->get_records());
    // sqlite_master table exists, but the publisher_info table doesn't exist?
    // Restart from scratch
    if (record.empty() || record.front()->fields.empty()) {
      [self resetRewardsDatabase];
      BLOG(3, @"DB: Migrate because we couldnt find tables in sqlite_master");
      completion(YES);
      return;
    }

    // Tables exist so migration has happened already, but somehow the flag wasn't
    // saved.
    self.prefs[kMigrationSucceeded] = @(YES);
    [self savePrefs];

    completion(NO);
  }];
}

- (NSString *)rewardsDatabasePath
{
  return [self.storagePath stringByAppendingPathComponent:@"Rewards.db"];
}

- (void)resetRewardsDatabase
{
  delete rewardsDatabase;
  const auto dbPath = [self rewardsDatabasePath];
  [NSFileManager.defaultManager removeItemAtPath:dbPath error:nil];
  [NSFileManager.defaultManager removeItemAtPath:[dbPath stringByAppendingString:@"-journal"] error:nil];
  rewardsDatabase = ledger::LedgerDatabase::CreateInstance(base::FilePath(dbPath.UTF8String));
}

- (void)getCreateScript:(ledger::client::GetCreateScriptCallback)callback
{
  NSString *migrationScript = @"";
  switch (self.migrationType) {
    case BATLedgerDatabaseMigrationTypeNone:
      // We shouldn't be migrating, therefore doesn't make sense that
      // `getCreateScript` was called
      BLOG(0, @"DB: Attempted CoreData migration with an empty migration script");
      break;
    case BATLedgerDatabaseMigrationTypeTokensOnly:
      migrationScript = [BATLedgerDatabase migrateCoreDataBATOnlyToSQLTransaction];
      break;
    case BATLedgerDatabaseMigrationTypeDefault:
    default:
      migrationScript = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  }
  callback(migrationScript.UTF8String, 10);
}

- (NSString *)randomStatePath
{
  return [self.storagePath stringByAppendingPathComponent:@"random_state.plist"];
}

- (NSString *)prefsPath
{
  return [self.storagePath stringByAppendingPathComponent:@"ledger_pref.plist"];
}

- (void)savePrefs
{
  NSDictionary *prefs = [self.prefs copy];
  NSString *path = [[self prefsPath] copy];
  dispatch_async(self.fileWriteThread, ^{
    [prefs writeToURL:[NSURL fileURLWithPath:path isDirectory:NO] error:nil];
  });
}

#pragma mark - Observers

- (void)addObserver:(BATBraveLedgerObserver *)observer
{
  [self.observers addObject:observer];
}

- (void)removeObserver:(BATBraveLedgerObserver *)observer
{
  [self.observers removeObject:observer];
}

#pragma mark - Global

BATClassLedgerBridge(BOOL, isDebug, setDebug, is_debug)
BATClassLedgerBridge(BOOL, isTesting, setTesting, is_testing)
BATClassLedgerBridge(int, reconcileInterval, setReconcileInterval, reconcile_interval)
BATClassLedgerBridge(BOOL, useShortRetries, setUseShortRetries, short_retries)

+ (BATEnvironment)environment
{
  return static_cast<BATEnvironment>(ledger::_environment);
}

+ (void)setEnvironment:(BATEnvironment)environment
{
  ledger::_environment = static_cast<ledger::type::Environment>(environment);
}

#pragma mark - Wallet

- (void)createWallet:(void (^)(NSError * _Nullable))completion
{
  const auto __weak weakSelf = self;
  // Results that can come from CreateWallet():
  //   - WALLET_CREATED: Good to go
  //   - LEDGER_ERROR: Already initialized
  //   - BAD_REGISTRATION_RESPONSE: Request credentials call failure or malformed data
  //   - REGISTRATION_VERIFICATION_FAILED: Missing master user token
  self.initializingWallet = YES;
  ledger->CreateWallet(^(ledger::type::Result result) {
    const auto strongSelf = weakSelf;
    if (!strongSelf) { return; }
    NSError *error = nil;
    if (result != ledger::type::Result::WALLET_CREATED) {
      std::map<ledger::type::Result, std::string> errorDescriptions {
        { ledger::type::Result::LEDGER_ERROR, "The wallet was already initialized" },
        { ledger::type::Result::BAD_REGISTRATION_RESPONSE, "Request credentials call failure or malformed data" },
        { ledger::type::Result::REGISTRATION_VERIFICATION_FAILED, "Missing master user token from registered persona" },
      };
      NSDictionary *userInfo = @{};
      const auto description = errorDescriptions[static_cast<ledger::type::Result>(result)];
      if (description.length() > 0) {
        userInfo = @{ NSLocalizedDescriptionKey: [NSString stringWithUTF8String:description.c_str()] };
      }
      error = [NSError errorWithDomain:BATBraveLedgerErrorDomain code:static_cast<NSInteger>(result) userInfo:userInfo];
    }

    [strongSelf startNotificationTimers];
    strongSelf.initializingWallet = NO;

    dispatch_async(dispatch_get_main_queue(), ^{
      if (completion) {
        completion(error);
      }

      for (BATBraveLedgerObserver *observer in [strongSelf.observers copy]) {
        if (observer.walletInitalized) {
          observer.walletInitalized(static_cast<BATResult>(result));
        }
      }
    });
  });
}

- (void)currentWalletInfo:(void (^)(BATBraveWallet *_Nullable wallet))completion
{
  ledger->GetBraveWallet(^(ledger::type::BraveWalletPtr wallet){
    if (wallet.get() == nullptr) {
      completion(nil);
      return;
    }
    const auto bridgedWallet = [[BATBraveWallet alloc] initWithBraveWallet:*wallet];
    completion(bridgedWallet);
  });
}

- (void)getRewardsParameters:(void (^)(BATRewardsParameters * _Nullable))completion
{
  ledger->GetRewardsParameters(^(ledger::type::RewardsParametersPtr info) {
    if (info) {
      self.rewardsParameters = [[BATRewardsParameters alloc] initWithRewardsParametersPtr:std::move(info)];
    } else {
      self.rewardsParameters = nil;
    }
    const auto __weak weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      if (completion) {
        completion(weakSelf.rewardsParameters);
      }
    });
  });
}

- (void)fetchBalance:(void (^)(BATBalance * _Nullable))completion
{
  const auto __weak weakSelf = self;
  ledger->FetchBalance(^(ledger::type::Result result, ledger::type::BalancePtr balance) {
    const auto strongSelf = weakSelf;
    if (result == ledger::type::Result::LEDGER_OK) {
      strongSelf.balance = [[BATBalance alloc] initWithBalancePtr:std::move(balance)];
    }
    dispatch_async(dispatch_get_main_queue(), ^{
      for (BATBraveLedgerObserver *observer in [self.observers copy]) {
        if (observer.fetchedBalance) {
          observer.fetchedBalance();
        }
      }
      if (completion) {
        completion(strongSelf.balance);
      }
    });
  });
}

- (void)recoverWalletUsingPassphrase:(NSString *)passphrase completion:(void (^)(NSError *_Nullable))completion
{
  const auto __weak weakSelf = self;
  // Results that can come from CreateWallet():
  //   - LEDGER_OK: Good to go
  //   - LEDGER_ERROR: Recovery failed
  ledger->RecoverWallet(std::string(passphrase.UTF8String),
    ^(const ledger::type::Result result) {
      const auto strongSelf = weakSelf;
      if (!strongSelf) { return; }
      NSError *error = nil;
      if (result != ledger::type::Result::LEDGER_OK) {
        std::map<ledger::type::Result, std::string> errorDescriptions {
          { ledger::type::Result::LEDGER_ERROR, "The recovery failed" },
        };
        NSDictionary *userInfo = @{};
        const auto description = errorDescriptions[result];
        if (description.length() > 0) {
          userInfo = @{ NSLocalizedDescriptionKey: [NSString stringWithUTF8String:description.c_str()] };
        }
        error = [NSError errorWithDomain:BATBraveLedgerErrorDomain code:static_cast<NSInteger>(result) userInfo:userInfo];
      }
      if (completion) {
        completion(error);
      }
    }
  );
}

- (void)hasSufficientBalanceToReconcile:(void (^)(BOOL))completion
{
  ledger->HasSufficientBalanceToReconcile(completion);
}

- (void)pendingContributionsTotal:(void (^)(double amount))completion
{
  ledger->GetPendingContributionsTotal(^(double total){
    completion(total);
  });
}

- (void)linkBraveWalletToPaymentId:(NSString *)paymentId completion:(void (^)(BATResult result, NSString *drainID))completion
{
  ledger->LinkBraveWallet(paymentId.UTF8String, ^(ledger::type::Result result, std::string drain_id) {
    completion(static_cast<BATResult>(result), [NSString stringWithUTF8String:drain_id.c_str()]);
  });
}

- (void)drainStatusForDrainId:(NSString *)drainId completion:(void (^)(BATResult result, BATDrainStatus status))completion
{
  ledger->GetDrainStatus(drainId.UTF8String, ^(ledger::type::Result result, ledger::type::DrainStatus status) {
    completion(static_cast<BATResult>(result),
               static_cast<BATDrainStatus>(status));
  });
}

- (void)transferrableAmount:(void (^)(double amount))completion
{
  ledger->GetTransferableAmount(^(double amount) {
    completion(amount);
  });
}

#pragma mark - User Wallets

- (void)fetchUpholdWallet:
    (nullable void (^)(BATExternalWallet* _Nullable wallet))completion {
  const auto __weak weakSelf = self;
  ledger->GetExternalWallet(ledger::constant::kWalletUphold, ^(
                                ledger::type::Result result,
                                ledger::type::ExternalWalletPtr walletPtr) {
    if (result == ledger::type::Result::LEDGER_OK &&
        walletPtr.get() != nullptr) {
      const auto bridgedWallet =
          [[BATExternalWallet alloc] initWithExternalWallet:*walletPtr];
      weakSelf.upholdWallet = bridgedWallet;
      if (completion) {
        completion(bridgedWallet);
      }
    } else {
      if (completion) {
        completion(nil);
      }
    }
  });
}

- (void)disconnectWalletOfType:(BATWalletType)walletType
                    completion:(nullable void (^)(BATResult result))completion
{
  ledger->DisconnectWallet(walletType.UTF8String, ^(ledger::type::Result result){
    if (completion) {
      completion(static_cast<BATResult>(result));
    }

    for (BATBraveLedgerObserver *observer in self.observers) {
      if (observer.externalWalletDisconnected) {
        observer.externalWalletDisconnected(walletType);
      }
    }
  });
}

- (void)authorizeExternalWalletOfType:(BATWalletType)walletType
                           queryItems:(NSDictionary<NSString *, NSString *> *)queryItems
                           completion:(void (^)(BATResult result, NSURL * _Nullable redirectURL))completion
{
  ledger->ExternalWalletAuthorization(walletType.UTF8String,
                                      MapFromNSDictionary(queryItems),
                                      ^(ledger::type::Result result, base::flat_map<std::string, std::string> args) {
    const auto it = args.find("redirect_url");
    std::string redirect;
    if (it != args.end()) {
      redirect = it->second;
    }
    NSURL *url = redirect.empty() ? nil : [NSURL URLWithString:[NSString stringWithUTF8String:redirect.c_str()]];
    completion(static_cast<BATResult>(result), url);

    if (result == ledger::type::Result::LEDGER_OK) {
      for (BATBraveLedgerObserver *observer in self.observers) {
        if (observer.externalWalletAuthorized) {
          observer.externalWalletAuthorized(walletType);
        }
      }
    }
  });
}

- (std::string)getLegacyWallet
{
  NSDictionary *externalWallets = self.prefs[kExternalWalletsPrefKey] ?: [[NSDictionary alloc] init];
  std::string wallet;
  NSData *data = [NSJSONSerialization dataWithJSONObject:externalWallets options:0 error:nil];
  if (data != nil) {
    NSString *dataString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    if (dataString.UTF8String != nil) {
      wallet = dataString.UTF8String;
    }
  }
  return wallet;
}

#pragma mark - Publishers

- (void)listActivityInfoFromStart:(unsigned int)start
                            limit:(unsigned int)limit
                           filter:(BATActivityInfoFilter *)filter
                       completion:(void (NS_NOESCAPE ^)(NSArray<BATPublisherInfo *> *))completion
{
  auto cppFilter = filter ? filter.cppObjPtr : ledger::type::ActivityInfoFilter::New();
  if (filter.excluded == BATExcludeFilterFilterExcluded) {
    ledger->GetExcludedList(^(ledger::type::PublisherInfoList list) {
      const auto publishers = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::type::PublisherInfoPtr& info){
        return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
      });
      completion(publishers);
    });
  } else {
    ledger->GetActivityInfoList(start, limit, std::move(cppFilter), ^(ledger::type::PublisherInfoList list) {
      const auto publishers = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::type::PublisherInfoPtr& info){
        return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
      });
      completion(publishers);
    });
  }
}

- (void)fetchPublisherActivityFromURL:(NSURL *)URL
                           faviconURL:(nullable NSURL *)faviconURL
                        publisherBlob:(nullable NSString *)publisherBlob
                                tabId:(uint64_t)tabId
{
  if (!URL.absoluteString) {
    return;
  }

  GURL parsedUrl(base::SysNSStringToUTF8(URL.absoluteString));

  if (!parsedUrl.is_valid()) {
      return;
  }

  auto origin = parsedUrl.GetOrigin();
  std::string baseDomain =
  GetDomainAndRegistry(origin.host(), net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  if (baseDomain == "") {
    return;
  }

  ledger::type::VisitDataPtr visitData = ledger::type::VisitData::New();
  visitData->domain = visitData->name = baseDomain;
  visitData->path = parsedUrl.PathForRequest();
  visitData->url = origin.spec();

  if (faviconURL.absoluteString) {
    visitData->favicon_url = base::SysNSStringToUTF8(faviconURL.absoluteString);
  }

  std::string blob = std::string();
  if (publisherBlob) {
    blob = base::SysNSStringToUTF8(publisherBlob);
  }

  ledger->GetPublisherActivityFromUrl(tabId, std::move(visitData), blob);
}

- (void)updatePublisherExclusionState:(NSString *)publisherId state:(BATPublisherExclude)state
{
  ledger->SetPublisherExclude(std::string(publisherId.UTF8String), (ledger::type::PublisherExclude)state, ^(const ledger::type::Result result) {
    if (result != ledger::type::Result::LEDGER_OK) {
      return;
    }
    for (BATBraveLedgerObserver *observer in [self.observers copy]) {
      if (observer.excludedSitesChanged) {
        observer.excludedSitesChanged(publisherId,
                                      state);
      }
    }
  });
}

- (void)restoreAllExcludedPublishers
{
  ledger->RestorePublishers(^(const ledger::type::Result result) {
    if (result != ledger::type::Result::LEDGER_OK) {
      return;
    }

    for (BATBraveLedgerObserver *observer in [self.observers copy]) {
      if (observer.excludedSitesChanged) {
        observer.excludedSitesChanged(@"-1",
                                      static_cast<BATPublisherExclude>(ledger::type::PublisherExclude::ALL));
      }
    }
  });
}

- (void)publisherBannerForId:(NSString *)publisherId completion:(void (^)(BATPublisherBanner * _Nullable banner))completion
{
  ledger->GetPublisherBanner(std::string(publisherId.UTF8String), ^(ledger::type::PublisherBannerPtr banner) {
    auto bridgedBanner = banner.get() != nullptr ? [[BATPublisherBanner alloc] initWithPublisherBanner:*banner] : nil;
    // native libs prefixes the logo and background image with this URL scheme
    const auto imagePrefix = @"chrome://rewards-image/";
    bridgedBanner.background = [bridgedBanner.background stringByReplacingOccurrencesOfString:imagePrefix withString:@""];
    bridgedBanner.logo = [bridgedBanner.logo stringByReplacingOccurrencesOfString:imagePrefix withString:@""];
    completion(bridgedBanner);
  });
}

- (void)refreshPublisherWithId:(NSString *)publisherId completion:(void (^)(BATPublisherStatus status))completion
{
  if (self.loadingPublisherList) {
    completion(BATPublisherStatusNotVerified);
    return;
  }
  ledger->RefreshPublisher(std::string(publisherId.UTF8String), ^(ledger::type::PublisherStatus status) {
    completion(static_cast<BATPublisherStatus>(status));
  });
}

#pragma mark - SKUs

- (void)processSKUItems:(NSArray<BATSKUOrderItem *> *)items
             completion:(void (^)(BATResult result, NSString *orderID))completion
{
  ledger->ProcessSKU(VectorFromNSArray(items, ^ledger::type::SKUOrderItem(BATSKUOrderItem *item) {
    return *item.cppObjPtr;
  }), ledger::constant::kWalletUnBlinded, ^(const ledger::type::Result result, const std::string& order_id) {
    completion(static_cast<BATResult>(result), [NSString stringWithUTF8String:order_id.c_str()]);
  });
}

#pragma mark - Tips

- (void)listRecurringTips:(void (^)(NSArray<BATPublisherInfo *> *))completion
{
  ledger->GetRecurringTips(^(ledger::type::PublisherInfoList list){
    const auto publishers = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::type::PublisherInfoPtr& info){
      return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
    });
    completion(publishers);
  });
}

- (void)addRecurringTipToPublisherWithId:(NSString *)publisherId amount:(double)amount completion:(void (^)(BOOL success))completion
{
  ledger::type::RecurringTipPtr info = ledger::type::RecurringTip::New();
  info->publisher_key = publisherId.UTF8String;
  info->amount = amount;
  info->created_at = [[NSDate date] timeIntervalSince1970];
  ledger->SaveRecurringTip(std::move(info), ^(ledger::type::Result result){
    const auto success = (result == ledger::type::Result::LEDGER_OK);
    if (success) {
      for (BATBraveLedgerObserver *observer in [self.observers copy]) {
        if (observer.recurringTipAdded) {
          observer.recurringTipAdded(publisherId);
        }
      }
    }
    completion(success);
  });
}

- (void)removeRecurringTipForPublisherWithId:(NSString *)publisherId
{
  ledger->RemoveRecurringTip(std::string(publisherId.UTF8String), ^(ledger::type::Result result){
    if (result == ledger::type::Result::LEDGER_OK) {
      for (BATBraveLedgerObserver *observer in [self.observers copy]) {
        if (observer.recurringTipRemoved) {
          observer.recurringTipRemoved(publisherId);
        }
      }
    }
  });
}

- (void)listOneTimeTips:(void (^)(NSArray<BATPublisherInfo *> *))completion
{
  ledger->GetOneTimeTips(^(ledger::type::PublisherInfoList list){
    const auto publishers = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::type::PublisherInfoPtr& info){
      return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
    });
    completion(publishers);
  });
}

- (void)tipPublisherDirectly:(BATPublisherInfo *)publisher amount:(double)amount currency:(NSString *)currency completion:(void (^)(BATResult result))completion
{
  ledger->OneTimeTip(std::string(publisher.id.UTF8String), amount, ^(ledger::type::Result result) {
    completion(static_cast<BATResult>(result));
  });
}

#pragma mark - Grants

- (NSArray<BATPromotion *> *)pendingPromotions
{
  return [self.mPendingPromotions copy];
}

- (NSArray<BATPromotion *> *)finishedPromotions
{
  return [self.mFinishedPromotions copy];
}

- (NSString *)notificationIDForPromo:(const ledger::type::PromotionPtr)promo
{
  bool isUGP = promo->type == ledger::type::PromotionType::UGP;
  const auto prefix = isUGP ? @"rewards_grant_" : @"rewards_grant_ads_";
  const auto promotionId = [NSString stringWithUTF8String:promo->id.c_str()];
  return [NSString stringWithFormat:@"%@%@", prefix, promotionId];
}

- (void)updatePendingAndFinishedPromotions:(void (^)())completion
{
  ledger->GetAllPromotions(^(ledger::type::PromotionMap map) {
    NSMutableArray *promos = [[NSMutableArray alloc] init];
    for (auto it = map.begin(); it != map.end(); ++it) {
      if (it->second.get() != nullptr) {
        [promos addObject:[[BATPromotion alloc] initWithPromotion:*it->second]];
      }
    }
    for (BATPromotion *promo in [self.mPendingPromotions copy]) {
      [self clearNotificationWithID:[self notificationIDForPromo:promo.cppObjPtr]];
    }
    [self.mFinishedPromotions removeAllObjects];
    [self.mPendingPromotions removeAllObjects];
    for (BATPromotion *promotion in promos) {
      if (promotion.status == BATPromotionStatusFinished) {
        [self.mFinishedPromotions addObject:promotion];

        if (promotion.type == BATPromotionTypeAds) {
          [self.ads reconcileAdRewards];
        }
      } else if (promotion.status == BATPromotionStatusActive ||
                 promotion.status == BATPromotionStatusAttested) {
        [self.mPendingPromotions addObject:promotion];
        bool isUGP = promotion.type == BATPromotionTypeUgp;
        auto notificationKind = isUGP ? BATRewardsNotificationKindGrant : BATRewardsNotificationKindGrantAds;

        [self addNotificationOfKind:notificationKind
                           userInfo:nil
                     notificationID:[self notificationIDForPromo:promotion.cppObjPtr]
                           onlyOnce:YES];
      }
    }
    if (completion) {
      completion();
    }
    for (BATBraveLedgerObserver *observer in [self.observers copy]) {
      if (observer.promotionsAdded) {
        observer.promotionsAdded(self.pendingPromotions);
      }
      if (observer.finishedPromotionsAdded) {
        observer.finishedPromotionsAdded(self.finishedPromotions);
      }
    }
  });
}

- (void)fetchPromotions:(nullable void (^)(NSArray<BATPromotion *> *grants))completion
{
  ledger->FetchPromotions(^(ledger::type::Result result, std::vector<ledger::type::PromotionPtr> promotions) {
    if (result != ledger::type::Result::LEDGER_OK) {
      return;
    }
    [self updatePendingAndFinishedPromotions:^{
      if (completion) {
        completion(self.pendingPromotions);
      }
    }];
  });
}

- (void)claimPromotion:(NSString *)promotionId publicKey:(NSString *)deviceCheckPublicKey completion:(void (^)(BATResult result, NSString * _Nonnull nonce))completion
{
  const auto payload = [NSDictionary dictionaryWithObject:deviceCheckPublicKey forKey:@"publicKey"];
  const auto jsonData = [NSJSONSerialization dataWithJSONObject:payload options:0 error:nil];
  if (!jsonData) {
    BLOG(0, @"Missing JSON payload while attempting to claim promotion");
    return;
  }
  const auto jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  ledger->ClaimPromotion(promotionId.UTF8String, jsonString.UTF8String, ^(const ledger::type::Result result, const std::string& nonce) {
    const auto bridgedNonce = [NSString stringWithUTF8String:nonce.c_str()];
    dispatch_async(dispatch_get_main_queue(), ^{
      completion(static_cast<BATResult>(result), bridgedNonce);
    });
  });
}

- (void)attestPromotion:(NSString *)promotionId solution:(BATPromotionSolution *)solution completion:(void (^)(BATResult result, BATPromotion * _Nullable promotion))completion
{
  ledger->AttestPromotion(std::string(promotionId.UTF8String), solution.JSONPayload.UTF8String, ^(const ledger::type::Result result, ledger::type::PromotionPtr promotion) {
    if (promotion.get() == nullptr) {
      if (completion) {
        dispatch_async(dispatch_get_main_queue(), ^{
          completion(static_cast<BATResult>(result), nil);
        });
      }
      return;
    }

    const auto bridgedPromotion = [[BATPromotion alloc] initWithPromotion:*promotion];
    if (result == ledger::type::Result::LEDGER_OK) {
      [self fetchBalance:nil];
      [self clearNotificationWithID:[self notificationIDForPromo:std::move(promotion)]];
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      if (completion) {
        completion(static_cast<BATResult>(result), bridgedPromotion);
      }
      if (result == ledger::type::Result::LEDGER_OK) {
        for (BATBraveLedgerObserver *observer in [self.observers copy]) {
          if (observer.promotionClaimed) {
            observer.promotionClaimed(bridgedPromotion);
          }
        }
      }
    });
  });
}

#pragma mark - History

- (void)balanceReportForMonth:(BATActivityMonth)month year:(int)year completion:(void (^)(BATBalanceReportInfo * _Nullable info))completion
{
  ledger->GetBalanceReport((ledger::type::ActivityMonth)month, year, ^(const ledger::type::Result result, ledger::type::BalanceReportInfoPtr info) {
    auto bridgedInfo = info.get() != nullptr ? [[BATBalanceReportInfo alloc] initWithBalanceReportInfo:*info.get()] : nil;
    completion(result == ledger::type::Result::LEDGER_OK ? bridgedInfo : nil);
  });
}

- (BATAutoContributeProperties *)autoContributeProperties
{
  ledger::type::AutoContributePropertiesPtr props = ledger->GetAutoContributeProperties();
  return [[BATAutoContributeProperties alloc] initWithAutoContributePropertiesPtr:std::move(props)];
}

#pragma mark - Pending Contributions

- (void)pendingContributions:(void (^)(NSArray<BATPendingContributionInfo *> *publishers))completion
{
  ledger->GetPendingContributions(^(ledger::type::PendingContributionInfoList list){
    const auto convetedList = NSArrayFromVector(&list, ^BATPendingContributionInfo *(const ledger::type::PendingContributionInfoPtr& info){
      return [[BATPendingContributionInfo alloc] initWithPendingContributionInfo:*info];
    });
    completion(convetedList);
  });
}

- (void)removePendingContribution:(BATPendingContributionInfo *)info completion:(void (^)(BATResult result))completion
{
  ledger->RemovePendingContribution(info.id,
                                    ^(const ledger::type::Result result){
    completion(static_cast<BATResult>(result));
  });
}

- (void)removeAllPendingContributions:(void (^)(BATResult result))completion
{
  ledger->RemoveAllPendingContributions(^(const ledger::type::Result result){
    completion(static_cast<BATResult>(result));
  });
}

#pragma mark - Reconcile

- (void)onReconcileComplete:(ledger::type::Result)result contribution:(ledger::type::ContributionInfoPtr)contribution
{
  // TODO we changed from probi to amount, so from string to double
  if (result == ledger::type::Result::LEDGER_OK) {
    if (contribution->type == ledger::type::RewardsType::RECURRING_TIP) {
      [self showTipsProcessedNotificationIfNeccessary];
    }
    [self fetchBalance:nil];
  }

  if ((result == ledger::type::Result::LEDGER_OK && contribution->type == ledger::type::RewardsType::AUTO_CONTRIBUTE) ||
      result == ledger::type::Result::LEDGER_ERROR ||
      result == ledger::type::Result::NOT_ENOUGH_FUNDS ||
      result == ledger::type::Result::TIP_ERROR) {
    const auto contributionId = [NSString stringWithUTF8String:contribution->contribution_id.c_str()];
    const auto info = @{ @"viewingId": contributionId,
                         @"result": @((BATResult)result),
                         @"type": @((BATRewardsType)contribution->type),
                         @"amount": [@(contribution->amount) stringValue] };

    [self addNotificationOfKind:BATRewardsNotificationKindAutoContribute
                       userInfo:info
                 notificationID:[NSString stringWithFormat:@"contribution_%@", contributionId]];
  }

  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.balanceReportUpdated) {
      observer.balanceReportUpdated();
    }
    if (observer.reconcileCompleted) {
      observer.reconcileCompleted(static_cast<BATResult>(result),
                                  [NSString stringWithUTF8String:contribution->contribution_id.c_str()],
                                  static_cast<BATRewardsType>(contribution->type),
                                  [@(contribution->amount) stringValue]);
    }
  }
}

#pragma mark - Misc

+ (bool)isMediaURL:(NSURL *)url firstPartyURL:(NSURL *)firstPartyURL referrerURL:(NSURL *)referrerURL
{
  std::string referrer = referrerURL != nil ? referrerURL.absoluteString.UTF8String : "";
  return ledger::Ledger::IsMediaLink(url.absoluteString.UTF8String,
                                     firstPartyURL.absoluteString.UTF8String,
                                     referrer);
}

- (NSString *)encodedURI:(NSString *)uri
{
  const auto encoded = ledger->URIEncode(std::string(uri.UTF8String));
  return [NSString stringWithUTF8String:encoded.c_str()];
}

- (void)rewardsInternalInfo:(void (NS_NOESCAPE ^)(BATRewardsInternalsInfo * _Nullable info))completion
{
  ledger->GetRewardsInternalsInfo(^(ledger::type::RewardsInternalsInfoPtr info) {
    auto bridgedInfo = info.get() != nullptr ? [[BATRewardsInternalsInfo alloc] initWithRewardsInternalsInfo:*info.get()] : nil;
    completion(bridgedInfo);
  });
}

- (void)allContributions:(void (^)(NSArray<BATContributionInfo *> *contributions))completion
{
  ledger->GetAllContributions(^(ledger::type::ContributionInfoList list) {
    const auto convetedList = NSArrayFromVector(&list, ^BATContributionInfo *(const ledger::type::ContributionInfoPtr& info){
      return [[BATContributionInfo alloc] initWithContributionInfo:*info];
    });
    completion(convetedList);
  });
}

#pragma mark - Reporting

- (void)setSelectedTabId:(UInt32)selectedTabId
{
  if (!self.initialized) { return; }

  if (_selectedTabId != selectedTabId) {
    ledger->OnHide(_selectedTabId, [[NSDate date] timeIntervalSince1970]);
  }
  _selectedTabId = selectedTabId;
  if (_selectedTabId > 0) {
    ledger->OnShow(_selectedTabId, [[NSDate date] timeIntervalSince1970]);
  }
}

- (void)applicationDidBecomeActive
{
  if (!self.initialized) { return; }

  ledger->OnForeground(self.selectedTabId, [[NSDate date] timeIntervalSince1970]);

  // Check if the last notification check was more than a day ago
  if (fabs([self.lastNotificationCheckDate timeIntervalSinceNow]) > kOneDay) {
    [self checkForNotificationsAndFetchGrants];
  }
}

- (void)applicationDidBackground
{
  if (!self.initialized) { return; }

  ledger->OnBackground(self.selectedTabId, [[NSDate date] timeIntervalSince1970]);
}

- (void)reportLoadedPageWithURL:(NSURL *)url tabId:(UInt32)tabId
{
  if (!self.initialized) { return; }

  GURL parsedUrl(url.absoluteString.UTF8String);
  auto origin = parsedUrl.GetOrigin();
  const std::string baseDomain =
  GetDomainAndRegistry(origin.host(), net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  if (baseDomain == "") {
    return;
  }

  const std::string publisher_url = origin.scheme() + "://" + baseDomain + "/";

  ledger::type::VisitDataPtr data = ledger::type::VisitData::New();
  data->tld = data->name = baseDomain;
  data->domain = origin.host();
  data->path = parsedUrl.path();
  data->tab_id = tabId;
  data->url = publisher_url;

  ledger->OnLoad(std::move(data), [[NSDate date] timeIntervalSince1970]);
}

- (void)reportXHRLoad:(NSURL *)url tabId:(UInt32)tabId firstPartyURL:(NSURL *)firstPartyURL referrerURL:(NSURL *)referrerURL
{
  if (!self.initialized) { return; }

  base::flat_map<std::string, std::string> partsMap;
  const auto urlComponents = [[NSURLComponents alloc] initWithURL:url resolvingAgainstBaseURL:NO];
  for (NSURLQueryItem *item in urlComponents.queryItems) {
    std::string value = item.value != nil ? item.value.UTF8String : "";
    partsMap[std::string(item.name.UTF8String)] = value;
  }

  auto visit = ledger::type::VisitData::New();
  visit->path = url.absoluteString.UTF8String;
  visit->tab_id = tabId;

  std::string ref = referrerURL != nil ? referrerURL.absoluteString.UTF8String : "";
  std::string fpu = firstPartyURL != nil ? firstPartyURL.absoluteString.UTF8String : "";

  ledger->OnXHRLoad(tabId,
                    url.absoluteString.UTF8String,
                    partsMap,
                    fpu,
                    ref,
                    std::move(visit));
}

- (void)reportPostData:(NSData *)postData url:(NSURL *)url tabId:(UInt32)tabId firstPartyURL:(NSURL *)firstPartyURL referrerURL:(NSURL *)referrerURL
{
  if (!self.initialized) { return; }

  GURL parsedUrl(url.absoluteString.UTF8String);
  if (!parsedUrl.is_valid()) {
    return;
  }

  const auto postDataString = [[[NSString alloc] initWithData:postData encoding:NSUTF8StringEncoding] stringByRemovingPercentEncoding];

  auto visit = ledger::type::VisitData::New();
  visit->path = parsedUrl.spec();
  visit->tab_id = tabId;

  std::string ref = referrerURL != nil ? referrerURL.absoluteString.UTF8String : "";
  std::string fpu = firstPartyURL != nil ? firstPartyURL.absoluteString.UTF8String : "";

  ledger->OnPostData(parsedUrl.spec(),
                     fpu,
                     ref,
                     postDataString.UTF8String,
                     std::move(visit));
}

- (void)reportTabNavigationOrClosedWithTabId:(UInt32)tabId
{
  if (!self.initialized) { return; }

  ledger->OnUnload(tabId, [[NSDate date] timeIntervalSince1970]);
}

#pragma mark - Preferences

BATLedgerBridge(int,
                minimumVisitDuration, setMinimumVisitDuration,
                GetPublisherMinVisitTime, SetPublisherMinVisitTime)

BATLedgerBridge(int,
                minimumNumberOfVisits, setMinimumNumberOfVisits,
                GetPublisherMinVisits, SetPublisherMinVisits)

BATLedgerBridge(BOOL,
                allowUnverifiedPublishers, setAllowUnverifiedPublishers,
                GetPublisherAllowNonVerified, SetPublisherAllowNonVerified)

BATLedgerBridge(BOOL,
                allowVideoContributions, setAllowVideoContributions,
                GetPublisherAllowVideos, SetPublisherAllowVideos)

BATLedgerReadonlyBridge(double, contributionAmount, GetAutoContributionAmount)

- (void)setContributionAmount:(double)contributionAmount
{
  ledger->SetAutoContributionAmount(contributionAmount);
}

BATLedgerBridge(BOOL,
                isAutoContributeEnabled, setAutoContributeEnabled,
                GetAutoContributeEnabled, SetAutoContributeEnabled)

- (void)setBooleanState:(const std::string&)name value:(bool)value
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  self.prefs[key] = [NSNumber numberWithBool:value];
  [self savePrefs];
}

- (bool)getBooleanState:(const std::string&)name
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  if (![self.prefs objectForKey:key]) {
    return NO;
  }

  return [self.prefs[key] boolValue];
}

- (void)setIntegerState:(const std::string&)name value:(int)value
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  self.prefs[key] = [NSNumber numberWithInt:value];
  [self savePrefs];
}

- (int)getIntegerState:(const std::string&)name
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  return [self.prefs[key] intValue];
}

- (void)setDoubleState:(const std::string&)name value:(double)value
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  self.prefs[key] = [NSNumber numberWithDouble:value];
  [self savePrefs];
}

- (double)getDoubleState:(const std::string&)name
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  return [self.prefs[key] doubleValue];
}

- (void)setStringState:(const std::string&)name value:(const std::string&)value
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  self.prefs[key] = [NSString stringWithUTF8String:value.c_str()];
  [self savePrefs];
}

- (std::string)getStringState:(const std::string&)name
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  const auto value = (NSString *)self.prefs[key];
  if (!value) { return ""; }
  return value.UTF8String;
}

- (void)setInt64State:(const std::string&)name value:(int64_t)value
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  self.prefs[key] = [NSNumber numberWithLongLong:value];
  [self savePrefs];
}

- (int64_t)getInt64State:(const std::string&)name
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  return [self.prefs[key] longLongValue];
}

- (void)setUint64State:(const std::string&)name value:(uint64_t)value
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  self.prefs[key] = [NSNumber numberWithUnsignedLongLong:value];
  [self savePrefs];
}

- (uint64_t)getUint64State:(const std::string&)name
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  return [self.prefs[key] unsignedLongLongValue];
}

- (void)clearState:(const std::string&)name
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  [self.prefs removeObjectForKey:key];
  [self savePrefs];
}

- (bool)getBooleanOption:(const std::string&)name
{
  DCHECK(!name.empty());

  const auto it = kBoolOptions.find(name);
  DCHECK(it != kBoolOptions.end());

  return kBoolOptions.at(name);
}

- (int)getIntegerOption:(const std::string&)name
{
  DCHECK(!name.empty());

  const auto it = kIntegerOptions.find(name);
  DCHECK(it != kIntegerOptions.end());

  return kIntegerOptions.at(name);
}

- (double)getDoubleOption:(const std::string&)name
{
  DCHECK(!name.empty());

  const auto it = kDoubleOptions.find(name);
  DCHECK(it != kDoubleOptions.end());

  return kDoubleOptions.at(name);
}

- (std::string)getStringOption:(const std::string&)name
{
  DCHECK(!name.empty());

  const auto it = kStringOptions.find(name);
  DCHECK(it != kStringOptions.end());

  return kStringOptions.at(name);
}

- (int64_t)getInt64Option:(const std::string&)name
{
  DCHECK(!name.empty());

  const auto it = kInt64Options.find(name);
  DCHECK(it != kInt64Options.end());

  return kInt64Options.at(name);
}

- (uint64_t)getUint64Option:(const std::string&)name
{
  DCHECK(!name.empty());

  const auto it = kUInt64Options.find(name);
  DCHECK(it != kUInt64Options.end());

  return kUInt64Options.at(name);
}

#pragma mark - Notifications

- (NSArray<BATRewardsNotification *> *)notifications
{
  return [self.mNotifications copy];
}

- (void)clearNotificationWithID:(NSString *)notificationID
{
  for (BATRewardsNotification *n in self.notifications) {
    if ([n.id isEqualToString:notificationID]) {
      [self clearNotification:n];
      return;
    }
  }
}

- (void)clearNotification:(BATRewardsNotification *)notification
{
  [self.mNotifications removeObject:notification];
  [self writeNotificationsToDisk];

  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.notificationsRemoved) {
      observer.notificationsRemoved(@[notification]);
    }
  }
}

- (void)clearAllNotifications
{
  NSArray *notifications = [self.mNotifications copy];
  [self.mNotifications removeAllObjects];
  [self writeNotificationsToDisk];

  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.notificationsRemoved) {
      observer.notificationsRemoved(notifications);
    }
  }
}

- (void)startNotificationTimers
{
  dispatch_async(dispatch_get_main_queue(), ^{
    // Startup timer, begins after 30-second delay.
    self.notificationStartupTimer =
    [NSTimer scheduledTimerWithTimeInterval:30
                                     target:self
                                   selector:@selector(checkForNotificationsAndFetchGrants)
                                   userInfo:nil
                                    repeats:NO];
  });

}

- (void)checkForNotificationsAndFetchGrants
{
  self.lastNotificationCheckDate = [NSDate date];

  [self showBackupNotificationIfNeccessary];
  [self showAddFundsNotificationIfNeccessary];
  [self fetchPromotions:nil];
}

- (void)showBackupNotificationIfNeccessary
{
  // This is currently not required as the user cannot manage their wallet on mobile... yet
  /*
  auto bootstamp = ledger->GetCreationStamp();
  auto userFunded = [self.prefs[kUserHasFundedKey] boolValue];
  auto backupSucceeded = [self.prefs[kBackupSucceededKey] boolValue];
  if (userFunded && !backupSucceeded) {
    auto frequency = 10; [self.prefs[kBackupNotificationFrequencyKey] doubleValue];
    auto interval = 10; [self.prefs[kBackupNotificationIntervalKey] doubleValue];
    auto delta = [[NSDate date] timeIntervalSinceDate:[NSDate dateWithTimeIntervalSince1970:bootstamp]];
    if (delta > interval) {
      auto nextBackupNotificationInterval = frequency + interval;
      self.prefs[kBackupNotificationIntervalKey] = @(nextBackupNotificationInterval);
      [self savePrefs];
      [self addNotificationOfKind:BATRewardsNotificationKindBackupWallet
                        arguments:nil
                   notificationID:@"rewards_notification_backup_wallet"];
    }
  }
  */
}

- (void)showAddFundsNotificationIfNeccessary
{
  const auto stamp = ledger->GetReconcileStamp();
  const auto now = [[NSDate date] timeIntervalSince1970];

  // Show add funds notification if reconciliation will occur in the
  // next 3 days and balance is too low.
  if (stamp - now > 3 * kOneDay) {
    return;
  }
  // Make sure it hasnt already been shown
  const auto upcomingAddFundsNotificationTime = [self.prefs[kNextAddFundsDateNotificationKey] doubleValue];
  if (upcomingAddFundsNotificationTime != 0.0 &&
      now < upcomingAddFundsNotificationTime) {
    return;
  }

  const auto __weak weakSelf = self;
  // Make sure they don't have a sufficient balance
  [self hasSufficientBalanceToReconcile:^(BOOL sufficient) {
    if (sufficient) {
      return;
    }
    const auto strongSelf = weakSelf;

    // Set next add funds notification in 3 days
    const auto nextTime = [[NSDate date] timeIntervalSince1970] + (kOneDay * 3);
    strongSelf.prefs[kNextAddFundsDateNotificationKey] = @(nextTime);
    [strongSelf savePrefs];

    [strongSelf addNotificationOfKind:BATRewardsNotificationKindInsufficientFunds
                             userInfo:nil
                       notificationID:@"rewards_notification_insufficient_funds"];
  }];
}

- (void)showTipsProcessedNotificationIfNeccessary
{
  if (!self.autoContributeEnabled) {
    return;
  }
  [self addNotificationOfKind:BATRewardsNotificationKindTipsProcessed
                     userInfo:nil
               notificationID:@"rewards_notification_tips_processed"];
}

- (void)addNotificationOfKind:(BATRewardsNotificationKind)kind
                     userInfo:(nullable NSDictionary *)userInfo
               notificationID:(nullable NSString *)identifier
{
  [self addNotificationOfKind:kind userInfo:userInfo notificationID:identifier onlyOnce:NO];
}

- (void)addNotificationOfKind:(BATRewardsNotificationKind)kind
                     userInfo:(nullable NSDictionary *)userInfo
               notificationID:(nullable NSString *)identifier
                     onlyOnce:(BOOL)onlyOnce
{
  NSParameterAssert(kind != BATRewardsNotificationKindInvalid);
  NSString *notificationID = [identifier copy];
  if (!identifier || identifier.length == 0) {
    notificationID = [NSUUID UUID].UUIDString;
  } else if (onlyOnce) {
    const auto idx = [self.mNotifications indexOfObjectPassingTest:^BOOL(BATRewardsNotification * _Nonnull obj, NSUInteger idx, BOOL * _Nonnull stop) {
      return obj.displayed && [obj.id isEqualToString:identifier];
    }];
    if (idx != NSNotFound) {
      return;
    }
  }

  const auto notification = [[BATRewardsNotification alloc] initWithID:notificationID
                                                             dateAdded:[[NSDate date] timeIntervalSince1970]
                                                                  kind:kind
                                                              userInfo:userInfo];
  if (onlyOnce) {
    notification.displayed = YES;
  }

  [self.mNotifications addObject:notification];

  // Post to observers
  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.notificationAdded) {
      observer.notificationAdded(notification);
    }
  }

  [NSNotificationCenter.defaultCenter postNotificationName:BATBraveLedgerNotificationAdded object:nil];

  [self writeNotificationsToDisk];
}

- (void)readNotificationsFromDisk
{
  const auto path = [self.storagePath stringByAppendingPathComponent:@"notifications"];
  const auto data = [NSData dataWithContentsOfFile:path];
  if (!data) {
    // Nothing to read
    self.mNotifications = [[NSMutableArray alloc] init];
    return;
  }

  NSError *error;
  self.mNotifications = [NSKeyedUnarchiver unarchivedObjectOfClass:NSArray.self fromData:data error:&error];
  if (!self.mNotifications) {
    self.mNotifications = [[NSMutableArray alloc] init];
    if (error) {
      BLOG(0, @"Failed to unarchive notifications on disk: %@",  error.debugDescription);
    }
  }
}

- (void)writeNotificationsToDisk
{
  const auto path = [self.storagePath stringByAppendingPathComponent:@"notifications"];
  if (self.notifications.count == 0) {
    // Nothing to write, delete anything we have stored
    if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
      [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
    }
    return;
  }

  NSError *error;
  const auto data = [NSKeyedArchiver archivedDataWithRootObject:self.notifications
                                          requiringSecureCoding:YES
                                                          error:&error];
  if (!data) {
    if (error) {
      BLOG(0, @"Failed to write notifications to disk: %@",  error.debugDescription);
    }
    return;
  }

  [data writeToURL:[NSURL fileURLWithPath:path isDirectory:NO] options:NSDataWritingAtomic error:nil];
}

#pragma mark - State

- (void)loadLedgerState:(ledger::client::OnLoadCallback)callback
{
  const auto contents = [self.commonOps loadContentsFromFileWithName:"ledger_state.json"];
  if (contents.length() > 0) {
    callback(ledger::type::Result::LEDGER_OK, contents);
  } else {
    callback(ledger::type::Result::NO_LEDGER_STATE, contents);
  }
  [self startNotificationTimers];
}

- (void)loadPublisherState:(ledger::client::OnLoadCallback)callback
{
  const auto contents = [self.commonOps loadContentsFromFileWithName:"publisher_state.json"];
  if (contents.length() > 0) {
    callback(ledger::type::Result::LEDGER_OK, contents);
  } else {
    callback(ledger::type::Result::NO_PUBLISHER_STATE, contents);
  }
}

- (void)loadState:(const std::string &)name callback:(ledger::client::OnLoadCallback)callback
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  const auto value = self.state[key];
  if (value) {
    callback(ledger::type::Result::LEDGER_OK, std::string(value.UTF8String));
  } else {
    callback(ledger::type::Result::LEDGER_ERROR, "");
  }
}

- (void)resetState:(const std::string &)name callback:(ledger::ResultCallback)callback
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  self.state[key] = nil;
  callback(ledger::type::Result::LEDGER_OK);
  // In brave-core, failed callback returns `LEDGER_ERROR`
  NSDictionary *state = [self.state copy];
  NSString *path = [self.randomStatePath copy];
  dispatch_async(self.fileWriteThread, ^{
    [state writeToURL:[NSURL fileURLWithPath:path isDirectory:NO] error:nil];
  });
}

- (void)saveState:(const std::string &)name value:(const std::string &)value callback:(ledger::ResultCallback)callback
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  self.state[key] = [NSString stringWithUTF8String:value.c_str()];
  callback(ledger::type::Result::LEDGER_OK);
  // In brave-core, failed callback returns `LEDGER_ERROR`
  NSDictionary *state = [self.state copy];
  NSString *path = [self.randomStatePath copy];
  dispatch_async(self.fileWriteThread, ^{
    [state writeToURL:[NSURL fileURLWithPath:path isDirectory:NO] error:nil];
  });
}

#pragma mark - Network

- (NSString *)customUserAgent
{
  return self.commonOps.customUserAgent;
}

- (void)setCustomUserAgent:(NSString *)customUserAgent
{
  self.commonOps.customUserAgent = [customUserAgent stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
}

- (void)loadURL:(ledger::type::UrlRequestPtr)request callback:(ledger::client::LoadURLCallback)callback
{
  std::map<ledger::type::UrlMethod, std::string> methodMap {
    {ledger::type::UrlMethod::GET, "GET"},
    {ledger::type::UrlMethod::POST, "POST"},
    {ledger::type::UrlMethod::PUT, "PUT"}
  };

  if (!request) {
    request = ledger::type::UrlRequest::New();
  }

  const auto copiedURL = [NSString stringWithUTF8String:request->url.c_str()];

  return [self.commonOps loadURLRequest:request->url headers:request->headers content:request->content content_type:request->content_type method:methodMap[request->method] callback:^(const std::string& errorDescription, int statusCode, const std::string &response, const base::flat_map<std::string, std::string> &headers) {
    ledger::type::UrlResponse url_response;
    url_response.url = copiedURL.UTF8String;
    url_response.error = errorDescription;
    url_response.status_code = statusCode;
    url_response.body = response;
    url_response.headers = headers;

    callback(url_response);
  }];
}

- (std::string)URIEncode:(const std::string &)value
{
  const auto allowedCharacters = [NSMutableCharacterSet alphanumericCharacterSet];
  [allowedCharacters addCharactersInString:@"-._~"];
  const auto string = [NSString stringWithUTF8String:value.c_str()];
  const auto encoded = [string stringByAddingPercentEncodingWithAllowedCharacters:allowedCharacters];
  return std::string(encoded.UTF8String);
}

- (void)fetchFavIcon:(const std::string &)url faviconKey:(const std::string &)favicon_key callback:(ledger::client::FetchIconCallback)callback
{
  const auto pageURL = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
  if (!self.faviconFetcher || !pageURL) {
    dispatch_async(dispatch_get_main_queue(), ^{
      callback(NO, std::string());
    });
    return;
  }
  self.faviconFetcher(pageURL, ^(NSURL * _Nullable faviconURL) {
    dispatch_async(dispatch_get_main_queue(), ^{
      callback(faviconURL != nil,
               faviconURL.absoluteString.UTF8String);
    });
  });
}

#pragma mark - Logging

- (void)log:(const char *)file line:(const int)line verboseLevel:(const int)verbose_level message:(const std::string &) message
{
  rewards::LogMessage(file, line, verbose_level, [NSString stringWithUTF8String:message.c_str()]);
}

#pragma mark - Publisher Database

- (void)handlePublisherListing:(NSArray<BATPublisherInfo *> *)publishers start:(uint32_t)start limit:(uint32_t)limit callback:(ledger::PublisherInfoListCallback)callback
{
  callback(VectorFromNSArray(publishers, ^ledger::type::PublisherInfoPtr(BATPublisherInfo *info){
    return info.cppObjPtr;
  }));
}
- (void)publisherListNormalized:(ledger::type::PublisherInfoList)list
{
  const auto list_converted = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::type::PublisherInfoPtr& info) {
    return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
  });

  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.publisherListNormalized) {
      observer.publisherListNormalized(list_converted);
    }
  }
}

- (void)onPanelPublisherInfo:(ledger::type::Result)result publisherInfo:(ledger::type::PublisherInfoPtr)publisher_info windowId:(uint64_t)windowId
{
  if (publisher_info.get() == nullptr || result != ledger::type::Result::LEDGER_OK) {
    return;
  }
  auto info = [[BATPublisherInfo alloc] initWithPublisherInfo:*publisher_info];
  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.fetchedPanelPublisher) {
      observer.fetchedPanelPublisher(info, windowId);
    }
  }
}

- (void)onContributeUnverifiedPublishers:(ledger::type::Result)result publisherKey:(const std::string &)publisher_key publisherName:(const std::string &)publisher_name
{
  switch (result) {
    case ledger::type::Result::PENDING_NOT_ENOUGH_FUNDS:
      [self addNotificationOfKind:BATRewardsNotificationKindPendingNotEnoughFunds
                         userInfo:nil
                   notificationID:@"not_enough_funds_for_pending"];
      break;
    case ledger::type::Result::PENDING_PUBLISHER_REMOVED: {
      const auto publisherID = [NSString stringWithUTF8String:publisher_key.c_str()];
      for (BATBraveLedgerObserver *observer in [self.observers copy]) {
        if (observer.pendingContributionsRemoved) {
          observer.pendingContributionsRemoved(@[publisherID]);
        }
      }
      break;
    }
    case ledger::type::Result::VERIFIED_PUBLISHER: {
      const auto notificationID = [NSString stringWithFormat:@"verified_publisher_%@",
                                   [NSString stringWithUTF8String:publisher_key.c_str()]];
      const auto name = [NSString stringWithUTF8String:publisher_name.c_str()];
      [self addNotificationOfKind:BATRewardsNotificationKindVerifiedPublisher
                         userInfo:@{ @"publisher_name": name }
                   notificationID:notificationID];
      break;
    }
    default:
      break;
  }
}

- (void)showNotification:(const std::string &)type args:(const std::vector<std::string>&)args callback:(ledger::ResultCallback)callback
{
  const auto notificationID = [NSString stringWithUTF8String:type.c_str()];
  const auto info = [[NSMutableDictionary<NSNumber *, NSString *> alloc] init];
  for (NSInteger i = 0; i < args.size(); i++) {
    info[@(i)] = [NSString stringWithUTF8String:args[i].c_str()];
  }
  [self addNotificationOfKind:BATRewardsNotificationKindGeneralLedger
                     userInfo:info
               notificationID:notificationID
                     onlyOnce:NO];
}
- (ledger::type::ClientInfoPtr)getClientInfo
{
  auto info = ledger::type::ClientInfo::New();
  info->os = ledger::type::OperatingSystem::UNDEFINED;
  info->platform = ledger::type::Platform::IOS;
  return info;
}

- (void)unblindedTokensReady
{
  [self fetchBalance:nil];
  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.balanceReportUpdated) {
      observer.balanceReportUpdated();
    }
  }
}

- (void)reconcileStampReset
{
  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.reconcileStampReset) {
      observer.reconcileStampReset();
    }
  }
}

- (void)runDBTransaction:(ledger::type::DBTransactionPtr)transaction
                callback:(ledger::client::RunDBTransactionCallback)callback
{
  __weak BATBraveLedger* weakSelf = self;
  base::PostTaskAndReplyWithResult(
      databaseQueue.get(), FROM_HERE,
      base::BindOnce(&RunDBTransactionOnTaskRunner, std::move(transaction),
                     rewardsDatabase),
      base::BindOnce(^(ledger::type::DBCommandResponsePtr response) {
        if (weakSelf)
          callback(std::move(response));
      }));
}

- (void)pendingContributionSaved:(const ledger::type::Result)result
{
  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.pendingContributionAdded) {
      observer.pendingContributionAdded();
    }
  }
}

- (void)walletDisconnected:(const std::string &)wallet_type
{
  const auto bridgedType = static_cast<BATWalletType>([NSString stringWithUTF8String:wallet_type.c_str()]);
  for (BATBraveLedgerObserver *observer in self.observers) {
    if (observer.externalWalletDisconnected) {
      observer.externalWalletDisconnected(bridgedType);
    }
  }
}

- (void)deleteLog:(ledger::ResultCallback)callback
{
  callback(ledger::type::Result::LEDGER_OK);
}

- (bool)setEncryptedStringState:(const std::string&)key value:(const std::string&)value
{
  const auto bridgedKey = [NSString stringWithUTF8String:key.c_str()];

  std::string encrypted_value;
  if (!OSCrypt::EncryptString(value, &encrypted_value)) {
    BLOG(0, @"Couldn't encrypt value for %@", bridgedKey);
    return false;
  }

  std::string encoded_value;
  base::Base64Encode(encrypted_value, &encoded_value);

  self.prefs[bridgedKey] = [NSString stringWithUTF8String:encoded_value.c_str()];
  [self savePrefs];
  return true;
}

- (std::string)getEncryptedStringState:(const std::string&)key
{
  const auto bridgedKey = [NSString stringWithUTF8String:key.c_str()];
  NSString *savedValue = self.prefs[bridgedKey];
  if (!savedValue || ![savedValue isKindOfClass:NSString.class]) {
    return "";
  }

  std::string encoded_value = savedValue.UTF8String;
  std::string encrypted_value;
  if (!base::Base64Decode(encoded_value, &encrypted_value)) {
    BLOG(0, @"base64 decode failed for %@", bridgedKey);
    return "";
  }

  std::string value;
  if (!OSCrypt::DecryptString(encrypted_value, &value)) {
    BLOG(0, @"Decrypting failed for %@", bridgedKey);
    return "";
  }

  return value;
}

@end
