// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "BATBraveRewards.h"
#import "DataController.h"

@implementation BATBraveRewardsConfiguration

+ (BATBraveRewardsConfiguration *)defaultConfiguration
{
  __auto_type config = [[BATBraveRewardsConfiguration alloc] init];
  config.stateStoragePath = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject;
  return config;
}

+ (BATBraveRewardsConfiguration *)productionConfiguration
{
  __auto_type config = [self defaultConfiguration];
  config.production = YES;
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
  config.production = self.production;
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
  [[NSFileManager defaultManager] removeItemAtPath:self.configuration.stateStoragePath error:nil];
  [[NSFileManager defaultManager] removeItemAtURL:DataController.shared.storeDirectoryURL error:nil];
  DataController.shared = [[DataController alloc] init];

  [self setupLedgerAndAds];
}

- (instancetype)initWithConfiguration:(BATBraveRewardsConfiguration *)configuration
{
  return [self initWithConfiguration:configuration ledgerClass:nil adsClass:nil];
}

- (instancetype)initWithConfiguration:(BATBraveRewardsConfiguration *)configuration ledgerClass:(nullable Class)ledgerClass adsClass:(nullable Class)adsClass
{
  if ((self = [super init])) {
    self.configuration = configuration;
    self.ledgerClass = ledgerClass ?: BATBraveLedger.class;
    self.adsClass = adsClass ?: BATBraveAds.class;

    BATBraveAds.debug = !configuration.production;
    BATBraveAds.production = configuration.production;
    BATBraveAds.testing = configuration.testing;

    BATBraveLedger.debug = !configuration.production;
    BATBraveLedger.production = configuration.production;
    BATBraveLedger.testing = configuration.testing;
    BATBraveLedger.useShortRetries = configuration.useShortRetries;
    BATBraveLedger.reconcileTime = configuration.overridenNumberOfSecondsBetweenReconcile;
    
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
  self.ledger.ads = self.ads;
}

#pragma clang diagnostic push

@end

@implementation BATBraveRewards (Reporting)

- (void)reportTabUpdated:(NSInteger)tabId
                     url:(NSURL *)url
              isSelected:(BOOL)isSelected
               isPrivate:(BOOL)isPrivate
              completion:(void (^)(BOOL verifiedPublisher))completion
{
  if (isSelected) {
    self.ledger.selectedTabId = (UInt32)tabId;
  }

  [self onTabRetrieved:tabId url:url];
  [self.ads reportTabUpdated:tabId url:url isSelected:isSelected isPrivate:isPrivate];
  
  if (tabId == self.ledger.selectedTabId) {
    [self.ledger publisherInfoForId:url.host completion:^(BATPublisherInfo * _Nullable info) {
      completion(info.verified);
    }];
  } else {
    completion(NO);
  }
}

- (void)reportLoadedPageWithURL:(NSURL *)url
                          tabId:(UInt32)tabId
                           html:(NSString *)html
                     completion:(void (^)(BOOL verifiedPublisher))completion
{
  [self onTabRetrieved:tabId url:url];
  [self.ads reportLoadedPageWithURL:url html:html];
  [self.ledger reportLoadedPageWithURL:url tabId:tabId];
  
  [self.ledger publisherInfoForId:url.host completion:^(BATPublisherInfo * _Nullable info) {
    completion(info.verified);
  }];
}

- (void)onTabRetrieved:(NSInteger)tabId url:(NSURL *)url
{
  // Check for private mode should be done on client side.
  if (!self.ledger.walletCreated || !self.ledger.isEnabled) { return; }
  
  // New publisher database entry will be created if the pub doesn't exist.
  [self.ledger publisherActivityFromURL:url faviconURL:nil publisherBlob:nil];
}

- (void)reportXHRLoad:(NSURL *)url tabId:(UInt32)tabId firstPartyURL:(NSURL *)firstPartyURL referrerURL:(NSURL *)referrerURL
{
  [self.ledger reportXHRLoad:url tabId:tabId firstPartyURL:firstPartyURL referrerURL:referrerURL];
}

- (void)reportPostData:(NSData *)postData url:(NSURL *)url tabId:(UInt32)tabId firstPartyURL:(NSURL *)firstPartyURL referrerURL:(NSURL *)referrerURL
{
  [self.ledger reportPostData:postData url:url tabId:tabId firstPartyURL:firstPartyURL referrerURL:referrerURL];
}

- (void)reportTabClosedWithTabId:(UInt32)tabId
{
  [self.ads reportTabClosedWithTabId:tabId];
  [self.ledger reportTabClosedWithTabId:tabId];
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
