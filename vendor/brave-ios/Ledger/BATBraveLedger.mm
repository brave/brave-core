/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <UIKit/UIKit.h>
#import "bat/ledger/ledger.h"

#import "Records+Private.h"
#import "ledger.mojom.objc+private.h"

#import "BATBraveLedger.h"
#import "BATBraveAds.h"
#import "BATCommonOperations.h"
#import "NSURL+Extensions.h"

#import "NativeLedgerClient.h"
#import "NativeLedgerClientBridge.h"
#import "RewardsLogStream.h"
#import "CppTransformations.h"

#import <objc/runtime.h>

#import "BATLedgerDatabase.h"

#import "url/gurl.h"
#import "net/base/registry_controlled_domains/registry_controlled_domain.h"

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

static NSString * const kNextAddFundsDateNotificationKey = @"BATNextAddFundsDateNotification";
static NSString * const kBackupNotificationIntervalKey = @"BATBackupNotificationInterval";
static NSString * const kBackupNotificationFrequencyKey = @"BATBackupNotificationFrequency";
static NSString * const kUserHasFundedKey = @"BATRewardsUserHasFunded";
static NSString * const kBackupSucceededKey = @"BATRewardsBackupSucceeded";

static const auto kOneDay = 24 * 60 * 60;

NS_INLINE ledger::ACTIVITY_MONTH BATGetPublisherMonth(NSDate *date) {
  const auto month = [[NSCalendar currentCalendar] component:NSCalendarUnitMonth fromDate:date];
  return (ledger::ACTIVITY_MONTH)month;
}

NS_INLINE int BATGetPublisherYear(NSDate *date) {
  return (int)[[NSCalendar currentCalendar] component:NSCalendarUnitYear fromDate:date];
}

@interface BATBraveLedger () <NativeLedgerClientBridge> {
  NativeLedgerClient *ledgerClient;
  ledger::Ledger *ledger;
}
@property (nonatomic, copy) NSString *storagePath;
@property (nonatomic) BATWalletProperties *walletInfo;
@property (nonatomic) BATBalance *balance;
@property (nonatomic) dispatch_queue_t fileWriteThread;
@property (nonatomic) NSMutableDictionary<NSString *, NSString *> *state;
@property (nonatomic) BATCommonOperations *commonOps;
@property (nonatomic) NSMutableDictionary<NSString *, __kindof NSObject *> *prefs;

@property (nonatomic) NSMutableArray<BATGrant *> *mPendingGrants;

@property (nonatomic) NSHashTable<BATBraveLedgerObserver *> *observers;

@property (nonatomic, getter=isLoadingPublisherList) BOOL loadingPublisherList;

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
    self.mPendingGrants = [[NSMutableArray alloc] init];
    self.observers = [NSHashTable weakObjectsHashTable];

    self.prefs = [[NSMutableDictionary alloc] initWithContentsOfFile:[self prefsPath]];
    if (!self.prefs) {
      self.prefs = [[NSMutableDictionary alloc] init];
      // Setup defaults
      self.prefs[kNextAddFundsDateNotificationKey] = @([[NSDate date] timeIntervalSince1970]);
      self.prefs[kBackupNotificationFrequencyKey] = @(7 * kOneDay); // 7 days
      self.prefs[kBackupNotificationIntervalKey] = @(7 * kOneDay); // 7 days
      self.prefs[kBackupSucceededKey] = @(NO);
      self.prefs[kUserHasFundedKey] = @(NO);
      [self savePrefs];
    }

    ledgerClient = new NativeLedgerClient(self);
    ledger = ledger::Ledger::CreateInstance(ledgerClient);
    ledger->Initialize(^(ledger::Result result){
      for (BATBraveLedgerObserver *observer in self.observers) {
        if (observer.walletInitalized) {
          observer.walletInitalized(static_cast<BATResult>(result));
        }
      }
    });

    // Add notifications for standard app foreground/background
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationDidBecomeActive) name:UIApplicationDidBecomeActiveNotification object:nil];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationDidBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];

    if (self.walletCreated) {
      [self fetchWalletDetails:nil];
      [self fetchBalance:nil];
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
BATClassLedgerBridge(BOOL, isProduction, setProduction, is_production)
BATClassLedgerBridge(int, reconcileTime, setReconcileTime, reconcile_time)
BATClassLedgerBridge(BOOL, useShortRetries, setUseShortRetries, short_retries)

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
  ledger->CreateWallet(std::string(), ^(ledger::Result result) {
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
    if (completion) {
      strongSelf.enabled = YES;
      strongSelf.autoContributeEnabled = YES;
      strongSelf.ads.enabled = YES;
      [strongSelf startNotificationTimers];
      dispatch_async(dispatch_get_main_queue(), ^{
        completion(error);
      });
    }
    
    for (BATBraveLedgerObserver *observer in strongSelf.observers) {
      if (observer.walletInitalized) {
        observer.walletInitalized(static_cast<BATResult>(result));
      }
    }
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
      for (BATBraveLedgerObserver *observer in self.observers) {
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
    ^(const ledger::Result result, const double balance, std::vector<ledger::GrantPtr> grants) {
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

- (double)reservedAmount {
  return [BATLedgerDatabase reservedAmountForPendingContributions];
}

BATLedgerReadonlyBridge(double, defaultContributionAmount, GetDefaultContributionAmount)

- (void)hasSufficientBalanceToReconcile:(void (^)(BOOL))completion
{
  ledger->HasSufficientBalanceToReconcile(completion);
}

#pragma mark - Publishers

- (void)listActivityInfoFromStart:(unsigned int)start
                            limit:(unsigned int)limit
                           filter:(BATActivityInfoFilter *)filter
                       completion:(void (NS_NOESCAPE ^)(NSArray<BATPublisherInfo *> *))completion
{
  auto cppFilter = filter ? filter.cppObjPtr : ledger::ActivityInfoFilter::New();
  if (filter.excluded == BATExcludeFilterFilterExcluded) {
    completion([BATLedgerDatabase excludedPublishers]);
  } else {
    ledger->GetActivityInfoList(start, limit, std::move(cppFilter), ^(const ledger::PublisherInfoList& list, uint32_t nextRecord) {
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
  GURL parsedUrl(URL.absoluteString.UTF8String);

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
  
  if (faviconURL) {
    visitData->favicon_url = std::string(faviconURL.absoluteString.UTF8String);
  }

  std::string blob = std::string();
  if (publisherBlob) {
    blob = std::string(publisherBlob.UTF8String);
  }

  ledger->GetPublisherActivityFromUrl(tabId, std::move(visitData), blob);
}

- (void)deleteActivityInfo:(const std::string &)publisher_key callback:(ledger::DeleteActivityInfoCallback )callback
{
  const auto bridgedKey = [NSString stringWithUTF8String:publisher_key.c_str()];
  const auto stamp = ledger->GetReconcileStamp();
  [BATLedgerDatabase deleteActivityInfoWithPublisherID:bridgedKey reconcileStamp:stamp completion:^(BOOL success) {
    if (success) {
      for (BATBraveLedgerObserver *observer in self.observers) {
        if (observer.activityRemoved) {
          observer.activityRemoved([NSString stringWithUTF8String:publisher_key.c_str()]);
        }
      }
    }
    callback(success ? ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR);
  }];
}

- (void)updatePublisherExclusionState:(NSString *)publisherId state:(BATPublisherExclude)state
{
  ledger->SetPublisherExclude(std::string(publisherId.UTF8String), (ledger::PUBLISHER_EXCLUDE)state, ^(const ledger::Result result) {
    if (result != ledger::Result::LEDGER_OK) {
      return;
    }
    for (BATBraveLedgerObserver *observer in self.observers) {
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

    for (BATBraveLedgerObserver *observer in self.observers) {
      if (observer.excludedSitesChanged) {
        observer.excludedSitesChanged(@"-1",
                                      static_cast<BATPublisherExclude>(ledger::PUBLISHER_EXCLUDE::ALL));
      }
    }
  });
}

- (NSUInteger)numberOfExcludedPublishers
{
  return [BATLedgerDatabase excludedPublishersCount];
}

- (void)publisherBannerForId:(NSString *)publisherId completion:(void (NS_NOESCAPE ^)(BATPublisherBanner * _Nullable banner))completion
{
  ledger->GetPublisherBanner(std::string(publisherId.UTF8String), ^(ledger::PublisherBannerPtr banner) {
    auto bridgedBanner = banner.get() != nullptr ? [[BATPublisherBanner alloc] initWithPublisherBanner:*banner] : nil;
    completion(bridgedBanner);
  });
}

- (nullable BATPublisherInfo *)currentActivityInfoWithPublisherId:(NSString *)publisherId
{
  const auto stamp = ledger->GetReconcileStamp();
  const auto filter = [[BATActivityInfoFilter alloc] init];
  filter.id = publisherId;
  filter.reconcileStamp = stamp;

  return [[BATLedgerDatabase publishersWithActivityFromOffset:0 limit:1 filter:filter] firstObject];
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

#pragma mark - Tips

- (void)listRecurringTips:(void (NS_NOESCAPE ^)(NSArray<BATPublisherInfo *> *))completion
{
  ledger->GetRecurringTips(^(const ledger::PublisherInfoList& list, uint32_t){
    const auto publishers = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::PublisherInfoPtr& info){
      return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
    });
    completion(publishers);
  });
}

- (void)addRecurringTipToPublisherWithId:(NSString *)publisherId amount:(double)amount completion:(void (^)(BOOL success))completion
{
  ledger::ContributionInfoPtr info = ledger::ContributionInfo::New();
  info->publisher = publisherId.UTF8String;
  info->value = amount;
  info->date = [[NSDate date] timeIntervalSince1970];
  ledger->SaveRecurringTip(std::move(info), ^(ledger::Result result){
    const auto success = (result == ledger::Result::LEDGER_OK);
    if (success) {
      for (BATBraveLedgerObserver *observer in self.observers) {
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
      for (BATBraveLedgerObserver *observer in self.observers) {
        if (observer.recurringTipRemoved) {
          observer.recurringTipRemoved(publisherId);
        }
      }
    }
  });
}

- (void)listOneTimeTips:(void (NS_NOESCAPE ^)(NSArray<BATPublisherInfo *> *))completion
{
  ledger->GetOneTimeTips(^(const ledger::PublisherInfoList& list, uint32_t){
    const auto publishers = NSArrayFromVector(&list, ^BATPublisherInfo *(const ledger::PublisherInfoPtr& info){
      return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
    });
    completion(publishers);
  });
}

- (void)tipPublisherDirectly:(BATPublisherInfo *)publisher amount:(int)amount currency:(NSString *)currency completion:(void (^)(BATResult result))completion
{
  ledger->DoDirectTip(std::string(publisher.id.UTF8String), amount, std::string(currency.UTF8String), ^(ledger::Result result) {
    completion(static_cast<BATResult>(result));
  });
}

#pragma mark - Grants

- (NSArray<BATGrant *> *)pendingGrants
{
  return [self.mPendingGrants copy];
}

- (BOOL)isGrantUGP:(const ledger::Grant &)grant
{
  return grant.type == "ugp";
}

- (NSString *)notificationIDForGrant:(const ledger::GrantPtr)grant
{
  const auto prefix = [self isGrantUGP:*grant] ? @"rewards_grant_" : @"rewards_grant_ads_";
  const auto promotionId = [NSString stringWithUTF8String:grant->promotion_id.c_str()];
  return [NSString stringWithFormat:@"%@%@", prefix, promotionId];
}

- (void)fetchAvailableGrantsForLanguage:(NSString *)language paymentId:(NSString *)paymentId
{
  [self fetchAvailableGrantsForLanguage:language paymentId:paymentId completion:nil];
}

- (void)fetchAvailableGrantsForLanguage:(NSString *)language paymentId:(NSString *)paymentId completion:(nullable void (^)(NSArray<BATGrant *> *grants))completion
{
  ledger->FetchGrants(std::string(language.UTF8String), std::string(paymentId.UTF8String), std::string(), ^(ledger::Result result, std::vector<ledger::GrantPtr> grants) {
    if (result != ledger::Result::LEDGER_OK) {
      return;
    }
    [self.mPendingGrants removeAllObjects];
    for (int i = 0; i < grants.size(); i++) {
      ledger::GrantPtr grant = std::move(grants[i]);
      const auto bridgedGrant = [[BATGrant alloc] initWithGrant:*grant];
      [self.mPendingGrants addObject:bridgedGrant];

      bool isUGP = [self isGrantUGP:*grant];
      auto notificationKind = isUGP ? BATRewardsNotificationKindGrant : BATRewardsNotificationKindGrantAds;

      [self addNotificationOfKind:notificationKind
                         userInfo:nil
                   notificationID:[self notificationIDForGrant:std::move(grant)]
                         onlyOnce:YES];
    }
    for (BATBraveLedgerObserver *observer in self.observers) {
      if (observer.grantsAdded) {
        observer.grantsAdded(self.pendingGrants);
      }
    }
    if (completion) {
      completion(self.pendingGrants);
    }
  });
}

- (void)grantCaptchaForPromotionId:(NSString *)promoID promotionType:(NSString *)promotionType completion:(void (^)(NSString * _Nonnull, NSString * _Nonnull))completion
{
  std::vector<std::string> headers;
  headers.push_back("brave-product:brave-core");
  headers.push_back("promotion-id:" + std::string(promoID.UTF8String));
  headers.push_back("promotion-type:" + std::string(promotionType.UTF8String));
  ledger->GetGrantCaptcha(headers,
      ^(const std::string &image, const std::string &hint) {
        dispatch_async(dispatch_get_main_queue(), ^{
          completion([NSString stringWithUTF8String:image.c_str()],
                     [NSString stringWithUTF8String:hint.c_str()]);
        });
      });
}

- (void)solveGrantCaptchWithPromotionId:(NSString *)promotionId solution:(NSString *)solution
{
  ledger->SolveGrantCaptcha(std::string(solution.UTF8String),
                            std::string(promotionId.UTF8String));
}

- (void)onGrantFinish:(ledger::Result)result grant:(ledger::GrantPtr)grant
{
  ledger::BalanceReportInfo report_info;
  auto now = [NSDate date];
  const auto bridgedGrant = [[BATGrant alloc] initWithGrant:*grant];
  if (result == ledger::Result::LEDGER_OK) {
    ledger::ReportType report_type = grant->type == "ads" ? ledger::ADS : ledger::GRANT;
    [self fetchBalance:nil];
    ledger->SetBalanceReportItem(BATGetPublisherMonth(now),
                                 BATGetPublisherYear(now),
                                 report_type,
                                 grant->probi);
  }

  [self clearNotificationWithID:[self notificationIDForGrant:std::move(grant)]];
  for (BATBraveLedgerObserver *observer in self.observers) {
    if (observer.balanceReportUpdated) {
      observer.balanceReportUpdated();
    }
    if (observer.grantClaimed) {
      observer.grantClaimed(bridgedGrant);
    }
  }
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
  ledger->GetBalanceReport((ledger::ACTIVITY_MONTH)month, year, ^(bool result, ledger::BalanceReportInfoPtr info) {
    auto bridgedInfo = info.get() != nullptr ? [[BATBalanceReportInfo alloc] initWithBalanceReportInfo:*info.get()] : nil;
    completion(result ? bridgedInfo : nil);
  });
}

- (BATAutoContributeProps *)autoContributeProps
{
  ledger::AutoContributePropsPtr props = ledger->GetAutoContributeProps();
  return [[BATAutoContributeProps alloc] initWithAutoContributePropsPtr:std::move(props)];
}

#pragma mark - Reconcile

- (void)onReconcileComplete:(ledger::Result)result viewingId:(const std::string &)viewing_id category:(const ledger::RewardsCategory)category probi:(const std::string &)probi
{
  if (result == ledger::Result::LEDGER_OK) {
    const auto now = [NSDate date];
    const auto nowTimestamp = [now timeIntervalSince1970];

    if (category == ledger::RewardsCategory::RECURRING_TIP) {
      [self showTipsProcessedNotificationIfNeccessary];
    }
    [self fetchBalance:nil];

    ledger->OnReconcileCompleteSuccess(viewing_id,
                                       category,
                                       probi,
                                       BATGetPublisherMonth(now),
                                       BATGetPublisherYear(now),
                                       nowTimestamp);
  }

  if ((result == ledger::Result::LEDGER_OK && category == ledger::RewardsCategory::AUTO_CONTRIBUTE) ||
      result == ledger::Result::LEDGER_ERROR ||
      result == ledger::Result::NOT_ENOUGH_FUNDS ||
      result == ledger::Result::TIP_ERROR) {

    const auto viewingId = [NSString stringWithUTF8String:viewing_id.c_str()];
    const auto info = @{ @"viewingId": viewingId,
                         @"result": @((BATResult)result),
                         @"category": @((BATRewardsCategory)category),
                         @"amount": [NSString stringWithUTF8String:probi.c_str()] };

    [self addNotificationOfKind:BATRewardsNotificationKindAutoContribute
                       userInfo:info
                 notificationID:[NSString stringWithFormat:@"contribution_%@", viewingId]];
  }

  for (BATBraveLedgerObserver *observer in self.observers) {
    if (observer.balanceReportUpdated) {
      observer.balanceReportUpdated();
    }
    if (observer.reconcileCompleted) {
      observer.reconcileCompleted(static_cast<BATResult>(result),
                                  [NSString stringWithUTF8String:viewing_id.c_str()],
                                  static_cast<BATRewardsCategory>(category),
                                  [NSString stringWithUTF8String:probi.c_str()]);
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
  ledger->OnForeground(self.selectedTabId, [[NSDate date] timeIntervalSince1970]);

  // Check if the last notification check was more than a day ago
  if (fabs([self.lastNotificationCheckDate timeIntervalSinceNow]) > kOneDay) {
    [self checkForNotificationsAndFetchGrants];
  }
}

- (void)applicationDidBackground
{
  ledger->OnBackground(self.selectedTabId, [[NSDate date] timeIntervalSince1970]);
}

- (void)reportLoadedPageWithURL:(NSURL *)url tabId:(UInt32)tabId
{
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
  ledger->OnUnload(tabId, [[NSDate date] timeIntervalSince1970]);
}

#pragma mark - Preferences

BATLedgerReadonlyBridge(BOOL, isEnabled, GetRewardsMainEnabled)

- (void)setEnabled:(BOOL)enabled
{
  ledger->SetRewardsMainEnabled(enabled);

  for (BATBraveLedgerObserver *observer in self.observers) {
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
  return ((NSString *)self.prefs[key]).UTF8String;
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

#pragma mark - Ads & Confirmations

- (void)confirmAd:(NSString *)info
{
  ledger->ConfirmAd(info.UTF8String);
}

- (void)confirmAction:(NSString *)uuid creativeSetID:(NSString *)creativeSetID type:(NSString *)type
{
  ledger->ConfirmAction(uuid.UTF8String, creativeSetID.UTF8String, type.UTF8String);
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
  for (BATBraveLedgerObserver *observer in self.observers) {
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
  
  for (BATBraveLedgerObserver *observer in self.observers) {
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
  
  for (BATBraveLedgerObserver *observer in self.observers) {
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
  [self fetchAvailableGrantsForLanguage:@"" paymentId:@""];
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
  for (BATBraveLedgerObserver *observer in self.observers) {
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
      NSLog(@"Failed to unarchive notifications on disk: %@", error);
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
      NSLog(@"Failed to write notifications to disk: %@", error);
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

- (void)saveLedgerState:(const std::string &)ledger_state handler:(ledger::LedgerCallbackHandler *)handler
{
  const auto result = [self.commonOps saveContents:ledger_state name:"ledger_state.json"];
  handler->OnLedgerStateSaved(result ? ledger::Result::LEDGER_OK : ledger::Result::NO_LEDGER_STATE);
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

- (void)savePublisherState:(const std::string &)publisher_state handler:(ledger::LedgerCallbackHandler *)handler
{
  const auto result = [self.commonOps saveContents:publisher_state name:"publisher_state.json"];
  handler->OnPublisherStateSaved(result ? ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR);
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

- (void)resetState:(const std::string &)name callback:(ledger::OnResetCallback)callback
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

- (void)saveState:(const std::string &)name value:(const std::string &)value callback:(ledger::OnSaveCallback)callback
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

- (void)loadURL:(const std::string &)url headers:(const std::vector<std::string> &)headers content:(const std::string &)content contentType:(const std::string &)contentType method:(const ledger::URL_METHOD)method callback:(ledger::LoadURLCallback)callback
{
  std::map<ledger::URL_METHOD, std::string> methodMap {
    {ledger::GET, "GET"},
    {ledger::POST, "POST"},
    {ledger::PUT, "PUT"}
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

- (std::string)generateGUID
{
  return [self.commonOps generateUUID];
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
  uint32_t next_record = 0;
  if (publishers.count == limit) {
    next_record = start + limit + 1;
  }

  callback(VectorFromNSArray(publishers, ^ledger::PublisherInfoPtr(BATPublisherInfo *info){
    return info.cppObjPtr;
  }), next_record);
}

- (void)getActivityInfoList:(uint32_t)start limit:(uint32_t)limit filter:(ledger::ActivityInfoFilterPtr)filter callback:(ledger::PublisherInfoListCallback)callback
{
  const auto filter_ = [[BATActivityInfoFilter alloc] initWithActivityInfoFilter:*filter];
  const auto publishers = [BATLedgerDatabase publishersWithActivityFromOffset:start limit:limit filter:filter_];

  [self handlePublisherListing:publishers start:start limit:limit callback:callback];
}

- (void)getOneTimeTips:(ledger::PublisherInfoListCallback)callback
{
  const auto now = [NSDate date];
  const auto publishers = [BATLedgerDatabase oneTimeTipsPublishersForMonth: (BATActivityMonth)BATGetPublisherMonth(now)
                                                        year:BATGetPublisherYear(now)];

  [self handlePublisherListing:publishers start:0 limit:0 callback:callback];
}

- (void)saveRecurringTip:(ledger::ContributionInfoPtr)info callback:(ledger::SaveRecurringTipCallback)callback
{
  [BATLedgerDatabase insertOrUpdateRecurringTipWithPublisherID:[NSString stringWithUTF8String:info->publisher.c_str()]
                                                        amount:info->value
                                                     dateAdded:info->date
                                                    completion:^(BOOL success) {
                                                      if (!success) {
                                                        callback(ledger::Result::LEDGER_ERROR);
                                                        return;
                                                      }
                                                      callback(ledger::Result::LEDGER_OK);
                                                    }];
}

- (void)getRecurringTips:(ledger::PublisherInfoListCallback)callback
{
  const auto publishers = [BATLedgerDatabase recurringTips];

  [self handlePublisherListing:publishers start:0 limit:0 callback:callback];
}

- (void)loadActivityInfo:(ledger::ActivityInfoFilterPtr)filter
                callback:(ledger::PublisherInfoCallback)callback
{
  const auto filter_ = [[BATActivityInfoFilter alloc] initWithActivityInfoFilter:*filter];
  // set limit to 2 to make sure there is only 1 valid result for the filter
  const auto publishers = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:2 filter:filter_];

  [self handlePublisherListing:publishers start:0 limit:2 callback:^(const ledger::PublisherInfoList& list, uint32_t) {
    // activity info not found
    if (list.size() == 0) {
      // we need to try to get at least publisher info in this case
      // this way we preserve publisher info
      const auto publisherID = filter_.id;
      const auto info = [BATLedgerDatabase publisherInfoWithPublisherID:publisherID];
      if (info) {
        callback(ledger::Result::LEDGER_OK, info.cppObjPtr);
      } else {
        // This part diverges from brave-core. brave-core actually goes into an infinite loop here?
        // Hope im missing something on their side where they don't even call this method unless
        // there's a publisher with that ID in the ActivityInfo and PublisherInfo table...
        callback(ledger::Result::NOT_FOUND, nullptr);
      }
    } else if (list.size() > 1) {
      callback(ledger::Result::TOO_MANY_RESULTS, ledger::PublisherInfoPtr());
    } else {
      callback(ledger::Result::LEDGER_OK, list[0].Clone());
    }
  }];
}

- (void)loadMediaPublisherInfo:(const std::string &)media_key
                      callback:(ledger::PublisherInfoCallback)callback
{
  const auto mediaKey = [NSString stringWithUTF8String:media_key.c_str()];
  const auto publisher = [BATLedgerDatabase mediaPublisherInfoWithMediaKey:mediaKey];
  if (publisher) {
    callback(ledger::Result::LEDGER_OK, publisher.cppObjPtr);
  } else {
    callback(ledger::Result::NOT_FOUND, nullptr);
  }
}

- (void)loadPanelPublisherInfo:(ledger::ActivityInfoFilterPtr)filter callback:(ledger::PublisherInfoCallback)callback
{
  const auto filter_ = [[BATActivityInfoFilter alloc] initWithActivityInfoFilter:*filter];
  const auto publisher = [BATLedgerDatabase panelPublisherWithFilter:filter_];
  if (publisher) {
    callback(ledger::Result::LEDGER_OK, publisher.cppObjPtr);
  } else {
    callback(ledger::Result::NOT_FOUND, nullptr);
  }
}

- (void)loadPublisherInfo:(const std::string &)publisher_key callback:(ledger::PublisherInfoCallback)callback
{
  const auto publisherID = [NSString stringWithUTF8String:publisher_key.c_str()];
  const auto publisher = [BATLedgerDatabase publisherInfoWithPublisherID:publisherID];
  if (publisher) {
    callback(ledger::Result::LEDGER_OK, publisher.cppObjPtr);
  } else {
    callback(ledger::Result::NOT_FOUND, nullptr);
  }
}

- (void)removeRecurringTip:(const std::string &)publisher_key callback:(ledger::RemoveRecurringTipCallback)callback
{
  const auto publisherID = [NSString stringWithUTF8String:publisher_key.c_str()];
  [BATLedgerDatabase removeRecurringTipWithPublisherID:publisherID completion:^(BOOL success) {
    callback(success ? ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR);
  }];
}

- (void)restorePublishers:(ledger::RestorePublishersCallback)callback
{
  [BATLedgerDatabase restoreExcludedPublishers:^(BOOL success) {
    const auto result = success ? ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR;
    callback(result);
  }];
}

- (void)saveActivityInfo:(ledger::PublisherInfoPtr)publisher_info callback:(ledger::PublisherInfoCallback)callback
{
  const auto* info = publisher_info.get();
  if (info != nullptr) {
    const auto publisher = [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
    [BATLedgerDatabase insertOrUpdateActivityInfoFromPublisher:publisher completion:^(BOOL success) {
      if (success) {
        callback(ledger::Result::LEDGER_OK, publisher.cppObjPtr);
      } else {
        callback(ledger::Result::LEDGER_ERROR, nullptr);
      }
    }];
  } else {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
  }
}

- (void)saveContributionInfo:(const std::string &)probi month:(const int)month year:(const int)year date:(const uint32_t)date publisherKey:(const std::string &)publisher_key category:(const ledger::RewardsCategory)category
{
  [BATLedgerDatabase insertContributionInfo:[NSString stringWithUTF8String:probi.c_str()]
                                      month:month
                                       year:year
                                       date:date
                               publisherKey:[NSString stringWithUTF8String:publisher_key.c_str()]
                                   category:(BATRewardsCategory)category
                                 completion:^(BOOL success) {
                                   for (BATBraveLedgerObserver *observer in self.observers) {
                                     if (observer.contributionAdded) {
                                       observer.contributionAdded(success,
                                                                  static_cast<BATRewardsCategory>(category));
                                     }
                                   }
                                 }];
}

- (void)saveMediaPublisherInfo:(const std::string &)media_key publisherId:(const std::string &)publisher_id
{
  [BATLedgerDatabase insertOrUpdateMediaPublisherInfoWithMediaKey:[NSString stringWithUTF8String:media_key.c_str()]
                                                      publisherID:[NSString stringWithUTF8String:publisher_id.c_str()]
                                                       completion:nil];
}

- (void)saveNormalizedPublisherList:(ledger::PublisherInfoList)normalized_list
{
  const auto list = NSArrayFromVector(&normalized_list, ^BATPublisherInfo *(const ledger::PublisherInfoPtr& info) {
    return [[BATPublisherInfo alloc] initWithPublisherInfo:*info];
  });
  [BATLedgerDatabase insertOrUpdateActivitiesInfoFromPublishers:list completion:^(BOOL success) {
    if (!success) {
      return;
    }
    for (BATBraveLedgerObserver *observer in self.observers) {
      if (observer.publisherListNormalized) {
        observer.publisherListNormalized(list);
      }
    }
  }];
}

- (void)savePendingContribution:(ledger::PendingContributionList)list callback:(ledger::SavePendingContributionCallback)callback
{
  const auto list_ = NSArrayFromVector(&list, ^BATPendingContribution *(const ledger::PendingContributionPtr& info) {
    return [[BATPendingContribution alloc] initWithPendingContribution:*info];
  });
  [BATLedgerDatabase insertPendingContributions:list_ completion:^(BOOL success) {
    if (!success) {
      callback(ledger::Result::LEDGER_ERROR);
      return;
    }
    for (BATBraveLedgerObserver *observer in self.observers) {
      if (observer.pendingContributionAdded) {
        for (BATPendingContribution *pc in list_) {
          observer.pendingContributionAdded(pc.publisherKey);
        }
      }
    }
    callback(ledger::Result::LEDGER_OK);
  }];
}

- (void)savePublisherInfo:(ledger::PublisherInfoPtr)publisher_info callback:(ledger::PublisherInfoCallback)callback
{
  if (publisher_info.get() != nullptr) {
    const auto publisher = [[BATPublisherInfo alloc] initWithPublisherInfo:*publisher_info];
    [BATLedgerDatabase insertOrUpdatePublisherInfo:publisher completion:^(BOOL success) {
      callback(ledger::Result::LEDGER_OK, publisher.cppObjPtr);
    }];
  } else {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
  }
}

- (void)getPendingContributions:(ledger::PendingContributionInfoListCallback)callback
{
  const auto pendingContributions = [BATLedgerDatabase pendingContributions];
  callback(VectorFromNSArray(pendingContributions, ^ledger::PendingContributionInfoPtr(BATPendingContributionInfo *info){
    return info.cppObjPtr;
  }));
}

- (void)getPendingContributionsTotal:(ledger::PendingContributionsTotalCallback)callback
{
  callback([BATLedgerDatabase reservedAmountForPendingContributions]);
}

- (void)onPanelPublisherInfo:(ledger::Result)result publisherInfo:(ledger::PublisherInfoPtr)publisher_info windowId:(uint64_t)windowId
{
  if (publisher_info.get() == nullptr || result != ledger::Result::LEDGER_OK) {
    return;
  }
  auto info = [[BATPublisherInfo alloc] initWithPublisherInfo:*publisher_info];
  for (BATBraveLedgerObserver *observer in self.observers) {
    if (observer.fetchedPanelPublisher) {
      observer.fetchedPanelPublisher(info, windowId);
    }
  }
}

- (void)removeAllPendingContributions:(ledger::RemovePendingContributionCallback)callback
{
  const auto pendingContributions = [BATLedgerDatabase pendingContributions];
  const auto keys = [[NSMutableArray alloc] init];
  for (BATPendingContributionInfo *info in pendingContributions) {
    [keys addObject:info.publisherKey];
  }

  [BATLedgerDatabase removeAllPendingContributions:^(BOOL success) {
    callback(success ? ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR);
    if (success) {
      for (BATBraveLedgerObserver *observer in self.observers) {
        if (observer.pendingContributionsRemoved) {
          observer.pendingContributionsRemoved(keys);
        }
      }
    }
  }];
}

- (void)removePendingContribution:(const std::string &)publisher_key viewingId:(const std::string &)viewing_id addedDate:(uint64_t)added_date callback:(ledger::RemovePendingContributionCallback)callback
{
  const auto publisherID = [NSString stringWithUTF8String:publisher_key.c_str()];
  const auto viewingID = [NSString stringWithUTF8String:viewing_id.c_str()];
  [BATLedgerDatabase removePendingContributionForPublisherID:publisherID
                                                   viewingID:viewingID
                                                   addedDate:added_date
                                                  completion:^(BOOL success) {
                                                    callback(success ? ledger::Result::LEDGER_OK :
                                                             ledger::Result::LEDGER_ERROR);
                                                    for (BATBraveLedgerObserver *observer in self.observers) {
                                                      if (observer.pendingContributionsRemoved) {
                                                        observer.pendingContributionsRemoved(@[publisherID]);
                                                      }
                                                    }
                                                  }];
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
      for (BATBraveLedgerObserver *observer in self.observers) {
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

- (void)getExternalWallets:(ledger::GetExternalWalletsCallback)callback
{
  // For uphold wallets (not implemented)
  std::map<std::string, ledger::ExternalWalletPtr> wallets;
  callback(std::move(wallets));
}

- (void)saveExternalWallet:(const std::string &)wallet_type wallet:(ledger::ExternalWalletPtr)wallet
{
  // For uphold wallets (not implemented)
}

- (void)showNotification:(const std::string &)type args:(const std::vector<std::string>&)args callback:(ledger::ShowNotificationCallback)callback
{
  // TODO: Add notifications
}

- (void)getServerPublisherInfo:(const std::string &)publisher_key callback:(ledger::GetServerPublisherInfoCallback)callback
{
  const auto publisherID = [NSString stringWithUTF8String:publisher_key.c_str()];
  const auto info = [BATLedgerDatabase serverPublisherInfoWithPublisherID:publisherID];
  if (!info) {
    callback(nullptr);
    return;
  }
  callback(info.cppObjPtr);
}

- (void)clearAndInsertServerPublisherList:(ledger::ServerPublisherInfoList)list callback:(ledger::ClearAndInsertServerPublisherListCallback)callback
{
  if (self.loadingPublisherList) {
    return;
  }
  const auto list_ = NSArrayFromVector(&list, ^BATServerPublisherInfo *(const ledger::ServerPublisherInfoPtr& info) {
    return [[BATServerPublisherInfo alloc] initWithServerPublisherInfo:*info];
  });
  self.loadingPublisherList = YES;
  [BATLedgerDatabase clearAndInsertList:list_ completion:^(BOOL success) {
    self.loadingPublisherList = NO;
    callback(success ? ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR);
    
    for (BATBraveLedgerObserver *observer in self.observers) {
      if (observer.publisherListUpdated) {
        observer.publisherListUpdated();
      }
    }
  }];
}

- (void)setTransferFee:(const std::string &)wallet_type transfer_fee:(ledger::TransferFeePtr)transfer_fee
{
  // FIXME: Add implementation
}

- (ledger::TransferFeeList)getTransferFees:(const std::string &)wallet_type
{
  // FIXME: Add implementation
  ledger::TransferFeeList list;
  return list;
}

- (void)removeTransferFee:(const std::string &)wallet_type id:(const std::string &)id
{
  // FIXME: Add implementation
}

@end
