/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif



typedef NS_ENUM(NSInteger, BATContributionStep) {
  BATContributionStepStepRetryCount = -7,
  BATContributionStepStepAcOff = -6,
  BATContributionStepStepRewardsOff = -5,
  BATContributionStepStepAcTableEmpty = -4,
  BATContributionStepStepNotEnoughFunds = -3,
  BATContributionStepStepFailed = -2,
  BATContributionStepStepCompleted = -1,
  BATContributionStepStepNo = 0,
  BATContributionStepStepStart = 1,
  BATContributionStepStepPrepare = 2,
  BATContributionStepStepReserve = 3,
  BATContributionStepStepExternalTransaction = 4,
  BATContributionStepStepCreds = 5,
} NS_SWIFT_NAME(ContributionStep);


typedef NS_ENUM(NSInteger, BATContributionProcessor) {
  BATContributionProcessorNone = 0,
  BATContributionProcessorBraveTokens = 1,
  BATContributionProcessorUphold = 2,
  BATContributionProcessorBraveUserFunds = 3,
} NS_SWIFT_NAME(ContributionProcessor);


typedef NS_ENUM(NSInteger, BATExcludeFilter) {
  BATExcludeFilterFilterAll = -1,
  BATExcludeFilterFilterDefault = 0,
  BATExcludeFilterFilterExcluded = 1,
  BATExcludeFilterFilterIncluded = 2,
  BATExcludeFilterFilterAllExceptExcluded = 3,
} NS_SWIFT_NAME(ExcludeFilter);


typedef NS_ENUM(NSInteger, BATContributionRetry) {
  BATContributionRetryStepNo = 0,
  BATContributionRetryStepReconcile = 1,
  BATContributionRetryStepCurrent = 2,
  BATContributionRetryStepPayload = 3,
  BATContributionRetryStepRegister = 4,
  BATContributionRetryStepViewing = 5,
  BATContributionRetryStepWinners = 6,
  BATContributionRetryStepPrepare = 7,
  BATContributionRetryStepProof = 8,
  BATContributionRetryStepVote = 9,
  BATContributionRetryStepFinal = 10,
} NS_SWIFT_NAME(ContributionRetry);


typedef NS_ENUM(NSInteger, BATResult) {
  BATResultLedgerOk = 0,
  BATResultLedgerError = 1,
  BATResultNoPublisherState = 2,
  BATResultNoLedgerState = 3,
  BATResultInvalidPublisherState = 4,
  BATResultInvalidLedgerState = 5,
  BATResultCaptchaFailed = 6,
  BATResultNoPublisherList = 7,
  BATResultTooManyResults = 8,
  BATResultNotFound = 9,
  BATResultRegistrationVerificationFailed = 10,
  BATResultBadRegistrationResponse = 11,
  BATResultWalletCreated = 12,
  BATResultAcTableEmpty = 14,
  BATResultNotEnoughFunds = 15,
  BATResultTipError = 16,
  BATResultCorruptedData = 17,
  BATResultGrantAlreadyClaimed = 18,
  BATResultContributionAmountTooLow = 19,
  BATResultVerifiedPublisher = 20,
  BATResultPendingPublisherRemoved = 21,
  BATResultPendingNotEnoughFunds = 22,
  BATResultRecurringTableEmpty = 23,
  BATResultExpiredToken = 24,
  BATResultBatNotAllowed = 25,
  BATResultAlreadyExists = 26,
  BATResultSafetynetAttestationFailed = 27,
  BATResultDatabaseInitFailed = 28,
  BATResultRetry = 29,
  BATResultRetryShort = 30,
  BATResultRetryLong = 31,
  BATResultContinue = 32,
  BATResultInProgress = 33,
  BATResultRewardsOff = 34,
  BATResultAcOff = 35,
} NS_SWIFT_NAME(Result);


typedef NS_ENUM(NSInteger, BATPublisherStatus) {
  BATPublisherStatusNotVerified = 0,
  BATPublisherStatusConnected = 1,
  BATPublisherStatusVerified = 2,
} NS_SWIFT_NAME(PublisherStatus);


typedef NS_ENUM(NSInteger, BATRewardsType) {
  BATRewardsTypeAutoContribute = 2,
  BATRewardsTypeOneTimeTip = 8,
  BATRewardsTypeRecurringTip = 16,
  BATRewardsTypeTransfer = 17,
  BATRewardsTypePayment = 18,
} NS_SWIFT_NAME(RewardsType);


typedef NS_ENUM(NSInteger, BATReportType) {
  BATReportTypeGrantUgp = 0,
  BATReportTypeAutoContribution = 1,
  BATReportTypeGrantAd = 3,
  BATReportTypeTipRecurring = 4,
  BATReportTypeTip = 5,
} NS_SWIFT_NAME(ReportType);


typedef NS_ENUM(NSInteger, BATUrlMethod) {
  BATUrlMethodGet = 0,
  BATUrlMethodPut = 1,
  BATUrlMethodPost = 2,
  BATUrlMethodPatch = 3,
} NS_SWIFT_NAME(UrlMethod);


typedef NS_ENUM(NSInteger, BATActivityMonth) {
  BATActivityMonthAny = -1,
  BATActivityMonthJanuary = 1,
  BATActivityMonthFebruary = 2,
  BATActivityMonthMarch = 3,
  BATActivityMonthApril = 4,
  BATActivityMonthMay = 5,
  BATActivityMonthJune = 6,
  BATActivityMonthJuly = 7,
  BATActivityMonthAugust = 8,
  BATActivityMonthSeptember = 9,
  BATActivityMonthOctober = 10,
  BATActivityMonthNovember = 11,
  BATActivityMonthDecember = 12,
} NS_SWIFT_NAME(ActivityMonth);


typedef NS_ENUM(NSInteger, BATPublisherExclude) {
  BATPublisherExcludeAll = -1,
  BATPublisherExcludeDefault = 0,
  BATPublisherExcludeExcluded = 1,
  BATPublisherExcludeIncluded = 2,
} NS_SWIFT_NAME(PublisherExclude);


typedef NS_ENUM(NSInteger, BATWalletStatus) {
  BATWalletStatusNotConnected = 0,
  BATWalletStatusConnected = 1,
  BATWalletStatusVerified = 2,
  BATWalletStatusDisconnectedNotVerified = 3,
  BATWalletStatusDisconnectedVerified = 4,
  BATWalletStatusPending = 5,
} NS_SWIFT_NAME(WalletStatus);


typedef NS_ENUM(NSInteger, BATEnvironment) {
  BATEnvironmentStaging = 0,
  BATEnvironmentProduction = 1,
  BATEnvironmentDevelopment = 2,
} NS_SWIFT_NAME(Environment);


typedef NS_ENUM(NSInteger, BATPromotionType) {
  BATPromotionTypeUgp = 0,
  BATPromotionTypeAds = 1,
} NS_SWIFT_NAME(PromotionType);


typedef NS_ENUM(NSInteger, BATPromotionStatus) {
  BATPromotionStatusActive = 0,
  BATPromotionStatusAttested = 1,
  BATPromotionStatusFinished = 4,
  BATPromotionStatusOver = 5,
  BATPromotionStatusCorrupted = 6,
} NS_SWIFT_NAME(PromotionStatus);


typedef NS_ENUM(NSInteger, BATPlatform) {
  BATPlatformDesktop = 0,
  BATPlatformAndroidR = 1,
  BATPlatformIos = 2,
} NS_SWIFT_NAME(Platform);


typedef NS_ENUM(NSInteger, BATOperatingSystem) {
  BATOperatingSystemWindows = 0,
  BATOperatingSystemMacos = 1,
  BATOperatingSystemLinux = 2,
  BATOperatingSystemUndefined = 3,
} NS_SWIFT_NAME(OperatingSystem);


typedef NS_ENUM(NSInteger, BATCredsBatchType) {
  BATCredsBatchTypeNone = 0,
  BATCredsBatchTypePromotion = 1,
  BATCredsBatchTypeSku = 2,
} NS_SWIFT_NAME(CredsBatchType);


typedef NS_ENUM(NSInteger, BATCredsBatchStatus) {
  BATCredsBatchStatusNone = 0,
  BATCredsBatchStatusBlinded = 1,
  BATCredsBatchStatusClaimed = 2,
  BATCredsBatchStatusSigned = 3,
  BATCredsBatchStatusFinished = 4,
  BATCredsBatchStatusCorrupted = 5,
} NS_SWIFT_NAME(CredsBatchStatus);


typedef NS_ENUM(NSInteger, BATSKUOrderStatus) {
  BATSKUOrderStatusNone = 0,
  BATSKUOrderStatusPending = 1,
  BATSKUOrderStatusPaid = 2,
  BATSKUOrderStatusFulfilled = 3,
  BATSKUOrderStatusCanceled = 4,
} NS_SWIFT_NAME(SKUOrderStatus);


typedef NS_ENUM(NSInteger, BATSKUOrderItemType) {
  BATSKUOrderItemTypeNone = 0,
  BATSKUOrderItemTypeSingleUse = 1,
} NS_SWIFT_NAME(SKUOrderItemType);


typedef NS_ENUM(NSInteger, BATSKUTransactionType) {
  BATSKUTransactionTypeNone = 0,
  BATSKUTransactionTypeUphold = 1,
  BATSKUTransactionTypeAnonymousCard = 2,
  BATSKUTransactionTypeTokens = 3,
} NS_SWIFT_NAME(SKUTransactionType);


typedef NS_ENUM(NSInteger, BATSKUTransactionStatus) {
  BATSKUTransactionStatusNone = 0,
  BATSKUTransactionStatusCreated = 1,
  BATSKUTransactionStatusCompleted = 2,
} NS_SWIFT_NAME(SKUTransactionStatus);


typedef NS_ENUM(NSInteger, BATInlineTipsPlatforms) {
  BATInlineTipsPlatformsNone = 0,
  BATInlineTipsPlatformsReddit = 1,
  BATInlineTipsPlatformsTwitter = 2,
  BATInlineTipsPlatformsGithub = 3,
} NS_SWIFT_NAME(InlineTipsPlatforms);



@class BATContributionInfo, BATContributionPublisher, BATPublisherInfo, BATPublisherBanner, BATPendingContribution, BATPendingContributionInfo, BATVisitData, BATRewardsParameters, BATBalance, BATAutoContributeProperties, BATMediaEventInfo, BATExternalWallet, BATBalanceReportInfo, BATActivityInfoFilterOrderPair, BATActivityInfoFilter, BATRewardsInternalsInfo, BATServerPublisherInfo, BATTransferFee, BATContributionQueue, BATContributionQueuePublisher, BATPromotion, BATUnblindedToken, BATClientInfo, BATRecurringTip, BATTransactionReportInfo, BATContributionReportInfo, BATMonthlyReportInfo, BATCredsBatch, BATSKUOrder, BATSKUOrderItem, BATSKUTransaction, BATUrlResponse, BATEventLog;

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(ContributionInfo)
@interface BATContributionInfo : NSObject <NSCopying>
@property (nonatomic, copy) NSString * contributionId;
@property (nonatomic) double amount;
@property (nonatomic) BATRewardsType type;
@property (nonatomic) BATContributionStep step;
@property (nonatomic) int32_t retryCount;
@property (nonatomic) uint64_t createdAt;
@property (nonatomic) BATContributionProcessor processor;
@property (nonatomic, copy) NSArray<BATContributionPublisher *> * publishers;
@end

NS_SWIFT_NAME(ContributionPublisher)
@interface BATContributionPublisher : NSObject <NSCopying>
@property (nonatomic, copy) NSString * contributionId;
@property (nonatomic, copy) NSString * publisherKey;
@property (nonatomic) double totalAmount;
@property (nonatomic) double contributedAmount;
@end

NS_SWIFT_NAME(PublisherInfo)
@interface BATPublisherInfo : NSObject <NSCopying>
@property (nonatomic, copy) NSString * id;
@property (nonatomic) uint64_t duration;
@property (nonatomic) double score;
@property (nonatomic) uint32_t visits;
@property (nonatomic) uint32_t percent;
@property (nonatomic) double weight;
@property (nonatomic) BATPublisherExclude excluded;
@property (nonatomic) int32_t category;
@property (nonatomic) uint64_t reconcileStamp;
@property (nonatomic) BATPublisherStatus status;
@property (nonatomic) uint64_t statusUpdatedAt;
@property (nonatomic, copy) NSString * name;
@property (nonatomic, copy) NSString * url;
@property (nonatomic, copy) NSString * provider;
@property (nonatomic, copy) NSString * faviconUrl;
@end

NS_SWIFT_NAME(PublisherBanner)
@interface BATPublisherBanner : NSObject <NSCopying>
@property (nonatomic, copy) NSString * publisherKey;
@property (nonatomic, copy) NSString * title;
@property (nonatomic, copy) NSString * name;
@property (nonatomic, copy) NSString * desc;
@property (nonatomic, copy) NSString * background;
@property (nonatomic, copy) NSString * logo;
@property (nonatomic, copy) NSArray<NSNumber *> * amounts;
@property (nonatomic, copy) NSString * provider;
@property (nonatomic, copy) NSDictionary<NSString *, NSString *> * links;
@property (nonatomic) BATPublisherStatus status;
@end

NS_SWIFT_NAME(PendingContribution)
@interface BATPendingContribution : NSObject <NSCopying>
@property (nonatomic, copy) NSString * publisherKey;
@property (nonatomic) double amount;
@property (nonatomic) uint64_t addedDate;
@property (nonatomic, copy) NSString * viewingId;
@property (nonatomic) BATRewardsType type;
@end

NS_SWIFT_NAME(PendingContributionInfo)
@interface BATPendingContributionInfo : NSObject <NSCopying>
@property (nonatomic) uint64_t id;
@property (nonatomic, copy) NSString * publisherKey;
@property (nonatomic) BATRewardsType type;
@property (nonatomic) BATPublisherStatus status;
@property (nonatomic) uint64_t statusUpdatedAt;
@property (nonatomic, copy) NSString * name;
@property (nonatomic, copy) NSString * url;
@property (nonatomic, copy) NSString * provider;
@property (nonatomic, copy) NSString * faviconUrl;
@property (nonatomic) double amount;
@property (nonatomic) uint64_t addedDate;
@property (nonatomic, copy) NSString * viewingId;
@property (nonatomic) uint64_t expirationDate;
@end

NS_SWIFT_NAME(VisitData)
@interface BATVisitData : NSObject <NSCopying>
@property (nonatomic, copy) NSString * tld;
@property (nonatomic, copy) NSString * domain;
@property (nonatomic, copy) NSString * path;
@property (nonatomic) uint32_t tabId;
@property (nonatomic, copy) NSString * name;
@property (nonatomic, copy) NSString * url;
@property (nonatomic, copy) NSString * provider;
@property (nonatomic, copy) NSString * faviconUrl;
@end

NS_SWIFT_NAME(RewardsParameters)
@interface BATRewardsParameters : NSObject <NSCopying>
@property (nonatomic) double rate;
@property (nonatomic) double autoContributeChoice;
@property (nonatomic, copy) NSArray<NSNumber *> * autoContributeChoices;
@property (nonatomic, copy) NSArray<NSNumber *> * tipChoices;
@property (nonatomic, copy) NSArray<NSNumber *> * monthlyTipChoices;
@end

NS_SWIFT_NAME(Balance)
@interface BATBalance : NSObject <NSCopying>
@property (nonatomic) double total;
@property (nonatomic) double userFunds;
@property (nonatomic, copy) NSDictionary<NSString *, NSNumber *> * wallets;
@end

NS_SWIFT_NAME(AutoContributeProperties)
@interface BATAutoContributeProperties : NSObject <NSCopying>
@property (nonatomic) bool enabledContribute;
@property (nonatomic) uint64_t contributionMinTime;
@property (nonatomic) int32_t contributionMinVisits;
@property (nonatomic) bool contributionNonVerified;
@property (nonatomic) bool contributionVideos;
@property (nonatomic) uint64_t reconcileStamp;
@end

NS_SWIFT_NAME(MediaEventInfo)
@interface BATMediaEventInfo : NSObject <NSCopying>
@property (nonatomic, copy) NSString * event;
@property (nonatomic, copy) NSString * time;
@property (nonatomic, copy) NSString * status;
@end

NS_SWIFT_NAME(ExternalWallet)
@interface BATExternalWallet : NSObject <NSCopying>
@property (nonatomic, copy) NSString * type;
@property (nonatomic, copy) NSString * token;
@property (nonatomic, copy) NSString * address;
@property (nonatomic) BATWalletStatus status;
@property (nonatomic, copy) NSString * verifyUrl;
@property (nonatomic, copy) NSString * addUrl;
@property (nonatomic, copy) NSString * withdrawUrl;
@property (nonatomic, copy) NSString * oneTimeString;
@property (nonatomic, copy) NSString * userName;
@property (nonatomic, copy) NSString * accountUrl;
@property (nonatomic, copy) NSString * loginUrl;
@end

NS_SWIFT_NAME(BalanceReportInfo)
@interface BATBalanceReportInfo : NSObject <NSCopying>
@property (nonatomic, copy) NSString * id;
@property (nonatomic) double grants;
@property (nonatomic) double earningFromAds;
@property (nonatomic) double autoContribute;
@property (nonatomic) double recurringDonation;
@property (nonatomic) double oneTimeDonation;
@end

NS_SWIFT_NAME(ActivityInfoFilterOrderPair)
@interface BATActivityInfoFilterOrderPair : NSObject <NSCopying>
@property (nonatomic, copy) NSString * propertyName;
@property (nonatomic) bool ascending;
@end

NS_SWIFT_NAME(ActivityInfoFilter)
@interface BATActivityInfoFilter : NSObject <NSCopying>
@property (nonatomic, copy) NSString * id;
@property (nonatomic) BATExcludeFilter excluded;
@property (nonatomic) uint32_t percent;
@property (nonatomic, copy) NSArray<BATActivityInfoFilterOrderPair *> * orderBy;
@property (nonatomic) uint64_t minDuration;
@property (nonatomic) uint64_t reconcileStamp;
@property (nonatomic) bool nonVerified;
@property (nonatomic) uint32_t minVisits;
@end

NS_SWIFT_NAME(RewardsInternalsInfo)
@interface BATRewardsInternalsInfo : NSObject <NSCopying>
@property (nonatomic, copy) NSString * paymentId;
@property (nonatomic) bool isKeyInfoSeedValid;
@property (nonatomic) uint64_t bootStamp;
@end

NS_SWIFT_NAME(ServerPublisherInfo)
@interface BATServerPublisherInfo : NSObject <NSCopying>
@property (nonatomic, copy) NSString * publisherKey;
@property (nonatomic) BATPublisherStatus status;
@property (nonatomic, copy) NSString * address;
@property (nonatomic) uint64_t updatedAt;
@property (nonatomic, copy, nullable) BATPublisherBanner * banner;
@end

NS_SWIFT_NAME(TransferFee)
@interface BATTransferFee : NSObject <NSCopying>
@property (nonatomic, copy) NSString * id;
@property (nonatomic) double amount;
@end

NS_SWIFT_NAME(ContributionQueue)
@interface BATContributionQueue : NSObject <NSCopying>
@property (nonatomic, copy) NSString * id;
@property (nonatomic) BATRewardsType type;
@property (nonatomic) double amount;
@property (nonatomic) bool partial;
@property (nonatomic) uint64_t createdAt;
@property (nonatomic) uint64_t completedAt;
@property (nonatomic, copy) NSArray<BATContributionQueuePublisher *> * publishers;
@end

NS_SWIFT_NAME(ContributionQueuePublisher)
@interface BATContributionQueuePublisher : NSObject <NSCopying>
@property (nonatomic, copy) NSString * publisherKey;
@property (nonatomic) double amountPercent;
@end

NS_SWIFT_NAME(Promotion)
@interface BATPromotion : NSObject <NSCopying>
@property (nonatomic, copy) NSString * id;
@property (nonatomic) uint32_t version;
@property (nonatomic) BATPromotionType type;
@property (nonatomic, copy) NSString * publicKeys;
@property (nonatomic) uint32_t suggestions;
@property (nonatomic) double approximateValue;
@property (nonatomic) BATPromotionStatus status;
@property (nonatomic) uint64_t expiresAt;
@property (nonatomic) uint64_t claimedAt;
@property (nonatomic) bool legacyClaimed;
@property (nonatomic, copy) NSString * claimId;
@end

NS_SWIFT_NAME(UnblindedToken)
@interface BATUnblindedToken : NSObject <NSCopying>
@property (nonatomic) uint64_t id;
@property (nonatomic, copy) NSString * tokenValue;
@property (nonatomic, copy) NSString * publicKey;
@property (nonatomic) double value;
@property (nonatomic, copy) NSString * credsId;
@property (nonatomic) uint64_t expiresAt;
@property (nonatomic) uint64_t redeemedAt;
@property (nonatomic, copy) NSString * redeemId;
@property (nonatomic) BATRewardsType redeemType;
@end

NS_SWIFT_NAME(ClientInfo)
@interface BATClientInfo : NSObject <NSCopying>
@property (nonatomic) BATPlatform platform;
@property (nonatomic) BATOperatingSystem os;
@end

NS_SWIFT_NAME(RecurringTip)
@interface BATRecurringTip : NSObject <NSCopying>
@property (nonatomic, copy) NSString * publisherKey;
@property (nonatomic) double amount;
@property (nonatomic) uint64_t createdAt;
@end

NS_SWIFT_NAME(TransactionReportInfo)
@interface BATTransactionReportInfo : NSObject <NSCopying>
@property (nonatomic) double amount;
@property (nonatomic) BATReportType type;
@property (nonatomic) BATContributionProcessor processor;
@property (nonatomic) uint64_t createdAt;
@end

NS_SWIFT_NAME(ContributionReportInfo)
@interface BATContributionReportInfo : NSObject <NSCopying>
@property (nonatomic, copy) NSString * contributionId;
@property (nonatomic) double amount;
@property (nonatomic) BATReportType type;
@property (nonatomic) BATContributionProcessor processor;
@property (nonatomic, copy) NSArray<BATPublisherInfo *> * publishers;
@property (nonatomic) uint64_t createdAt;
@end

NS_SWIFT_NAME(MonthlyReportInfo)
@interface BATMonthlyReportInfo : NSObject <NSCopying>
@property (nonatomic, copy) BATBalanceReportInfo * balance;
@property (nonatomic, copy) NSArray<BATTransactionReportInfo *> * transactions;
@property (nonatomic, copy) NSArray<BATContributionReportInfo *> * contributions;
@end

NS_SWIFT_NAME(CredsBatch)
@interface BATCredsBatch : NSObject <NSCopying>
@property (nonatomic, copy) NSString * credsId;
@property (nonatomic) int32_t size;
@property (nonatomic, copy) NSString * creds;
@property (nonatomic, copy) NSString * blindedCreds;
@property (nonatomic, copy) NSString * signedCreds;
@property (nonatomic, copy) NSString * publicKey;
@property (nonatomic, copy) NSString * batchProof;
@property (nonatomic, copy) NSString * triggerId;
@property (nonatomic) BATCredsBatchType triggerType;
@property (nonatomic) BATCredsBatchStatus status;
@end

NS_SWIFT_NAME(SKUOrder)
@interface BATSKUOrder : NSObject <NSCopying>
@property (nonatomic, copy) NSString * orderId;
@property (nonatomic) double totalAmount;
@property (nonatomic, copy) NSString * merchantId;
@property (nonatomic, copy) NSString * location;
@property (nonatomic) BATSKUOrderStatus status;
@property (nonatomic, copy) NSString * contributionId;
@property (nonatomic) uint64_t createdAt;
@property (nonatomic, copy) NSArray<BATSKUOrderItem *> * items;
@end

NS_SWIFT_NAME(SKUOrderItem)
@interface BATSKUOrderItem : NSObject <NSCopying>
@property (nonatomic, copy) NSString * orderItemId;
@property (nonatomic, copy) NSString * orderId;
@property (nonatomic, copy) NSString * sku;
@property (nonatomic) int32_t quantity;
@property (nonatomic) double price;
@property (nonatomic, copy) NSString * name;
@property (nonatomic, copy) NSString * desc;
@property (nonatomic) BATSKUOrderItemType type;
@property (nonatomic) uint64_t expiresAt;
@property (nonatomic) uint64_t createdAt;
@end

NS_SWIFT_NAME(SKUTransaction)
@interface BATSKUTransaction : NSObject <NSCopying>
@property (nonatomic, copy) NSString * transactionId;
@property (nonatomic, copy) NSString * orderId;
@property (nonatomic, copy) NSString * externalTransactionId;
@property (nonatomic) BATSKUTransactionType type;
@property (nonatomic) double amount;
@property (nonatomic) BATSKUTransactionStatus status;
@property (nonatomic) uint64_t createdAt;
@end

NS_SWIFT_NAME(UrlResponse)
@interface BATUrlResponse : NSObject <NSCopying>
@property (nonatomic, copy) NSString * url;
@property (nonatomic, copy) NSString * error;
@property (nonatomic) int32_t statusCode;
@property (nonatomic, copy) NSString * body;
@property (nonatomic, copy) NSDictionary<NSString *, NSString *> * headers;
@end

NS_SWIFT_NAME(EventLog)
@interface BATEventLog : NSObject <NSCopying>
@property (nonatomic, copy) NSString * eventLogId;
@property (nonatomic, copy) NSString * key;
@property (nonatomic, copy) NSString * value;
@property (nonatomic) uint64_t createdAt;
@end

NS_ASSUME_NONNULL_END