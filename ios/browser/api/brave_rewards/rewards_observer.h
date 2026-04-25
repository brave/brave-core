/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_REWARDS_REWARDS_OBSERVER_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_REWARDS_REWARDS_OBSERVER_H_

#import <Foundation/Foundation.h>
#import "rewards.mojom.objc.h"

@class BraveRewardsAPI, RewardsNotification;

NS_ASSUME_NONNULL_BEGIN

/// A rewards observer can get notified when certain actions happen
///
/// Creating a RewardsObserver alone will not respond to any events. Set
/// each closure that you wish to watch based on the data being displayed on
/// screen
OBJC_EXPORT
NS_SWIFT_NAME(RewardsObserver)
@interface RewardsObserver : NSObject

@property(nonatomic, readonly, weak) BraveRewardsAPI* rewardsAPI;

- (instancetype)initWithRewardsAPI:(BraveRewardsAPI*)rewardsAPI;

/// Executed when the wallet is first initialized
@property(nonatomic, copy, nullable) void (^walletInitalized)
    (BraveRewardsResult result);

/// A publisher was fetched by its URL for a specific tab identified by tabId
@property(nonatomic, copy, nullable) void (^fetchedPanelPublisher)
    (BraveRewardsPublisherInfo* info, uint64_t tabId);

@property(nonatomic, copy, nullable) void (^publisherListUpdated)();

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_REWARDS_REWARDS_OBSERVER_H_
