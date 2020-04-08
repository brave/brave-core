/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "ledger.mojom.objc.h"
#import "Enums.h"

@class BATBraveLedger, BATRewardsNotification;

NS_ASSUME_NONNULL_BEGIN

/// A ledger observer can get notified when certain actions happen
///
/// Creating a LedgerObserver alone will not respond to any events. Set
/// each closure that you wish to watch based on the data being displayed on
/// screen
NS_SWIFT_NAME(LedgerObserver)
@interface BATBraveLedgerObserver : NSObject

@property (nonatomic, readonly, weak) BATBraveLedger *ledger;

- (instancetype)initWithLedger:(BATBraveLedger *)ledger;

/// Rewards was enabled or disabled globally
@property (nonatomic, copy, nullable) void (^rewardsEnabledStateUpdated)(BOOL enabled);

/// Executed when the wallet is first initialized
@property (nonatomic, copy, nullable) void (^walletInitalized)(BATResult result);

/// A publisher was fetched by its URL for a specific tab identified by tabId
@property (nonatomic, copy, nullable) void (^fetchedPanelPublisher)(BATPublisherInfo *info, uint64_t tabId);

@property (nonatomic, copy, nullable) void (^publisherListUpdated)();

/// 
@property (nonatomic, copy, nullable) void (^finishedPromotionsAdded)(NSArray<BATPromotion *> *promotions);

/// Eligable grants were added to the wallet
@property (nonatomic, copy, nullable) void (^promotionsAdded)(NSArray<BATPromotion *> *promotions);

/// A grant was claimed
@property (nonatomic, copy, nullable) void (^promotionClaimed)(BATPromotion *promotion);

/// A reconcile transaction completed and the user may have an updated balance
/// and likely an updated balance report
@property (nonatomic, copy, nullable) void (^reconcileCompleted)(BATResult result,
                                                                 NSString *viewingId,
                                                                 BATRewardsType type,
                                                                 NSString *probi);

/// The users balance report has been updated
@property (nonatomic, copy, nullable) void (^balanceReportUpdated)();

/// The exclusion state of a given publisher has been changed
@property (nonatomic, copy, nullable) void (^excludedSitesChanged)(NSString *publisherKey, BATPublisherExclude excluded);

/// Called when the ledger removes activity info for a given publisher
@property (nonatomic, copy, nullable) void (^activityRemoved)(NSString *publisherKey);

/// confirmationsTransactionHistoryDidChange
@property (nonatomic, copy, nullable) void (^confirmationsTransactionHistoryDidChange)();

/// The publisher list was normalized and saved
@property (nonatomic, copy, nullable) void (^publisherListNormalized)(NSArray<BATPublisherInfo *> *normalizedList);

@property (nonatomic, copy, nullable) void (^pendingContributionAdded)();

@property (nonatomic, copy, nullable) void (^pendingContributionsRemoved)(NSArray<NSString *> *publisherKeys);

@property (nonatomic, copy, nullable) void (^recurringTipAdded)(NSString *publisherKey);

@property (nonatomic, copy, nullable) void (^recurringTipRemoved)(NSString *publisherKey);

// A users contribution was added
@property (nonatomic, copy, nullable) void (^contributionAdded)(BOOL successful, BATRewardsType type);

/// A notification was added to the wallet
@property (nonatomic, copy, nullable) void (^notificationAdded)(BATRewardsNotification *notification);

/// A notification was removed from the wallet
@property (nonatomic, copy, nullable) void (^notificationsRemoved)(NSArray<BATRewardsNotification *> *notification);

/// Wallet balance was fetched and updated
@property (nonatomic, copy, nullable) void (^fetchedBalance)();

@property (nonatomic, copy, nullable) void (^externalWalletAuthorized)(BATWalletType type);

@property (nonatomic, copy, nullable) void (^externalWalletDisconnected)(BATWalletType type);

/// The reconcile stamp reset
@property (nonatomic, copy, nullable) void (^reconcileStampReset)();

@end

NS_ASSUME_NONNULL_END
