/* This Source Code Form is subject to the terms of the Mozilla Public
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

typedef NS_ENUM(NSInteger, BATReportType) {
  BATReportTypeGrant = 0,
  BATReportTypeAutoContribution = 1,
  BATReportTypeDeposit = 2,
  BATReportTypeAds = 3,
  BATReportTypeTipRecurring = 4,
  BATReportTypeTip = 5
} NS_SWIFT_NAME(ReportType);

typedef NS_ENUM(NSInteger, BATURLMethod) {
  BATURLMethodGet = 0,
  BATURLMethodPut = 1,
  BATURLMethodPost = 2
} NS_SWIFT_NAME(URLMethod);
