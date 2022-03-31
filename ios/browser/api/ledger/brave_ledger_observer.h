/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_LEDGER_BRAVE_LEDGER_OBSERVER_H_
#define BRAVE_IOS_BROWSER_API_LEDGER_BRAVE_LEDGER_OBSERVER_H_

#import <Foundation/Foundation.h>
#import "ledger.mojom.objc.h"

@class BraveLedger, RewardsNotification;

NS_ASSUME_NONNULL_BEGIN

/// A ledger observer can get notified when certain actions happen
///
/// Creating a LedgerObserver alone will not respond to any events. Set
/// each closure that you wish to watch based on the data being displayed on
/// screen
OBJC_EXPORT
NS_SWIFT_NAME(LedgerObserver)
@interface BraveLedgerObserver : NSObject

@property(nonatomic, readonly, weak) BraveLedger* ledger;

- (instancetype)initWithLedger:(BraveLedger*)ledger;

/// Executed when the wallet is first initialized
@property(nonatomic, copy, nullable) void (^walletInitalized)
    (LedgerResult result);

/// A publisher was fetched by its URL for a specific tab identified by tabId
@property(nonatomic, copy, nullable) void (^fetchedPanelPublisher)
    (LedgerPublisherInfo* info, uint64_t tabId);

@property(nonatomic, copy, nullable) void (^publisherListUpdated)();

///
@property(nonatomic, copy, nullable) void (^finishedPromotionsAdded)
    (NSArray<LedgerPromotion*>* promotions);

/// Eligable grants were added to the wallet
@property(nonatomic, copy, nullable) void (^promotionsAdded)
    (NSArray<LedgerPromotion*>* promotions);

/// A grant was claimed
@property(nonatomic, copy, nullable) void (^promotionClaimed)
    (LedgerPromotion* promotion);

/// A reconcile transaction completed and the user may have an updated balance
/// and likely an updated balance report
@property(nonatomic, copy, nullable) void (^reconcileCompleted)
    (LedgerResult result,
     NSString* viewingId,
     LedgerRewardsType type,
     NSString* probi);

/// The users balance report has been updated
@property(nonatomic, copy, nullable) void (^balanceReportUpdated)();

/// The exclusion state of a given publisher has been changed
@property(nonatomic, copy, nullable) void (^excludedSitesChanged)
    (NSString* publisherKey, LedgerPublisherExclude excluded);

/// Called when the ledger removes activity info for a given publisher
@property(nonatomic, copy, nullable) void (^activityRemoved)
    (NSString* publisherKey);

/// The publisher list was normalized and saved
@property(nonatomic, copy, nullable) void (^publisherListNormalized)
    (NSArray<LedgerPublisherInfo*>* normalizedList);

@property(nonatomic, copy, nullable) void (^pendingContributionAdded)();

@property(nonatomic, copy, nullable) void (^pendingContributionsRemoved)
    (NSArray<NSString*>* publisherKeys);

@property(nonatomic, copy, nullable) void (^recurringTipAdded)
    (NSString* publisherKey);

@property(nonatomic, copy, nullable) void (^recurringTipRemoved)
    (NSString* publisherKey);

// A users contribution was added
@property(nonatomic, copy, nullable) void (^contributionAdded)
    (BOOL successful, LedgerRewardsType type);

/// A notification was added to the wallet
@property(nonatomic, copy, nullable) void (^notificationAdded)
    (RewardsNotification* notification);

/// A notification was removed from the wallet
@property(nonatomic, copy, nullable) void (^notificationsRemoved)
    (NSArray<RewardsNotification*>* notification);

/// Wallet balance was fetched and updated
@property(nonatomic, copy, nullable) void (^fetchedBalance)();

@property(nonatomic, copy, nullable) void (^externalWalletAuthorized)
    (NSString* type);

@property(nonatomic, copy, nullable) void (^externalWalletDisconnected)
    (NSString* type);

/// The reconcile stamp reset
@property(nonatomic, copy, nullable) void (^reconcileStampReset)();

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_LEDGER_BRAVE_LEDGER_OBSERVER_H_
