/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "Enums.h"

@class BATBalanceReportInfo, BATReconcileInfo, BATRewardsInternalsInfo, BATTransactionInfo, BATTransactionsInfo, BATMediaEventInfo;

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(BalanceReportInfo)
@interface BATBalanceReportInfo : NSObject
@property (nonatomic) NSString * openingBalance;
@property (nonatomic) NSString * closingBalance;
@property (nonatomic) NSString * deposits;
@property (nonatomic) NSString * grants;
@property (nonatomic) NSString * earningFromAds;
@property (nonatomic) NSString * autoContribute;
@property (nonatomic) NSString * recurringDonation;
@property (nonatomic) NSString * oneTimeDonation;
@property (nonatomic) NSString * total;
@end

NS_SWIFT_NAME(ReconcileInfo)
@interface BATReconcileInfo : NSObject
@property (nonatomic) NSString * viewingid;
@property (nonatomic) NSString * amount;
@property (nonatomic) BATContributionRetry retryStep;
@property (nonatomic) int retryLevel;
@end

NS_SWIFT_NAME(RewardsInternalsInfo)
@interface BATRewardsInternalsInfo : NSObject
@property (nonatomic) NSString * paymentId;
@property (nonatomic) bool isKeyInfoSeedValid;
@property (nonatomic) NSString * personaId;
@property (nonatomic) NSString * userId;
@property (nonatomic) unsigned long long bootStamp;
@property (nonatomic) NSDictionary<NSString *, BATReconcileInfo *> * currentReconciles;
@end

NS_SWIFT_NAME(TransactionInfo)
@interface BATTransactionInfo : NSObject
@property (nonatomic) unsigned long long timestampInSeconds;
@property (nonatomic) double estimatedRedemptionValue;
@property (nonatomic) NSString * confirmationType;
@end

NS_SWIFT_NAME(TransactionsInfo)
@interface BATTransactionsInfo : NSObject
@property (nonatomic) NSArray<BATTransactionInfo *> * transactions;
@end

NS_SWIFT_NAME(MediaEventInfo)
@interface BATMediaEventInfo : NSObject
@property (nonatomic) NSString * event;
@property (nonatomic) NSString * time;
@property (nonatomic) NSString * status;
@end

NS_ASSUME_NONNULL_END
