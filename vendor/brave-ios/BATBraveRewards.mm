// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "BATBraveRewards.h"
#import "DataController.h"
#import "RewardsLogging.h"

#include "base/task/current_thread.h"
#include "base/task/single_thread_task_executor.h"

base::SingleThreadTaskExecutor* g_task_executor = nullptr;

@implementation BATBraveRewardsConfiguration

+ (BATBraveRewardsConfiguration *)defaultConfiguration
{
  __auto_type config = [[BATBraveRewardsConfiguration alloc] init];
  config.environment = BATEnvironmentDevelopment;
  config.stateStoragePath = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject;
  return config;
}

+ (BATBraveRewardsConfiguration *)stagingConfiguration
{
  __auto_type config = [self defaultConfiguration];
  config.environment = BATEnvironmentStaging;
  return config;
}

+ (BATBraveRewardsConfiguration *)productionConfiguration
{
  __auto_type config = [self defaultConfiguration];
  config.environment = BATEnvironmentProduction;
  return config;
}

+ (BATBraveRewardsConfiguration *)testingConfiguration
{
  __auto_type config = [self defaultConfiguration];
  config.stateStoragePath = NSTemporaryDirectory();
  config.testing = YES;
  config.useShortRetries = YES;
  config.overridenNumberOfSecondsBetweenReconcile = 30;
  return config;
}

- (id)copyWithZone:(NSZone *)zone
{
  __auto_type config = [[BATBraveRewardsConfiguration alloc] init];
  config.stateStoragePath = self.stateStoragePath;
  config.environment = self.environment;
  config.testing = self.testing;
  config.useShortRetries = self.useShortRetries;
  config.overridenNumberOfSecondsBetweenReconcile = self.overridenNumberOfSecondsBetweenReconcile;
  return config;
}

@end

@interface BATBraveRewards ()
@property (nonatomic) BATBraveAds *ads;
@property (nonatomic) BATBraveLedger *ledger;
@property (nonatomic, copy) BATBraveRewardsConfiguration *configuration;
@property (nonatomic, assign) Class ledgerClass;
@property (nonatomic, assign) Class adsClass;
@end

@implementation BATBraveRewards

- (void)reset
{
  [[NSFileManager defaultManager] removeItemAtPath:[self.configuration.stateStoragePath stringByAppendingPathComponent:@"ledger"] error:nil];
  [[NSFileManager defaultManager] removeItemAtPath:[self.configuration.stateStoragePath stringByAppendingPathComponent:@"ads"] error:nil];
  if (DataController.defaultStoreExists) {
    [[NSFileManager defaultManager] removeItemAtURL:DataController.shared.storeDirectoryURL error:nil];
    DataController.shared = [[DataController alloc] init];
  }

  [self setupLedgerAndAds];
}

- (instancetype)initWithConfiguration:(BATBraveRewardsConfiguration *)configuration
{
  return [self initWithConfiguration:configuration delegate:nil ledgerClass:nil adsClass:nil];
}

- (instancetype)initWithConfiguration:(BATBraveRewardsConfiguration *)configuration
                             delegate:(nullable id<BATBraveRewardsDelegate>)delegate
                          ledgerClass:(nullable Class)ledgerClass
                             adsClass:(nullable Class)adsClass
{
  if ((self = [super init])) {
    if (!base::CurrentThread::Get()) {
      g_task_executor = new base::SingleThreadTaskExecutor(base::MessagePumpType::UI);
    }

    rewards::set_rewards_client_for_logging(self);
    
    self.configuration = configuration;
    self.delegate = delegate;
    self.ledgerClass = ledgerClass ?: BATBraveLedger.class;
    self.adsClass = adsClass ?: BATBraveAds.class;

    BATBraveAds.debug = configuration.environment != BATEnvironmentProduction;
    BATBraveAds.environment = configuration.environment;
    if (configuration.buildChannel != nil) {
      BATBraveAds.buildChannel = configuration.buildChannel;
    }

    BATBraveLedger.debug = configuration.environment != BATEnvironmentProduction;
    BATBraveLedger.environment = configuration.environment;
    BATBraveLedger.testing = configuration.testing;
    BATBraveLedger.useShortRetries = configuration.useShortRetries;
    BATBraveLedger.reconcileInterval = configuration.overridenNumberOfSecondsBetweenReconcile;
    
    [self setupLedgerAndAds];
  }
  return self;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-messaging-id"

- (void)setupLedgerAndAds
{
  NSString *adsStorage = [self.configuration.stateStoragePath stringByAppendingPathComponent:@"ads"];
  self.ads = [[self.adsClass alloc] initWithStateStoragePath:adsStorage];
  NSString *ledgerStorage = [self.configuration.stateStoragePath stringByAppendingPathComponent:@"ledger"];
  self.ledger = [[self.ledgerClass alloc] initWithStateStoragePath:ledgerStorage];
  __auto_type __weak weakSelf = self;
  self.ads.ledger = self.ledger;
  self.ledger.ads = self.ads;
  self.ledger.faviconFetcher = ^(NSURL *pageURL, void (^completion)(NSURL * _Nullable)) {
    [weakSelf.delegate faviconURLFromPageURL:pageURL completion:completion];
  };
}

#pragma clang diagnostic push

@end

@implementation BATBraveRewards (Reporting)

- (void)reportTabUpdated:(NSInteger)tabId
                     url:(NSURL *)url
              faviconURL:(nullable NSURL *)faviconURL
              isSelected:(BOOL)isSelected
               isPrivate:(BOOL)isPrivate
{
  if (isSelected) {
    self.ledger.selectedTabId = (UInt32)tabId;
    [self onTabRetrieved:tabId url:url faviconURL:faviconURL html:nil];
  }
  [self.ads reportTabUpdated:tabId url:url isSelected:isSelected isPrivate:isPrivate];
}

- (void)reportLoadedPageWithURL:(NSURL *)url
                     faviconURL:(nullable NSURL *)faviconURL
                          tabId:(UInt32)tabId
                           html:(NSString *)html
                   adsInnerText:(nullable NSString *)adsInnerText
{
  [self onTabRetrieved:tabId url:url faviconURL:faviconURL html:html];
  if (adsInnerText != nil) {
    [self.ads reportLoadedPageWithURL:url innerText:adsInnerText tabId:tabId];
  }
  [self.ledger reportLoadedPageWithURL:url tabId:tabId];
}

- (void)onTabRetrieved:(NSInteger)tabId url:(NSURL *)url faviconURL:(nullable NSURL *)faviconURL html:(nullable NSString *)html
{
  // Check for private mode should be done on client side.
  if (!self.ledger.walletCreated || !self.ledger.isEnabled) { return; }
  
  // New publisher database entry will be created if the pub doesn't exist.
  [self.ledger fetchPublisherActivityFromURL:url faviconURL:faviconURL publisherBlob:html tabId:tabId];
}

- (void)reportXHRLoad:(NSURL *)url tabId:(UInt32)tabId firstPartyURL:(NSURL *)firstPartyURL referrerURL:(NSURL *)referrerURL
{
  [self.ledger reportXHRLoad:url tabId:tabId firstPartyURL:firstPartyURL referrerURL:referrerURL];
}

- (void)reportPostData:(NSData *)postData url:(NSURL *)url tabId:(UInt32)tabId firstPartyURL:(NSURL *)firstPartyURL referrerURL:(NSURL *)referrerURL
{
  [self.ledger reportPostData:postData url:url tabId:tabId firstPartyURL:firstPartyURL referrerURL:referrerURL];
}

- (void)reportTabNavigationWithTabId:(UInt32)tabId
{
  [self.ledger reportTabNavigationOrClosedWithTabId:tabId];
}

- (void)reportTabClosedWithTabId:(UInt32)tabId
{
  [self.ads reportTabClosedWithTabId:tabId];
  [self.ledger reportTabNavigationOrClosedWithTabId:tabId];
}

- (void)reportMediaStartedWithTabId:(UInt32)tabId
{
  [self.ads reportMediaStartedWithTabId:tabId];
}

- (void)reportMediaStoppedWithTabId:(UInt32)tabId
{
  [self.ads reportMediaStoppedWithTabId:tabId];
}

@end
