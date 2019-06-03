// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

#import "BATBraveAds.h"
#import "BATBraveLedger.h"

NS_ASSUME_NONNULL_BEGIN

/// Configuration around brave rewards for ads & ledger
NS_SWIFT_NAME(BraveRewardsConfiguration)
@interface BATBraveRewardsConfiguration : NSObject <NSCopying>

/// Whether or not rewards is being tested
@property (nonatomic, getter=isTesting) BOOL testing;
/// Whether or not rewards is in production
@property (nonatomic, getter=isProduction) BOOL production;
/// Where ledger and ads should save their state
@property (nonatomic, copy) NSString *stateStoragePath;
/// The number of seconds between overrides. Defaults to 0 (no override) which means reconciles
/// occur every 30 days (see: bat-native-ledger/static_values.h/_reconcile_default_interval)
@property (nonatomic) int overridenNumberOfSecondsBetweenReconcile;
/// Whether or not to enable short retries between contribution attempts
@property (nonatomic) BOOL useShortRetries;

/// The default configuration. Channel is debug, no changes to ledger configuration
///
/// State is stored in Application Support
@property (nonatomic, class, readonly) BATBraveRewardsConfiguration *defaultConfiguration NS_SWIFT_NAME(default);
/// The production configuration. Channel is production, no changes to ledger configuration
///
/// State is stored in Application Support
@property (nonatomic, class, readonly) BATBraveRewardsConfiguration *productionConfiguration NS_SWIFT_NAME(production);
/// The testing configuration. Channel is debug & testing. Short retries are enabled, number of
/// seconds between reconciles is set to 30 seconds instead of 30 days.
///
/// State is saved to a directory created in /tmp
@property (nonatomic, class, readonly) BATBraveRewardsConfiguration *testingConfiguration NS_SWIFT_NAME(testing);

@end

/// A container for handling Brave Rewards. Use `ads` to handle how many ads the users see,
/// when to display them. Use `ledger` to manage interactions between the users wallet & publishers
NS_SWIFT_NAME(BraveRewards)
@interface BATBraveRewards : NSObject

@property (nonatomic, readonly) BATBraveAds *ads;
@property (nonatomic, readonly) BATBraveLedger *ledger;

/// Resets the ads & ledger (by purging its data). This should likely never be used in production.
- (void)reset;

/// Create a BraveRewards instance with a given configuration
- (instancetype)initWithConfiguration:(BATBraveRewardsConfiguration *)configuration;
/// Create a BraveRewards instance with a given configuration and custom ledger classes for mocking
- (instancetype)initWithConfiguration:(BATBraveRewardsConfiguration *)configuration
                          ledgerClass:(nullable Class)ledgerClass
                             adsClass:(nullable Class)adsClass NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@end

@interface BATBraveRewards (Reporting)

/// Report that a tab with a given id was updated
- (void)reportTabUpdated:(NSInteger)tabId url:(NSURL *)url isSelected:(BOOL)isSelected isPrivate:(BOOL)isPrivate;
/// Report that a page has loaded in the current browser tab, and the HTML is available for analysis
- (void)reportLoadedPageWithURL:(NSURL *)url tabId:(UInt32)tabId html:(NSString *)html NS_SWIFT_NAME(reportLoadedPage(url:tabId:html:));
/// Report any XHR load happening in the page
- (void)reportXHRLoad:(NSURL *)url
                tabId:(UInt32)tabId
        firstPartyURL:(NSURL *)firstPartyURL
          referrerURL:(nullable NSURL *)referrerURL NS_SWIFT_NAME(reportXHRLoad(url:tabId:firstPartyURL:referrerURL:));
/// Report posting data to a form?
- (void)reportPostData:(NSData *)postData
                   url:(NSURL *)url
                 tabId:(UInt32)tabId
         firstPartyURL:(NSURL *)firstPartyURL
           referrerURL:(nullable NSURL *)referrerURL NS_SWIFT_NAME(reportPostData(_:url:tabId:firstPartyURL:referrerURL:));;
/// Report that media has started on a tab with a given id
- (void)reportMediaStartedWithTabId:(UInt32)tabId NS_SWIFT_NAME(reportMediaStarted(tabId:));
/// Report that media has stopped on a tab with a given id
- (void)reportMediaStoppedWithTabId:(UInt32)tabId NS_SWIFT_NAME(reportMediaStopped(tabId:));
/// Report that a tab with a given id was closed by the user
- (void)reportTabClosedWithTabId:(UInt32)tabId NS_SWIFT_NAME(reportTabClosed(tabId:));

@end

NS_ASSUME_NONNULL_END
