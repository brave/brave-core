/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "Enums.h"

@class BATAutoContributeProps, BATBalanceReportInfo, BATContributionInfo, BATGrant, BATPublisherBanner, BATReconcileInfo, BATRewardsInternalsInfo, BATTransactionInfo, BATTransactionsInfo, BATTwitchEventInfo, BATVisitData, BATWalletInfo;

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(AutoContributeProps)
@interface BATAutoContributeProps : NSObject
@property (nonatomic) bool enabledContribute;
@property (nonatomic) unsigned long long contributionMinTime;
@property (nonatomic) int contributionMinVisits;
@property (nonatomic) bool contributionNonVerified;
@property (nonatomic) bool contributionVideos;
@property (nonatomic) unsigned long long reconcileStamp;
@end

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

NS_SWIFT_NAME(ContributionInfo)
@interface BATContributionInfo : NSObject
@property (nonatomic) NSString * publisher;
@property (nonatomic) double value;
@property (nonatomic) unsigned long long date;
@end

NS_SWIFT_NAME(Grant)
@interface BATGrant : NSObject
@property (nonatomic) NSString * altcurrency;
@property (nonatomic) NSString * probi;
@property (nonatomic) NSString * promotionId;
@property (nonatomic) unsigned long long expiryTime;
@property (nonatomic) NSString * type;
@end

NS_SWIFT_NAME(PublisherBanner)
@interface BATPublisherBanner : NSObject
@property (nonatomic) NSString * publisherKey;
@property (nonatomic) NSString * title;
@property (nonatomic) NSString * name;
@property (nonatomic) NSString * description;
@property (nonatomic) NSString * background;
@property (nonatomic) NSString * logo;
@property (nonatomic) NSArray<NSNumber *> * amounts;
@property (nonatomic) NSString * provider;
@property (nonatomic) NSDictionary<NSString *, NSString *> * social;
@property (nonatomic) bool verified;
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

NS_SWIFT_NAME(TwitchEventInfo)
@interface BATTwitchEventInfo : NSObject
@property (nonatomic) NSString * event;
@property (nonatomic) NSString * time;
@property (nonatomic) NSString * status;
@end

NS_SWIFT_NAME(VisitData)
@interface BATVisitData : NSObject
@property (nonatomic) NSString * tld;
@property (nonatomic) NSString * domain;
@property (nonatomic) NSString * path;
@property (nonatomic) unsigned int tabId;
@property (nonatomic) NSString * name;
@property (nonatomic) NSString * url;
@property (nonatomic) NSString * provider;
@property (nonatomic) NSString * faviconUrl;
@end

NS_SWIFT_NAME(WalletInfo)
@interface BATWalletInfo : NSObject
@property (nonatomic) NSString * altcurrency;
@property (nonatomic) NSString * probi;
@property (nonatomic) double balance;
@property (nonatomic) double feeAmount;
@property (nonatomic) NSDictionary<NSString *, NSNumber *> * rates;
@property (nonatomic) NSArray<NSNumber *> * parametersChoices;
@property (nonatomic) NSArray<NSNumber *> * parametersRange;
@property (nonatomic) unsigned int parametersDays;
@property (nonatomic) NSArray<BATGrant *> * grants;
@end

NS_ASSUME_NONNULL_END
