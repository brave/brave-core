/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

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
  BATActivityMonthDecember = 12
} NS_SWIFT_NAME(ActivityMonth);

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
  BATContributionRetryStepFinal = 10
} NS_SWIFT_NAME(ContributionRetry);

typedef NS_ENUM(NSInteger, BATExcludeFilter) {
  BATExcludeFilterFilterAll = -1,
  BATExcludeFilterFilterDefault = 0,
  BATExcludeFilterFilterExcluded = 1,
  BATExcludeFilterFilterIncluded = 2,
  BATExcludeFilterFilterAllExceptExcluded = 3
} NS_SWIFT_NAME(ExcludeFilter);

typedef NS_ENUM(NSInteger, BATLogLevel) {
  BATLogLevelLogError = 1,
  BATLogLevelLogWarning = 2,
  BATLogLevelLogInfo = 3,
  BATLogLevelLogDebug = 4,
  BATLogLevelLogRequest = 5,
  BATLogLevelLogResponse = 6
} NS_SWIFT_NAME(LogLevel);

typedef NS_ENUM(NSInteger, BATPublisherExclude) {
  BATPublisherExcludeAll = -1,
  BATPublisherExcludeDefault = 0,
  BATPublisherExcludeExcluded = 1,
  BATPublisherExcludeIncluded = 2
} NS_SWIFT_NAME(PublisherExclude);

typedef NS_ENUM(NSInteger, BATRewardsCategory) {
  BATRewardsCategoryAutoContribute = 2,
  BATRewardsCategoryOneTimeTip = 8,
  BATRewardsCategoryRecurringTip = 16,
  BATRewardsCategoryAllCategories = 31
} NS_SWIFT_NAME(RewardsCategory);

typedef NS_ENUM(NSInteger, BATReportType) {
  BATReportTypeGrant = 0,
  BATReportTypeAutoContribution = 1,
  BATReportTypeDeposit = 2,
  BATReportTypeAds = 3,
  BATReportTypeTipRecurring = 4,
  BATReportTypeTip = 5
} NS_SWIFT_NAME(ReportType);

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
  BATResultGrantNotFound = 13,
  BATResultAcTableEmpty = 14,
  BATResultNotEnoughFunds = 15,
  BATResultTipError = 16,
  BATResultCorruptedWallet = 17,
  BATResultGrantAlreadyClaimed = 18,
  BATResultContributionAmountTooLow = 19
} NS_SWIFT_NAME(Result);

typedef NS_ENUM(NSInteger, BATURLMethod) {
  BATURLMethodGet = 0,
  BATURLMethodPut = 1,
  BATURLMethodPost = 2
} NS_SWIFT_NAME(URLMethod);
