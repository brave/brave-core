/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_LEDGER_BRAVE_LEDGER_H_
#define BRAVE_IOS_BROWSER_API_LEDGER_BRAVE_LEDGER_H_

#import <Foundation/Foundation.h>
#import "core.mojom.objc.h"
#import "core_types.mojom.objc.h"

@class BraveLedgerObserver, PromotionSolution, RewardsNotification;

NS_ASSUME_NONNULL_BEGIN

typedef NSString* ExternalWalletType NS_SWIFT_NAME(ExternalWalletType)
    NS_STRING_ENUM;

static ExternalWalletType const ExternalWalletTypeUphold = @"uphold";
static ExternalWalletType const ExternalWalletTypeAnonymous = @"anonymous";
static ExternalWalletType const ExternalWalletTypeUnblindedTokens = @"blinded";

typedef void (^LedgerFaviconFetcher)(
    NSURL* pageURL,
    void (^completion)(NSURL* _Nullable faviconURL));

/// The error domain for ledger related errors
OBJC_EXPORT NSString* const BraveLedgerErrorDomain;

OBJC_EXPORT NSNotificationName const BraveLedgerNotificationAdded
    NS_SWIFT_NAME(BraveLedger.notificationAdded);

typedef NSString* BraveGeneralLedgerNotificationID NS_STRING_ENUM;
OBJC_EXPORT BraveGeneralLedgerNotificationID const
    BraveGeneralLedgerNotificationIDWalletNowVerified;
OBJC_EXPORT BraveGeneralLedgerNotificationID const
    BraveGeneralLedgerNotificationIDWalletDisconnected;

OBJC_EXPORT
@interface BraveLedger : NSObject

@property(nonatomic, copy, nullable) LedgerFaviconFetcher faviconFetcher;

/// Create a brave ledger that will read and write its state to the given path
- (instancetype)initWithStateStoragePath:(NSString*)path;

- (instancetype)init NS_UNAVAILABLE;

#pragma mark - Initialization

/// Initialize the ledger service.
///
/// This must be called before other methods on this class are called
- (void)initializeLedgerService:(nullable void (^)())completion;

/// Whether or not the ledger service has been initialized already
@property(nonatomic, readonly, getter=isInitialized) BOOL initialized;

/// Whether or not the ledger service is currently initializing
@property(nonatomic, readonly, getter=isInitializing) BOOL initializing;

/// The result when initializing the ledger service. Should be
/// `BraveRewardsResultLedgerOk` if `initialized` is `true`
///
/// If this is not `BraveRewardsResultLedgerOk`, rewards is not usable for the
/// user
@property(nonatomic, readonly) BraveRewardsResult initializationResult;

/// Whether or not data migration failed when initializing and the user should
/// be notified.
@property(nonatomic, readonly) BOOL dataMigrationFailed;

#pragma mark - Observers

/// Add an interface to the list of observers
///
/// Observers are stored weakly and do not necessarily need to be removed
- (void)addObserver:(BraveLedgerObserver*)observer;

/// Removes an interface from the list of observers
- (void)removeObserver:(BraveLedgerObserver*)observer;

#pragma mark - Wallet

/// Whether or not the wallet is currently in the process of being created
@property(nonatomic, readonly, getter=isInitializingWallet)
    BOOL initializingWallet;

/// Creates a cryptocurrency wallet
- (void)createWallet:(nullable void (^)(NSError* _Nullable error))completion;

/// Get the brave wallet's payment ID and seed for ads confirmations
- (void)currentWalletInfo:
    (void (^)(BraveRewardsRewardsWallet* _Nullable wallet))completion;

/// Get parameters served from the server
- (void)getRewardsParameters:
    (nullable void (^)(BraveRewardsRewardsParameters* _Nullable))completion;

/// The parameters send from the server
@property(nonatomic, readonly, nullable)
    BraveRewardsRewardsParameters* rewardsParameters;

/// Fetch details about the users wallet (if they have one) and assigns it to
/// `balance`
- (void)fetchBalance:
    (nullable void (^)(BraveRewardsBalance* _Nullable))completion;

/// The users current wallet balance and related info
@property(nonatomic, readonly, nullable) BraveRewardsBalance* balance;

/// Returns reserved amount of pending contributions to publishers.
- (void)pendingContributionsTotal:(void (^)(double amount))completion
    NS_SWIFT_NAME(pendingContributionsTotal(completion:));

/// Obtain a drain status given some drain ID previously obtained from
/// `linkBraveWalletToPaymentId:completion:`
- (void)drainStatusForDrainId:(NSString*)drainId
                   completion:
                       (void (^)(BraveRewardsResult result,
                                 BraveRewardsDrainStatus status))completion
    NS_SWIFT_NAME(drainStatus(for:completion:));

#pragma mark - Publishers

@property(nonatomic, readonly, getter=isLoadingPublisherList)
    BOOL loadingPublisherList;

/// Get publisher info & its activity based on its publisher key
///
/// This key is _not_ always the URL's host. Use `publisherActivityFromURL`
/// instead when obtaining a publisher given a URL
///
/// @note `completion` callback is called synchronously
- (void)listActivityInfoFromStart:(unsigned int)start
                            limit:(unsigned int)limit
                           filter:(BraveRewardsActivityInfoFilter*)filter
                       completion:
                           (void (^)(NSArray<BraveRewardsPublisherInfo*>*))
                               completion;

/// Start a fetch to get a publishers activity information given a URL
///
/// Use `BraveLedgerObserver` to retrieve a panel publisher if one is found
- (void)fetchPublisherActivityFromURL:(NSURL*)URL
                           faviconURL:(nullable NSURL*)faviconURL
                        publisherBlob:(nullable NSString*)publisherBlob
                                tabId:(uint64_t)tabId;

/// Update a publishers exclusion state
- (void)updatePublisherExclusionState:(NSString*)publisherId
                                state:(BraveRewardsPublisherExclude)state
    NS_SWIFT_NAME(updatePublisherExclusionState(withId:state:));

/// Restore all sites which had been previously excluded
- (void)restoreAllExcludedPublishers;

/// Get the publisher banner given some publisher key
///
/// This key is _not_ always the URL's host. Use `publisherActivityFromURL`
/// instead when obtaining a publisher given a URL
///
/// @note `completion` callback is called synchronously
- (void)publisherBannerForId:(NSString*)publisherId
                  completion:
                      (void (^)(BraveRewardsPublisherBanner* _Nullable banner))
                          completion;

/// Refresh a publishers verification status
- (void)refreshPublisherWithId:(NSString*)publisherId
                    completion:(void (^)(BraveRewardsPublisherStatus status))
                                   completion;

#pragma mark - SKUs

- (void)processSKUItems:(NSArray<BraveRewardsSKUOrderItem*>*)items
             completion:(void (^)(BraveRewardsResult result,
                                  NSString* orderID))completion;

#pragma mark - Tips

/// Get a list of publishers who the user has recurring tips on
///
/// @note `completion` callback is called synchronously
- (void)listRecurringTips:
    (void (^)(NSArray<BraveRewardsPublisherInfo*>*))completion;

- (void)addRecurringTipToPublisherWithId:(NSString*)publisherId
                                  amount:(double)amount
                              completion:(void (^)(BOOL success))completion
    NS_SWIFT_NAME(addRecurringTip(publisherId:amount:completion:));

- (void)removeRecurringTipForPublisherWithId:(NSString*)publisherId
    NS_SWIFT_NAME(removeRecurringTip(publisherId:));

/// Get a list of publishers who the user has made direct tips too
///
/// @note `completion` callback is called synchronously
- (void)listOneTimeTips:
    (void (^)(NSArray<BraveRewardsPublisherInfo*>*))completion;

- (void)tipPublisherDirectly:(BraveRewardsPublisherInfo*)publisher
                      amount:(double)amount
                    currency:(NSString*)currency
                  completion:(void (^)(BraveRewardsResult result))completion;

#pragma mark - Promotions

@property(nonatomic, readonly)
    NSArray<BraveRewardsPromotion*>* pendingPromotions;

@property(nonatomic, readonly)
    NSArray<BraveRewardsPromotion*>* finishedPromotions;

/// Updates `pendingPromotions` and `finishedPromotions` based on the database
- (void)updatePendingAndFinishedPromotions:(nullable void (^)())completion;

- (void)fetchPromotions:
    (nullable void (^)(NSArray<BraveRewardsPromotion*>* grants))completion;

- (void)claimPromotion:(NSString*)promotionId
             publicKey:(NSString*)deviceCheckPublicKey
            completion:(void (^)(BraveRewardsResult result,
                                 NSString* _Nonnull nonce))completion;

- (void)attestPromotion:(NSString*)promotionId
               solution:(PromotionSolution*)solution
             completion:
                 (nullable void (^)(BraveRewardsResult result,
                                    BraveRewardsPromotion* _Nullable promotion))
                     completion;

#pragma mark - Pending Contributions

- (void)pendingContributions:
    (void (^)(NSArray<BraveRewardsPendingContributionInfo*>* publishers))
        completion;

- (void)removePendingContribution:(BraveRewardsPendingContributionInfo*)info
                       completion:
                           (void (^)(BraveRewardsResult result))completion;

- (void)removeAllPendingContributions:
    (void (^)(BraveRewardsResult result))completion;

#pragma mark - History

- (void)balanceReportForMonth:(BraveRewardsActivityMonth)month
                         year:(int)year
                   completion:
                       (void (^)(BraveRewardsBalanceReportInfo* _Nullable info))
                           completion;

@property(nonatomic, nullable, readonly)
    BraveRewardsAutoContributeProperties* autoContributeProperties;

#pragma mark - Misc

- (void)rewardsInternalInfo:
    (void (^)(BraveRewardsRewardsInternalsInfo* _Nullable info))completion;

- (void)allContributions:
    (void (^)(NSArray<BraveRewardsContributionInfo*>* contributions))completion;

@property(nonatomic, readonly, copy) NSString* rewardsDatabasePath;

#pragma mark - Reporting

@property(nonatomic) UInt32 selectedTabId;

/// Report that a page has loaded in the current browser tab, and the HTML is
/// available for analysis
- (void)reportLoadedPageWithURL:(NSURL*)url
                          tabId:(UInt32)tabId
    NS_SWIFT_NAME(reportLoadedPage(url:tabId:));

- (void)reportXHRLoad:(NSURL*)url
                tabId:(UInt32)tabId
        firstPartyURL:(NSURL*)firstPartyURL
          referrerURL:(nullable NSURL*)referrerURL;

/// Report that a tab with a given id navigated or was closed by the user
- (void)reportTabNavigationOrClosedWithTabId:(UInt32)tabId
    NS_SWIFT_NAME(reportTabNavigationOrClosed(tabId:));

#pragma mark - Preferences

/// The number of seconds before a publisher is added.
@property(nonatomic, assign) int minimumVisitDuration;
/// The minimum number of visits before a publisher is added
@property(nonatomic, assign) int minimumNumberOfVisits;
/// Whether or not to allow auto contributions to unverified publishers
@property(nonatomic, assign) BOOL allowUnverifiedPublishers;
/// The auto-contribute amount
@property(nonatomic, assign) double contributionAmount;
/// Whether or not the user will automatically contribute
@property(nonatomic, assign, getter=isAutoContributeEnabled)
    BOOL autoContributeEnabled;
/// A custom user agent for network operations on ledger
@property(nonatomic, copy, nullable) NSString* customUserAgent;

#pragma mark - Notifications

/// Gets a list of notifications awaiting user interaction
@property(nonatomic, readonly) NSArray<RewardsNotification*>* notifications;

/// Clear a given notification
- (void)clearNotification:(RewardsNotification*)notification;

/// Clear all the notifications
- (void)clearAllNotifications;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_LEDGER_BRAVE_LEDGER_H_
