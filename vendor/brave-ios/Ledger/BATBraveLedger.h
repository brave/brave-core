/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "Records.h"
#import "ledger.mojom.objc.h"
#import "BATRewardsNotification.h"
#import "BATBraveLedgerObserver.h"

@class BATBraveAds;

NS_ASSUME_NONNULL_BEGIN

typedef void (^BATFaviconFetcher)(NSURL *pageURL, void (^completion)(NSURL * _Nullable faviconURL));

/// The error domain for ledger related errors
extern NSString * const BATBraveLedgerErrorDomain NS_SWIFT_NAME(BraveLedgerErrorDomain);

extern NSNotificationName const BATBraveLedgerNotificationAdded NS_SWIFT_NAME(BraveLedger.NotificationAdded);

NS_SWIFT_NAME(BraveLedger)
@interface BATBraveLedger : NSObject

@property (nonatomic, weak) BATBraveAds *ads;

@property (nonatomic, copy, nullable) BATFaviconFetcher faviconFetcher;

/// Create a brave ledger that will read and write its state to the given path
- (instancetype)initWithStateStoragePath:(NSString *)path;

- (instancetype)init NS_UNAVAILABLE;

#pragma mark - Observers

/// Add an interface to the list of observers
///
/// Observers are stored weakly and do not necessarily need to be removed
- (void)addObserver:(BATBraveLedgerObserver *)observer;

/// Removes an interface from the list of observers
- (void)removeObserver:(BATBraveLedgerObserver *)observer;

#pragma mark - Global

/// Whether or not to use staging servers. Defaults to false
@property (nonatomic, class, getter=isDebug) BOOL debug;
/// Whether or not to use production servers. Defaults to true
@property (nonatomic, class, getter=isProduction) BOOL production;
/// Marks if this is being ran in a test environment. Defaults to false
@property (nonatomic, class, getter=isTesting) BOOL testing;
/// Number of minutes between reconciles override. Defaults to 0 (no override)
@property (nonatomic, class) int reconcileTime;
/// Whether or not to use short contribution retries. Defaults to false
@property (nonatomic, class) BOOL useShortRetries;

#pragma mark - Wallet

/// Whether or not the wallet has been created
@property (nonatomic, readonly, getter=isWalletCreated) BOOL walletCreated;

/// Creates a cryptocurrency wallet
- (void)createWallet:(nullable void (^)(NSError * _Nullable error))completion;

/// Fetch details about the users wallet (if they have one) and assigns it to `walletInfo`
- (void)fetchWalletDetails:(nullable void (^)(BATWalletProperties * _Nullable))completion;

/// The users wallet info if one has been created
@property (nonatomic, readonly, nullable) BATWalletProperties *walletInfo;

/// Fetch details about the users wallet (if they have one) and assigns it to `balance`
- (void)fetchBalance:(nullable void (^)(BATBalance * _Nullable))completion;

/// The users current wallet balance and related info
@property (nonatomic, readonly, nullable) BATBalance *balance;

/// The wallet's passphrase. nil if the wallet has not been created yet
@property (nonatomic, readonly, nullable) NSString *walletPassphrase;

/// Recover the users wallet using their passphrase
- (void)recoverWalletUsingPassphrase:(NSString *)passphrase
                          completion:(nullable void (^)(NSError * _Nullable))completion;

@property (nonatomic, readonly) double defaultContributionAmount;

/// Retrieves the users most up to date balance to determin whether or not the
/// wallet has a sufficient balance to complete a reconcile
- (void)hasSufficientBalanceToReconcile:(void (^)(BOOL sufficient))completion;

/// Returns reserved amount of pending contributions to publishers.
@property (nonatomic, readonly) double reservedAmount;

#pragma mark - Publishers

/// Get publisher info & its activity based on its publisher key
///
/// This key is _not_ always the URL's host. Use `publisherActivityFromURL`
/// instead when obtaining a publisher given a URL
///
/// @note `completion` callback is called synchronously
- (void)listActivityInfoFromStart:(unsigned int)start
                            limit:(unsigned int)limit
                           filter:(BATActivityInfoFilter *)filter
                       completion:(void (NS_NOESCAPE ^)(NSArray<BATPublisherInfo *> *))completion;

/// Start a fetch to get a publishers activity information given a URL
///
/// Use `BATBraveLedgerObserver` to retrieve a panel publisher if one is found
- (void)fetchPublisherActivityFromURL:(NSURL *)URL
                           faviconURL:(nullable NSURL *)faviconURL
                        publisherBlob:(nullable NSString *)publisherBlob
                                tabId:(uint64_t)tabId;

/// Returns activity info for current reconcile stamp.
- (nullable BATPublisherInfo *)currentActivityInfoWithPublisherId:(NSString *)publisherId;

/// Update a publishers exclusion state
- (void)updatePublisherExclusionState:(NSString *)publisherId
                                state:(BATPublisherExclude)state
      NS_SWIFT_NAME(updatePublisherExclusionState(withId:state:));

/// Restore all sites which had been previously excluded
- (void)restoreAllExcludedPublishers;

@property (nonatomic, readonly) NSUInteger numberOfExcludedPublishers;

/// Get the publisher banner given some publisher key
///
/// This key is _not_ always the URL's host. Use `publisherActivityFromURL`
/// instead when obtaining a publisher given a URL
///
/// @note `completion` callback is called synchronously
- (void)publisherBannerForId:(NSString *)publisherId
                  completion:(void (NS_NOESCAPE ^)(BATPublisherBanner * _Nullable banner))completion;

/// Refresh a publishers verification status
- (void)refreshPublisherWithId:(NSString *)publisherId
                    completion:(void (^)(BOOL verified))completion;

#pragma mark - Tips

/// Get a list of publishers who the user has recurring tips on
///
/// @note `completion` callback is called synchronously
- (void)listRecurringTips:(void (NS_NOESCAPE ^)(NSArray<BATPublisherInfo *> *))completion;

- (void)addRecurringTipToPublisherWithId:(NSString *)publisherId
                                  amount:(double)amount
                              completion:(void (^)(BOOL success))completion NS_SWIFT_NAME(addRecurringTip(publisherId:amount:completion:));

- (void)removeRecurringTipForPublisherWithId:(NSString *)publisherId NS_SWIFT_NAME(removeRecurringTip(publisherId:));

/// Get a list of publishers who the user has made direct tips too
///
/// @note `completion` callback is called synchronously
- (void)listOneTimeTips:(void (NS_NOESCAPE ^)(NSArray<BATPublisherInfo *> *))completion;

- (void)tipPublisherDirectly:(BATPublisherInfo *)publisher
                      amount:(int)amount
                    currency:(NSString *)currency
                  completion:(void (^)(BATResult result))completion;


#pragma mark - Grants

@property (nonatomic, readonly) NSArray<BATGrant *> *pendingGrants;

- (void)fetchAvailableGrantsForLanguage:(NSString *)language
                              paymentId:(NSString *)paymentId;

- (void)fetchAvailableGrantsForLanguage:(NSString *)language
                              paymentId:(NSString *)paymentId
                             completion:(nullable void (^)(NSArray<BATGrant *> *grants))completion;

- (void)grantCaptchaForPromotionId:(NSString *)promoID
                     promotionType:(NSString *)promotionType
                        completion:(void (^)(NSString *image, NSString *hint))completion;

- (void)solveGrantCaptchWithPromotionId:(NSString *)promotionId
                               solution:(NSString *)solution;

#pragma mark - History

@property (nonatomic, readonly) NSDictionary<NSString *, BATBalanceReportInfo *> *balanceReports;

- (BATBalanceReportInfo *)balanceReportForMonth:(BATActivityMonth)month
                                           year:(int)year;

@property (nonatomic, readonly) BATAutoContributeProps *autoContributeProps;

#pragma mark - Misc

+ (bool)isMediaURL:(NSURL *)url
     firstPartyURL:(nullable NSURL *)firstPartyURL
       referrerURL:(nullable NSURL *)referrerURL;

/// Get an encoded URL that can be placed in another URL
- (NSString *)encodedURI:(NSString *)uri;

- (void)rewardsInternalInfo:(void (NS_NOESCAPE ^)(BATRewardsInternalsInfo * _Nullable info))completion;

#pragma mark - Reporting

@property (nonatomic) UInt32 selectedTabId;

/// Report that a page has loaded in the current browser tab, and the HTML is available for analysis
- (void)reportLoadedPageWithURL:(NSURL *)url tabId:(UInt32)tabId NS_SWIFT_NAME(reportLoadedPage(url:tabId:));

- (void)reportXHRLoad:(NSURL *)url
                tabId:(UInt32)tabId
        firstPartyURL:(NSURL *)firstPartyURL
          referrerURL:(nullable NSURL *)referrerURL;

- (void)reportPostData:(NSData *)postData
                   url:(NSURL *)url
                 tabId:(UInt32)tabId
         firstPartyURL:(NSURL *)firstPartyURL
           referrerURL:(nullable NSURL *)referrerURL;

/// Report that a tab with a given id navigated or was closed by the user
- (void)reportTabNavigationOrClosedWithTabId:(UInt32)tabId NS_SWIFT_NAME(reportTabNavigationOrClosed(tabId:));

#pragma mark - Preferences

/// Whether or not brave rewards is enabled
@property (nonatomic, assign, getter=isEnabled) BOOL enabled;
/// The number of seconds before a publisher is added.
@property (nonatomic, assign) UInt64 minimumVisitDuration;
/// The minimum number of visits before a publisher is added
@property (nonatomic, assign) UInt32 minimumNumberOfVisits;
/// Whether or not to allow auto contributions to unverified publishers
@property (nonatomic, assign) BOOL allowUnverifiedPublishers;
/// Whether or not to allow auto contributions to videos
@property (nonatomic, assign) BOOL allowVideoContributions;
/// The auto-contribute amount
@property (nonatomic, assign) double contributionAmount;
/// Whether or not the user will automatically contribute
@property (nonatomic, assign, getter=isAutoContributeEnabled) BOOL autoContributeEnabled;

#pragma mark - Ads & Confirmations

/// Update ad totals on month roll over without fetching latest balances from
/// server
- (void)updateAdsRewards;

/// Get the number of ads received and the estimated earnings of viewing said ads for this cycle
- (void)adsDetailsForCurrentCycle:(void (^)(NSInteger adsReceived, double estimatedEarnings))completion NS_SWIFT_NAME(adsDetailsForCurrentCycle(_:));

#pragma mark - Notifications

/// Gets a list of notifications awaiting user interaction
@property (nonatomic, readonly) NSArray<BATRewardsNotification *> *notifications;

/// Clear a given notification
- (void)clearNotification:(BATRewardsNotification *)notification;

/// Clear all the notifications
- (void)clearAllNotifications;

@end

NS_ASSUME_NONNULL_END
