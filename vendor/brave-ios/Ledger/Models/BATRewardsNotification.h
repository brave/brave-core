// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, BATRewardsNotificationKind) {
  BATRewardsNotificationKindInvalid,
  BATRewardsNotificationKindAutoContribute,
  BATRewardsNotificationKindGrant,
  BATRewardsNotificationKindGrantAds,
  BATRewardsNotificationKindFailedContribution,
  BATRewardsNotificationKindInsufficientFunds,
  BATRewardsNotificationKindBackupWallet,
  BATRewardsNotificationKindTipsProcessed,
  BATRewardsNotificationKindAdsLaunch, // Unused
  BATRewardsNotificationKindVerifiedPublisher,
  BATRewardsNotificationKindPendingNotEnoughFunds,
  BATRewardsNotificationKindGeneralLedger // Comes from ledger
} NS_SWIFT_NAME(RewardsNotification.Kind);

OBJC_EXPORT
NS_SWIFT_NAME(RewardsNotification)
@interface BATRewardsNotification : NSObject <NSSecureCoding>

@property (nonatomic, copy) NSString *id;
@property (nonatomic) NSTimeInterval dateAdded;
@property (nonatomic) BATRewardsNotificationKind kind;
@property (nonatomic, copy) NSDictionary *userInfo;
@property (nonatomic) BOOL displayed;

- (instancetype)initWithID:(NSString *)notificationID
                 dateAdded:(NSTimeInterval)dateAdded
                      kind:(BATRewardsNotificationKind)kind
                  userInfo:(nullable NSDictionary *)userInfo;

@end

NS_ASSUME_NONNULL_END
