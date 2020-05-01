/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <UIKit/UIKit.h>
#import "bat/ledger/ledger.h"
#import "bat/ledger/global_constants.h"
#import "bat/ledger/option_keys.h"

#import "Records+Private.h"
#import "ledger.mojom.objc+private.h"

#import "BATBraveLedger.h"
#import "BATBraveAds.h"
#import "BATCommonOperations.h"
#import "NSURL+Extensions.h"
#import "BATExternalWallet+DictionaryValue.h"

#import "NativeLedgerClient.h"
#import "NativeLedgerClientBridge.h"
#import "RewardsLogStream.h"
#import "CppTransformations.h"

#import <objc/runtime.h>

#import "BATLedgerDatabase.h"
#import "DataController.h"

#import "base/time/time.h"
#import "url/gurl.h"
#import "net/base/registry_controlled_domains/registry_controlled_domain.h"
#import "base/strings/sys_string_conversions.h"
#import "brave/components/brave_rewards/browser/rewards_database.h"

#define BLOG(__severity) RewardsLogStream(__FILE__, __LINE__, __severity).stream()

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
const std::map<std::string, bool> kBoolOptions = {};
const std::map<std::string, int> kIntegerOptions = {};
const std::map<std::string, double> kDoubleOptions = {};
const std::map<std::string, std::string> kStringOptions = {};
const std::map<std::string, int64_t> kInt64Options = {};
const std::map<std::string, uint64_t> kUInt64Options = {
  {ledger::kOptionPublisherListRefreshInterval,
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

@interface BATBraveLedger () <NativeLedgerClientBridge> {
  NativeLedgerClient *ledgerClient;
  ledger::Ledger *ledger;
  brave_rewards::RewardsDatabase *rewardsDatabase;
}
@property (nonatomic, copy) NSString *storagePath;
@property (nonatomic) BATWalletProperties *walletInfo;
@property (nonatomic) BATBalance *balance;
@property (nonatomic) NSMutableDictionary<BATWalletType, BATExternalWallet *> *mExternalWallets;
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

@property (nonatomic) dispatch_queue_t databaseQueue;

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
    self.mExternalWallets = [[NSMutableDictionary alloc] init];
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
    
    self.databaseQueue = dispatch_queue_create("com.rewards.db-transactions", DISPATCH_QUEUE_SERIAL);
    
    const auto* dbPath = [self rewardsDatabasePath].UTF8String;
    rewardsDatabase = new brave_rewards::RewardsDatabase(base::FilePath(dbPath));

    ledgerClient = new NativeLedgerClient(self);
    ledger = ledger::Ledger::CreateInstance(ledgerClient);

    self.migrationType = BATLedgerDatabaseMigrationTypeDefault;
    BOOL needsMigration = [self databaseNeedsMigration];
    if (needsMigration) {
      [BATLedgerDatabase deleteCoreDataServerPublisherList:nil];
    }
    [self initializeLedgerService:needsMigration];
    
    // Add notifications for standard app foreground/background
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationDidBecomeActive) name:UIApplicationDidBecomeActiveNotification object:nil];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationDidBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];

    if (self.walletCreated) {
      [self fetchWalletDetails:nil];
      [self fetchBalance:nil];
      [self fetchExternalWalletForType:BATWalletTypeUphold completion:nil];
    }

    [self readNotificationsFromDisk];
  }
  return self;
}

- (void)dealloc
{
  [NSNotificationCenter.defaultCenter removeObserver:self];
  [self.notificationStartupTimer invalidate];

  delete ledger;
  delete ledgerClient;
  delete rewardsDatabase;
}

- (void)initializeLedgerService:(BOOL)executeMigrateScript
{
  if (self.initialized || self.initializing) {
    return;
  }
  self.initializing = YES;
  
  BLOG(ledger::LogLevel::LOG_DEBUG) << "DB: Migrate from CoreData? " << (executeMigrateScript ? "YES" : "NO") << std::endl;
  ledger->Initialize(executeMigrateScript, ^(ledger::Result result){
    self.initialized = (result == ledger::Result::LEDGER_OK ||
                        result == ledger::Result::NO_LEDGER_STATE ||
                        result == ledger::Result::NO_PUBLISHER_STATE);
    self.initializing = NO;
    if (self.initialized) {
      self.prefs[kMigrationSucceeded] = @(YES);
      [self savePrefs];
    } else {
      BLOG(ledger::LogLevel::LOG_ERROR) << "Ledger Initialization Failed with error: " << std::to_string(static_cast<int>(result)) << std::endl;
      if (result == ledger::Result::DATABASE_INIT_FAILED) {
        // Failed to migrate data...
        switch (self.migrationType) {
          case BATLedgerDatabaseMigrationTypeDefault:
            BLOG(ledger::LogLevel::LOG_ERROR) << "DB: Full migration failed, attempting BAT only migration." << std::endl;
            self.dataMigrationFailed = YES;
            self.migrationType = BATLedgerDatabaseMigrationTypeTokensOnly;
            [self resetRewardsDatabase];
            // attempt re-initialize without other data
            [self initializeLedgerService:YES];
            return;
          case BATLedgerDatabaseMigrationTypeTokensOnly:
            BLOG(ledger::LogLevel::LOG_ERROR) << "DB: BAT only migration failed. Initializing without migration." << std::endl;
            self.dataMigrationFailed = YES;
            self.migrationType = BATLedgerDatabaseMigrationTypeNone;
            [self resetRewardsDatabase];
            // attempt initialize without migrating at all
            [self initializeLedgerService:NO];
            return;
          default:
            break;
        }
      }
    }
    self.initializationResult = static_cast<BATResult>(result);
    for (BATBraveLedgerObserver *observer in [self.observers copy]) {
      if (observer.walletInitalized) {
        observer.walletInitalized(self.initializationResult);
      }
    }
  });
}

- (BOOL)databaseNeedsMigration
{
  // Check if we even have a DB to migrate
  if (!DataController.defaultStoreExists) {
    return NO;
  }
  // Have we set the pref saying ledger has alaready initialized successfully?
  if ([self.prefs[kMigrationSucceeded] boolValue]) {
    return NO;
  }
  // Can we even check the DB
  if (!rewardsDatabase) {
    BLOG(ledger::LogLevel::LOG_DEBUG) << "DB: No rewards database object" << std::endl;
    return YES;
  }
  // Check integrity of the new DB. Safe to assume if `publisher_info` table
  // exists, then all the others do as well.
  auto transaction = ledger::DBTransaction::New();
  const auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'publisher_info';";
  command->record_bindings = { ledger::DBCommand::RecordBindingType::STRING_TYPE };
  transaction->commands.push_back(command->Clone());
  
  auto response = ledger::DBCommandResponse::New();
  rewardsDatabase->RunTransaction(std::move(transaction), response.get());
  
  // Failed to even run the check, tables probably don't exist,
  // restart from scratch
  if (response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    [self resetRewardsDatabase];
    BLOG(ledger::LogLevel::LOG_DEBUG) << "DB: Failed to run transaction with status: " << std::to_string(static_cast<int>(response->status)) << std::endl;
    return YES;
  }
  
  const auto record = std::move(response->result->get_records());
  // sqlite_master table exists, but the publisher_info table doesn't exist?
  // Restart from scratch
  if (record.empty() || record.front()->fields.empty()) {
    [self resetRewardsDatabase];
    BLOG(ledger::LogLevel::LOG_DEBUG) << "DB: Migrate because we couldnt find tables in sqlite_master" << std::endl;
    return YES;
  }
  
  // Tables exist so migration has happened already, but somehow the flag wasn't
  // saved.
  self.prefs[kMigrationSucceeded] = @(YES);
  [self savePrefs];
  
  return NO;
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
  rewardsDatabase = new brave_rewards::RewardsDatabase(base::FilePath(dbPath.UTF8String));
}

- (void)getCreateScript:(ledger::GetCreateScriptCallback)callback
{
  NSString *migrationScript = @"";
  switch (self.migrationType) {
    case BATLedgerDatabaseMigrationTypeNone:
      // We shouldn't be migrating, therefore doesn't make sense that
      // `getCreateScript` was called
      BLOG(ledger::LogLevel::LOG_ERROR) << "DB: Attempted CoreData migration with an empty migration script" << std::endl;
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
  NSDictionary *prefs = self.prefs;
  NSString *path = [self prefsPath];
  dispatch_async(self.fileWriteThread, ^{
    [prefs writeToFile:path atomically:YES];
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
BATClassLedgerBridge(int, reconcileTime, setReconcileTime, reconcile_time)
BATClassLedgerBridge(BOOL, useShortRetries, setUseShortRetries, short_retries)

+ (BATEnvironment)environment
{
  return static_cast<BATEnvironment>(ledger::_environment);
}

+ (void)setEnvironment:(BATEnvironment)environment
{
  ledger::_environment = static_cast<ledger::Environment>(environment);
}

#pragma mark - Wallet

BATLedgerReadonlyBridge(BOOL, isWalletCreated, IsWalletCreated)

- (void)createWallet:(void (^)(NSError * _Nullable))completion
{
  const auto __weak weakSelf = self;
  // Results that can come from CreateWallet():
  //   - WALLET_CREATED: Good to go
  //   - LEDGER_ERROR: Already initialized
  //   - BAD_REGISTRATION_RESPONSE: Request credentials call failure or malformed data
  //   - REGISTRATION_VERIFICATION_FAILED: Missing master user token
  self.initializingWallet = YES;
  ledger->CreateWallet(^(ledger::Result result) {
    const auto strongSelf = weakSelf;
    if (!strongSelf) { return; }
    NSError *error = nil;
    if (result != ledger::Result::WALLET_CREATED) {
      std::map<ledger::Result, std::string> errorDescriptions {
        { ledger::Result::LEDGER_ERROR, "The wallet was already initialized" },
        { ledger::Result::BAD_REGISTRATION_RESPONSE, "Request credentials call failure or malformed data" },
        { ledger::Result::REGISTRATION_VERIFICATION_FAILED, "Missing master user token from registered persona" },
      };
      NSDictionary *userInfo = @{};
      const auto description = errorDescriptions[static_cast<ledger::Result>(result)];
      if (description.length() > 0) {
        userInfo = @{ NSLocalizedDescriptionKey: [NSString stringWithUTF8String:description.c_str()] };
      }
      error = [NSError errorWithDomain:BATBraveLedgerErrorDomain code:static_cast<NSInteger>(result) userInfo:userInfo];
    }
    
    strongSelf.enabled = YES;
    strongSelf.autoContributeEnabled = YES;
    strongSelf.ads.enabled = [BATBraveAds isCurrentLocaleSupported];
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

- (void)fetchWalletDetails:(void (^)(BATWalletProperties * _Nullable))completion
{
  ledger->FetchWalletProperties(^(ledger::Result result, ledger::WalletPropertiesPtr info) {
    [self onWalletProperties:result arg1:std::move(info)];
    const auto __weak weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      if (completion) {
        completion(weakSelf.walletInfo);
      }
    });
  });
}

- (void)onWalletProperties:(ledger::Result)result arg1:(ledger::WalletPropertiesPtr)arg1
{
  if (result == ledger::Result::LEDGER_OK) {
    if (arg1.get() != nullptr) {
      self.walletInfo = [[BATWalletProperties alloc] initWithWalletPropertiesPtr:std::move(arg1)];
    } else {
      self.walletInfo = nil;
    }
  }
}

- (void)fetchBalance:(void (^)(BATBalance * _Nullable))completion
{
  const auto __weak weakSelf = self;
  ledger->FetchBalance(^(ledger::Result result, ledger::BalancePtr balance) {
    const auto strongSelf = weakSelf;
    if (result == ledger::Result::LEDGER_OK) {
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

- (NSString *)walletPassphrase
{
  if (ledger->IsWalletCreated()) {
    return [NSString stringWithUTF8String:ledger->GetWalletPassphrase().c_str()];
  }
  return nil;
}

- (void)recoverWalletUsingPassphrase:(NSString *)passphrase completion:(void (^)(NSError *_Nullable))completion
{
  const auto __weak weakSelf = self;
  // Results that can come from CreateWallet():
  //   - LEDGER_OK: Good to go
  //   - LEDGER_ERROR: Recovery failed
  ledger->RecoverWallet(std::string(passphrase.UTF8String),
    ^(const ledger::Result result, const double balance) {
      const auto strongSelf = weakSelf;
      if (!strongSelf) { return; }
      NSError *error = nil;
      if (result != ledger::Result::LEDGER_OK) {
        std::map<ledger::Result, std::string> errorDescriptions {
          { ledger::Result::LEDGER_ERROR, "The recovery failed" },
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

BATLedgerReadonlyBridge(double, defaultContributionAmount, GetDefaultContributionAmount)

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

#pragma mark - User Wallets

- (NSDictionary<BATWalletType, BATExternalWallet *> *)externalWallets
{
  return [self.mExternalWallets copy];
}

- (void)fetchExternalWalletForType:(BATWalletType)walletType
                        completion:(nullable void (^)(BATExternalWallet * _Nullable wallet))completion
{
  ledger->GetExternalWallet(walletType.UTF8String, ^(ledger::Result result, ledger::ExternalWalletPtr walletPtr) {
    if (result == ledger::Result::LEDGER_OK && walletPtr.get() != nullptr) {
      const auto bridgedWallet = [[BATExternalWallet alloc] initWithExternalWallet:*walletPtr];
      self.mExternalWallets[walletType] = bridgedWallet;
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
  ledger->DisconnectWallet(walletType.UTF8String, ^(ledger::Result result){
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
                                      ^(ledger::Result result, std::map<std::string, std::string> args) {
    const auto it = args.find("redirect_url");
    std::string redirect;
    if (it != args.end()) {
      redirect = it->second;
    }
    NSURL *url = redirect.empty() ? nil : [NSURL URLWithString:[NSString stringWithUTF8String:redirect.c_str()]];
    completion(static_cast<BATResult>(result), url);
    
    if (result == ledger::Result::LEDGER_OK) {
      for (BATBraveLedgerObserver *observer in self.observers) {
        if (observer.externalWalletAuthorized) {
          observer.externalWalletAuthorized(walletType);
        }
      }
    }
  });
}

- (void)getExternalWallets:(ledger::GetExternalWalletsCallback)callback
{
  std::map<std::string, ledger::ExternalWalletPtr> wallets;
  NSDictionary *externalWallets = self.prefs[kExternalWalletsPrefKey] ?: [[NSDictionary alloc] init];
  for (NSString *walletTypeKey in externalWallets) {
    const auto wallet = [[BATExternalWallet alloc] initWithDictionaryValue:externalWallets[walletTypeKey]];
    wallets.insert(std::make_pair(walletTypeKey.UTF8String, wallet.cppObjPtr));
  }
  callback(std::move(wallets));
}

- (void)saveExternalWallet:(const std::string &)wallet_type wallet:(ledger::ExternalWalletPtr)wallet
{
  if (wallet.get() == nullptr) { return; }
  const auto bridgedWallet = [[BATExternalWallet alloc] initWithExternalWallet:*wallet];
  const auto bridgedType = [NSString stringWithUTF8String:wallet_type.c_str()];
  NSMutableDictionary *externalWallets = [self.prefs[kExternalWalletsPrefKey] mutableCopy] ?: [[NSMutableDictionary alloc] init];
  externalWallets[bridgedType] = [bridgedWallet dictionaryValue];
  self.prefs[kExternalWalletsPrefKey] = externalWallets;
  [self savePrefs];
}

- (void)setTransferFee:(const std::string &)wallet_type transfer_fee:(ledger::TransferFeePtr)transfer_fee
{
  if (transfer_fee.get() == nullptr) {
    return;
  }
  const auto bridgedType = [NSString stringWithUTF8String:wallet_type.c_str()];
  const auto feeID = [NSString stringWithUTF8String:transfer_fee->id.c_str()];
  const auto feeDict = @{
    @"id": feeID,
    @"amount": @(transfer_fee->amount),
    @"execution_timestamp": @(transfer_fee->execution_timestamp),
    @"execution_id": @(transfer_fee->execution_id)
  };

  NSMutableDictionary *transferFees = [self.prefs[kTransferFeesPrefKey] mutableCopy] ?: [[NSMutableDictionary alloc] init];
  NSMutableDictionary *feesForWalletType = [transferFees[bridgedType] mutableCopy] ?: [[NSMutableDictionary alloc] init];
  feesForWalletType[feeID] = feeDict;
  transferFees[bridgedType] = feesForWalletType;
  self.prefs[kTransferFeesPrefKey] = transferFees;
  [self savePrefs];
}

- (ledger::TransferFeeList)getTransferFees:(const std::string &)wallet_type
{
  const auto bridgedType = [NSString stringWithUTF8String:wallet_type.c_str()];
  ledger::TransferFeeList list;
  NSDictionary *transferFees = self.prefs[kTransferFeesPrefKey] ?: [[NSDictionary alloc] init];
  NSDictionary *walletFees = transferFees[bridgedType];
  if (!walletFees || walletFees.count == 0) {
    return list;
  }
  for (NSString *feeID in walletFees) {
    NSDictionary* feeDict = walletFees[feeID];
    auto fee = ledger::TransferFee::New();
    fee->id = feeID.UTF8String;
    fee->amount = [feeDict[@"amount"] doubleValue];
    fee->execution_id = [feeDict[@"execution_id"] longValue];
    fee->execution_timestamp = [feeDict[@"execution_timestamp"] longLongValue];
    list.insert(std::make_pair(feeID.UTF8String, std::move(fee)));
  }
  return list;
}

- (void)removeTransferFee:(const std::string &)wallet_type id:(const std::string &)id
{
  const auto bridgedType = [NSString stringWithUTF8String:wallet_type.c_str()];
  const auto bridgedID = [NSString stringWithUTF8String:id.c_str()];
  NSMutableDictionary *transferFees = [self.prefs[kTransferFeesPrefKey] mutableCopy] ?: [[NSMutableDictionary alloc] init];
  NSMutableDictionary *feesForWalletType = [transferFees[bridgedType] mutableCopy] ?: [[NSMutableDictionary alloc] init];
  [feesForWalletType removeObjectForKey:bridgedID];
  transferFees[bridgedType] = feesForWalletType;
  self.prefs[kTransferFeesPrefKey] = transferFees;
  [self savePrefs];
}

#pragma mark - Publishers

- (void)listActivityInfoFromStart:(unsigned int)start
                            limit:(unsigned int)limit
                           filter:(BATActivityInfoFilter *)filter
                       completion:(void (NS_NOESCAPE ^)(NSArray<BATPublisherInfo *> *))completion
{
  auto cppFilter = filter ? filter.cppObjPtr : ledger::ActivityInfoFilter::New();
  if (filter.excluded == BATExcludeFilterFilterExcluded) {
    ledger->GetExcludedList(^(ledger::PublisherInfoList list) {
      const auto publishers = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::PublisherInfoPtr& info){
        return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
      });
      completion(publishers);
    });
  } else {
    ledger->GetActivityInfoList(start, limit, std::move(cppFilter), ^(ledger::PublisherInfoList list) {
      const auto publishers = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::PublisherInfoPtr& info){
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

  ledger::VisitDataPtr visitData = ledger::VisitData::New();
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
  ledger->SetPublisherExclude(std::string(publisherId.UTF8String), (ledger::PublisherExclude)state, ^(const ledger::Result result) {
    if (result != ledger::Result::LEDGER_OK) {
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
  ledger->RestorePublishers(^(const ledger::Result result) {
    if (result != ledger::Result::LEDGER_OK) {
      return;
    }

    for (BATBraveLedgerObserver *observer in [self.observers copy]) {
      if (observer.excludedSitesChanged) {
        observer.excludedSitesChanged(@"-1",
                                      static_cast<BATPublisherExclude>(ledger::PublisherExclude::ALL));
      }
    }
  });
}

- (void)publisherBannerForId:(NSString *)publisherId completion:(void (^)(BATPublisherBanner * _Nullable banner))completion
{
  ledger->GetPublisherBanner(std::string(publisherId.UTF8String), ^(ledger::PublisherBannerPtr banner) {
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
  ledger->RefreshPublisher(std::string(publisherId.UTF8String), ^(ledger::PublisherStatus status) {
    completion(static_cast<BATPublisherStatus>(status));
  });
}

#pragma mark - SKUs

- (void)processSKUItems:(NSArray<BATSKUOrderItem *> *)items
             completion:(void (^)(BATResult result, NSString *orderID))completion
{
  auto wallet = ledger::ExternalWallet::New();
  wallet->type = ledger::kWalletUnBlinded;
  
  ledger->ProcessSKU(VectorFromNSArray(items, ^ledger::SKUOrderItem(BATSKUOrderItem *item) {
    return *item.cppObjPtr;
  }), std::move(wallet), ^(const ledger::Result result, const std::string& order_id) {
    completion(static_cast<BATResult>(result), [NSString stringWithUTF8String:order_id.c_str()]);
  });
}

#pragma mark - Tips

- (void)listRecurringTips:(void (^)(NSArray<BATPublisherInfo *> *))completion
{
  ledger->GetRecurringTips(^(ledger::PublisherInfoList list){
    const auto publishers = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::PublisherInfoPtr& info){
      return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
    });
    completion(publishers);
  });
}

- (void)addRecurringTipToPublisherWithId:(NSString *)publisherId amount:(double)amount completion:(void (^)(BOOL success))completion
{
  ledger::RecurringTipPtr info = ledger::RecurringTip::New();
  info->publisher_key = publisherId.UTF8String;
  info->amount = amount;
  info->created_at = [[NSDate date] timeIntervalSince1970];
  ledger->SaveRecurringTip(std::move(info), ^(ledger::Result result){
    const auto success = (result == ledger::Result::LEDGER_OK);
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
  ledger->RemoveRecurringTip(std::string(publisherId.UTF8String), ^(ledger::Result result){
    if (result == ledger::Result::LEDGER_OK) {
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
  ledger->GetOneTimeTips(^(ledger::PublisherInfoList list){
    const auto publishers = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::PublisherInfoPtr& info){
      return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
    });
    completion(publishers);
  });
}

- (void)tipPublisherDirectly:(BATPublisherInfo *)publisher amount:(double)amount currency:(NSString *)currency completion:(void (^)(BATResult result))completion
{
  ledger->OneTimeTip(std::string(publisher.id.UTF8String), amount, ^(ledger::Result result) {
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

- (NSString *)notificationIDForPromo:(const ledger::PromotionPtr)promo
{
  bool isUGP = promo->type == ledger::PromotionType::UGP;
  const auto prefix = isUGP ? @"rewards_grant_" : @"rewards_grant_ads_";
  const auto promotionId = [NSString stringWithUTF8String:promo->id.c_str()];
  return [NSString stringWithFormat:@"%@%@", prefix, promotionId];
}

- (void)updatePendingAndFinishedPromotions:(void (^)())completion
{
  ledger->GetAllPromotions(^(ledger::PromotionMap map) {
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
  ledger->FetchPromotions(^(ledger::Result result, std::vector<ledger::PromotionPtr> promotions) {
    if (result != ledger::Result::LEDGER_OK) {
      return;
    }
    [self updatePendingAndFinishedPromotions:^{
      if (completion) {
        completion(self.pendingPromotions);
      }
    }];
  });
}

- (void)claimPromotion:(NSString *)deviceCheckPublicKey completion:(void (^)(BATResult result, NSString * _Nonnull nonce))completion
{
  const auto payload = [NSDictionary dictionaryWithObject:deviceCheckPublicKey forKey:@"publicKey"];
  const auto jsonData = [NSJSONSerialization dataWithJSONObject:payload options:0 error:nil];
  if (!jsonData) {
    BLOG(ledger::LogLevel::LOG_ERROR) << "Missing JSON payload while attempting to claim promotion" << std::endl;
    return;
  }
  const auto jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  ledger->ClaimPromotion(jsonString.UTF8String, ^(const ledger::Result result, const std::string& json) {
    const auto jsonData = [[NSString stringWithUTF8String:json.c_str()] dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary *nonce = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:nil];
    dispatch_async(dispatch_get_main_queue(), ^{
      completion(static_cast<BATResult>(result),
                 nonce[@"nonce"]);
    });
  });
}

- (void)attestPromotion:(NSString *)promotionId solution:(BATPromotionSolution *)solution completion:(void (^)(BATResult result, BATPromotion * _Nullable promotion))completion
{
  ledger->AttestPromotion(std::string(promotionId.UTF8String), solution.JSONPayload.UTF8String, ^(const ledger::Result result, ledger::PromotionPtr promotion) {
    if (promotion.get() == nullptr) return;
    
    const auto bridgedPromotion = [[BATPromotion alloc] initWithPromotion:*promotion];
    if (result == ledger::Result::LEDGER_OK) {
      [self fetchBalance:nil];
      [self clearNotificationWithID:[self notificationIDForPromo:std::move(promotion)]];
    }
    
    dispatch_async(dispatch_get_main_queue(), ^{
      if (completion) {
        completion(static_cast<BATResult>(result), bridgedPromotion);
      }
      if (result == ledger::Result::LEDGER_OK) {
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

- (NSDictionary<NSString *, BATBalanceReportInfo *> *)balanceReports
{
  const auto reports = ledger->GetAllBalanceReports();
  const auto bridgedReports = [[NSMutableDictionary<NSString *, BATBalanceReportInfo *> alloc] init];
  for (const auto& r : reports) {
    if (r.second.get() == nullptr) { continue; }
    bridgedReports[[NSString stringWithUTF8String:r.first.c_str()]] =
      [[BATBalanceReportInfo alloc] initWithBalanceReportInfo:*r.second];
  }
  return bridgedReports;
}

- (void)balanceReportForMonth:(BATActivityMonth)month year:(int)year completion:(void (NS_NOESCAPE ^)(BATBalanceReportInfo * _Nullable info))completion
{
  ledger->GetBalanceReport((ledger::ActivityMonth)month, year, ^(const ledger::Result result, ledger::BalanceReportInfoPtr info) {
    auto bridgedInfo = info.get() != nullptr ? [[BATBalanceReportInfo alloc] initWithBalanceReportInfo:*info.get()] : nil;
    completion(result == ledger::Result::LEDGER_OK ? bridgedInfo : nil);
  });
}

- (BATAutoContributeProps *)autoContributeProps
{
  ledger::AutoContributePropsPtr props = ledger->GetAutoContributeProps();
  return [[BATAutoContributeProps alloc] initWithAutoContributePropsPtr:std::move(props)];
}

#pragma mark - Pending Contributions

- (void)pendingContributions:(void (^)(NSArray<BATPendingContributionInfo *> *publishers))completion
{
  ledger->GetPendingContributions(^(ledger::PendingContributionInfoList list){
    const auto convetedList = NSArrayFromVector(&list, ^BATPendingContributionInfo *(const ledger::PendingContributionInfoPtr& info){
      return [[BATPendingContributionInfo alloc] initWithPendingContributionInfo:*info];
    });
    completion(convetedList);
  });
}

- (void)removePendingContribution:(BATPendingContributionInfo *)info completion:(void (^)(BATResult result))completion
{
  ledger->RemovePendingContribution(info.id,
                                    ^(const ledger::Result result){
    completion(static_cast<BATResult>(result));
  });
}

- (void)removeAllPendingContributions:(void (^)(BATResult result))completion
{
  ledger->RemoveAllPendingContributions(^(const ledger::Result result){
    completion(static_cast<BATResult>(result));
  });
}

#pragma mark - Reconcile

- (void)onReconcileComplete:(ledger::Result)result viewingId:(const std::string &)viewing_id type:(const ledger::RewardsType)type amount:(const double)amount
{
  // TODO we changed from probi to amount, so from string to double
  if (result == ledger::Result::LEDGER_OK) {
    if (type == ledger::RewardsType::RECURRING_TIP) {
      [self showTipsProcessedNotificationIfNeccessary];
    }
    [self fetchBalance:nil];
  }

  if ((result == ledger::Result::LEDGER_OK && type == ledger::RewardsType::AUTO_CONTRIBUTE) ||
      result == ledger::Result::LEDGER_ERROR ||
      result == ledger::Result::NOT_ENOUGH_FUNDS ||
      result == ledger::Result::TIP_ERROR) {
    const auto viewingId = [NSString stringWithUTF8String:viewing_id.c_str()];
    const auto info = @{ @"viewingId": viewingId,
                         @"result": @((BATResult)result),
                         @"type": @((BATRewardsType)type),
                         @"amount": [@(amount) stringValue] };

    [self addNotificationOfKind:BATRewardsNotificationKindAutoContribute
                       userInfo:info
                 notificationID:[NSString stringWithFormat:@"contribution_%@", viewingId]];
  }

  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.balanceReportUpdated) {
      observer.balanceReportUpdated();
    }
    if (observer.reconcileCompleted) {
      observer.reconcileCompleted(static_cast<BATResult>(result),
                                  [NSString stringWithUTF8String:viewing_id.c_str()],
                                  static_cast<BATRewardsType>(type),
                                  [@(amount) stringValue]);
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
  ledger->GetRewardsInternalsInfo(^(ledger::RewardsInternalsInfoPtr info) {
    auto bridgedInfo = info.get() != nullptr ? [[BATRewardsInternalsInfo alloc] initWithRewardsInternalsInfo:*info.get()] : nil;
    completion(bridgedInfo);
  });
}

- (void)loadNicewareList:(ledger::GetNicewareListCallback)callback
{
  NSError *error;
  const auto bundle = [NSBundle bundleForClass:[BATBraveLedger class]];
  const auto path = [bundle pathForResource:@"wordlist" ofType:nil];
  const auto contents = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:&error];
  if (error || contents.length == 0) {
    callback(ledger::Result::LEDGER_ERROR, "");
  } else {
    callback(ledger::Result::LEDGER_OK, std::string(contents.UTF8String));
  }
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
  
  ledger::VisitDataPtr data = ledger::VisitData::New();
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
  
  std::map<std::string, std::string> partsMap;
  const auto urlComponents = [[NSURLComponents alloc] initWithURL:url resolvingAgainstBaseURL:NO];
  for (NSURLQueryItem *item in urlComponents.queryItems) {
    std::string value = item.value != nil ? item.value.UTF8String : "";
    partsMap[std::string(item.name.UTF8String)] = value;
  }

  auto visit = ledger::VisitData::New();
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
  
  auto visit = ledger::VisitData::New();
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

BATLedgerReadonlyBridge(BOOL, isEnabled, GetRewardsMainEnabled)

- (void)setEnabled:(BOOL)enabled
{
  ledger->SetRewardsMainEnabled(enabled);
  if (enabled) {
    [self.ads initializeIfAdsEnabled];
  } else {
    [self.ads shutdown];
  }

  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.rewardsEnabledStateUpdated) {
      observer.rewardsEnabledStateUpdated(enabled);
    }
  }
}

BATLedgerBridge(UInt64,
                minimumVisitDuration, setMinimumVisitDuration,
                GetPublisherMinVisitTime, SetPublisherMinVisitTime)

BATLedgerBridge(UInt32,
                minimumNumberOfVisits, setMinimumNumberOfVisits,
                GetPublisherMinVisits, SetPublisherMinVisits)

BATLedgerBridge(BOOL,
                allowUnverifiedPublishers, setAllowUnverifiedPublishers,
                GetPublisherAllowNonVerified, SetPublisherAllowNonVerified)

BATLedgerBridge(BOOL,
                allowVideoContributions, setAllowVideoContributions,
                GetPublisherAllowVideos, SetPublisherAllowVideos)

BATLedgerReadonlyBridge(double, contributionAmount, GetContributionAmount)

- (void)setContributionAmount:(double)contributionAmount
{
  ledger->SetUserChangedContribution();
  ledger->SetContributionAmount(contributionAmount);
}

BATLedgerBridge(BOOL,
                isAutoContributeEnabled, setAutoContributeEnabled,
                GetAutoContribute, SetAutoContribute)

- (void)setBooleanState:(const std::string&)name value:(bool)value
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  self.prefs[key] = [NSNumber numberWithBool:value];
  [self savePrefs];
}

- (bool)getBooleanState:(const std::string&)name
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
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

#pragma mark - Ads & Confirmations

- (void)confirmAd:(NSString *)json confirmationType:(NSString *)confirmationType
{
  ledger->ConfirmAd(json.UTF8String, confirmationType.UTF8String);
}

- (void)confirmAction:(NSString *)creativeInstanceId creativeSetID:(NSString *)creativeSetID confirmationType:(NSString *)confirmationType
{
  ledger->ConfirmAction(creativeInstanceId.UTF8String, creativeSetID.UTF8String, confirmationType.UTF8String);
}

- (void)setCatalogIssuers:(NSString *)issuers
{
  ledger->SetCatalogIssuers(issuers.UTF8String);
}

- (void)updateAdsRewards
{
  ledger->UpdateAdsRewards();
}

- (void)adsDetailsForCurrentCycle:(void (^)(NSInteger adsReceived, double estimatedEarnings, NSDate *nextPaymentDate))completion
{
  ledger->GetTransactionHistory(^(std::unique_ptr<ledger::TransactionsInfo> list) {
    if (list == nullptr) {
      completion(0, 0.0, nil);
    } else {
      NSDate *nextPaymentDate = nil;
      if (list->next_payment_date_in_seconds > 0) {
        nextPaymentDate = [NSDate dateWithTimeIntervalSince1970:list->next_payment_date_in_seconds];
      }
      completion(list->ad_notifications_received_this_month,
                 list->estimated_pending_rewards,
                 nextPaymentDate);
    }
  });
}

- (void)setConfirmationsIsReady:(const bool)is_ready
{
   [self.ads setConfirmationsIsReady:is_ready];
}

- (void)confirmationsTransactionHistoryDidChange
{
  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.confirmationsTransactionHistoryDidChange) {
      observer.confirmationsTransactionHistoryDidChange();
    }
  }
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
  if (!self.isEnabled) {
    return;
  }

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
  auto bootstamp = ledger->GetBootStamp();
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
      BLOG(ledger::LogLevel::LOG_ERROR) << "Failed to unarchive notifications on disk: " << error.debugDescription.UTF8String << std::endl;
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
      BLOG(ledger::LogLevel::LOG_ERROR) << "Failed to write notifications to disk: " << error.debugDescription.UTF8String << std::endl;
    }
    return;
  }

  dispatch_async(self.fileWriteThread, ^{
    [data writeToFile:path atomically:YES];
  });
}

#pragma mark - State

- (void)loadLedgerState:(ledger::OnLoadCallback)callback
{
  const auto contents = [self.commonOps loadContentsFromFileWithName:"ledger_state.json"];
  if (contents.length() > 0) {
    callback(ledger::Result::LEDGER_OK, contents);
  } else {
    callback(ledger::Result::NO_LEDGER_STATE, contents);
  }
  [self startNotificationTimers];
}

- (void)saveLedgerState:(const std::string &)ledger_state callback:(ledger::ResultCallback)callback
{
  const auto result = [self.commonOps saveContents:ledger_state name:"ledger_state.json"];
  callback(result ? ledger::Result::LEDGER_OK : ledger::Result::NO_LEDGER_STATE);
}

- (void)loadPublisherState:(ledger::OnLoadCallback)callback
{
  const auto contents = [self.commonOps loadContentsFromFileWithName:"publisher_state.json"];
  if (contents.length() > 0) {
    callback(ledger::Result::LEDGER_OK, contents);
  } else {
    callback(ledger::Result::NO_PUBLISHER_STATE, contents);
  }
}

- (void)savePublisherState:(const std::string &)publisher_state callback:(ledger::ResultCallback)callback
{
  const auto result = [self.commonOps saveContents:publisher_state name:"publisher_state.json"];
  callback(result ? ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR);
}

- (void)loadState:(const std::string &)name callback:(ledger::OnLoadCallback)callback
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  const auto value = self.state[key];
  if (value) {
    callback(ledger::Result::LEDGER_OK, std::string(value.UTF8String));
  } else {
    callback(ledger::Result::LEDGER_ERROR, "");
  }
}

- (void)resetState:(const std::string &)name callback:(ledger::ResultCallback)callback
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  self.state[key] = nil;
  callback(ledger::Result::LEDGER_OK);
  // In brave-core, failed callback returns `LEDGER_ERROR`
  NSDictionary *state = [self.state copy];
  NSString *path = [self.randomStatePath copy];
  dispatch_async(self.fileWriteThread, ^{
    [state writeToFile:path atomically:YES];
  });
}

- (void)saveState:(const std::string &)name value:(const std::string &)value callback:(ledger::ResultCallback)callback
{
  const auto key = [NSString stringWithUTF8String:name.c_str()];
  self.state[key] = [NSString stringWithUTF8String:value.c_str()];
  callback(ledger::Result::LEDGER_OK);
  // In brave-core, failed callback returns `LEDGER_ERROR`
  NSDictionary *state = [self.state copy];
  NSString *path = [self.randomStatePath copy];
  dispatch_async(self.fileWriteThread, ^{
    [state writeToFile:path atomically:YES];
  });
}

#pragma mark - Timers

- (void)setTimer:(uint64_t)time_offset timerId:(uint32_t *)timer_id
{
  const auto __weak weakSelf = self;
  const auto createdTimerID = [self.commonOps createTimerWithOffset:time_offset timerFired:^(uint32_t firedTimerID) {
    const auto strongSelf = weakSelf;
    if (!strongSelf.commonOps) { return; }
    strongSelf->ledger->OnTimer(firedTimerID);
    [strongSelf.commonOps removeTimerWithID:firedTimerID];
  }];
  *timer_id = createdTimerID;
}

- (void)killTimer:(const uint32_t)timer_id
{
  [self.commonOps removeTimerWithID:timer_id];
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

- (void)loadURL:(const std::string &)url headers:(const std::vector<std::string> &)headers content:(const std::string &)content contentType:(const std::string &)contentType method:(const ledger::UrlMethod)method callback:(ledger::LoadURLCallback)callback
{
  std::map<ledger::UrlMethod, std::string> methodMap {
    {ledger::UrlMethod::GET, "GET"},
    {ledger::UrlMethod::POST, "POST"},
    {ledger::UrlMethod::PUT, "PUT"}
  };
  return [self.commonOps loadURLRequest:url headers:headers content:content content_type:contentType method:methodMap[method] callback:^(int statusCode, const std::string &response, const std::map<std::string, std::string> &headers) {
    callback(statusCode, response, headers);
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

- (void)fetchFavIcon:(const std::string &)url faviconKey:(const std::string &)favicon_key callback:(ledger::FetchIconCallback)callback
{
  if (!self.faviconFetcher) {
    callback(NO, std::string());
    return;
  }
  const auto pageURL = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
  self.faviconFetcher(pageURL, ^(NSURL * _Nullable faviconURL) {
    dispatch_async(dispatch_get_main_queue(), ^{
      callback(faviconURL != nil,
               faviconURL.absoluteString.UTF8String);
    });
  });
}

#pragma mark - Logging

- (std::unique_ptr<ledger::LogStream>)verboseLog:(const char *)file line:(int)line vlogLevel:(int)vlog_level
{
  return std::make_unique<RewardsLogStream>(file, line, (ledger::LogLevel)vlog_level);
}

- (std::unique_ptr<ledger::LogStream>)log:(const char *)file line:(int)line logLevel:(const ledger::LogLevel)log_level
{
  return std::make_unique<RewardsLogStream>(file, line, log_level);
}

#pragma mark - Publisher Database

- (void)handlePublisherListing:(NSArray<BATPublisherInfo *> *)publishers start:(uint32_t)start limit:(uint32_t)limit callback:(ledger::PublisherInfoListCallback)callback
{
  callback(VectorFromNSArray(publishers, ^ledger::PublisherInfoPtr(BATPublisherInfo *info){
    return info.cppObjPtr;
  }));
}
- (void)publisherListNormalized:(ledger::PublisherInfoList)list
{
  const auto list_converted = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::PublisherInfoPtr& info) {
    return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
  });

  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.publisherListNormalized) {
      observer.publisherListNormalized(list_converted);
    }
  }
}

- (void)onPanelPublisherInfo:(ledger::Result)result publisherInfo:(ledger::PublisherInfoPtr)publisher_info windowId:(uint64_t)windowId
{
  if (publisher_info.get() == nullptr || result != ledger::Result::LEDGER_OK) {
    return;
  }
  auto info = [[BATPublisherInfo alloc] initWithPublisherInfo:*publisher_info];
  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.fetchedPanelPublisher) {
      observer.fetchedPanelPublisher(info, windowId);
    }
  }
}

- (void)onContributeUnverifiedPublishers:(ledger::Result)result publisherKey:(const std::string &)publisher_key publisherName:(const std::string &)publisher_name
{
  switch (result) {
    case ledger::Result::PENDING_NOT_ENOUGH_FUNDS:
      [self addNotificationOfKind:BATRewardsNotificationKindPendingNotEnoughFunds
                         userInfo:nil
                   notificationID:@"not_enough_funds_for_pending"];
      break;
    case ledger::Result::PENDING_PUBLISHER_REMOVED: {
      const auto publisherID = [NSString stringWithUTF8String:publisher_key.c_str()];
      for (BATBraveLedgerObserver *observer in [self.observers copy]) {
        if (observer.pendingContributionsRemoved) {
          observer.pendingContributionsRemoved(@[publisherID]);
        }
      }
      break;
    }
    case ledger::Result::VERIFIED_PUBLISHER: {
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
- (ledger::ClientInfoPtr)getClientInfo
{
  auto info = ledger::ClientInfo::New();
  info->os = ledger::OperatingSystem::UNDEFINED;
  info->platform = ledger::Platform::IOS;
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

- (void)runDBTransaction:(ledger::DBTransactionPtr)transaction
                callback:(ledger::RunDBTransactionCallback)callback
{
  if (!rewardsDatabase || transaction.get() == nullptr) {
    auto response = ledger::DBCommandResponse::New();
    response->status = ledger::DBCommandResponse::Status::RESPONSE_ERROR;
    callback(std::move(response));
  } else {
    __block auto transactionClone = transaction->Clone();
    dispatch_async(self.databaseQueue, ^{
      __block auto response = ledger::DBCommandResponse::New();
      rewardsDatabase->RunTransaction(std::move(transactionClone), response.get());
      dispatch_async(dispatch_get_main_queue(), ^{
        callback(std::move(response));
      });
    });
  }
}

- (void)pendingContributionSaved:(const ledger::Result)result
{
  for (BATBraveLedgerObserver *observer in [self.observers copy]) {
    if (observer.pendingContributionAdded) {
      observer.pendingContributionAdded();
    }
  }
}

@end
