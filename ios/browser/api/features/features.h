// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_FEATURES_FEATURES_H_
#define BRAVE_IOS_BROWSER_API_FEATURES_FEATURES_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface Feature : NSObject
@property(nonatomic, readonly) NSString* name;
@property(nonatomic, readonly) bool enabled;
@end

OBJC_EXPORT
@interface FeatureList : NSObject
// MARK: - Brave Features

@property(class, nonatomic, readonly) Feature* kAIChat;
@property(class, nonatomic, readonly) Feature* kAIChatHistory;
@property(class, nonatomic, readonly)
    Feature* kAdblockOverrideRegexDiscardPolicy;
@property(class, nonatomic, readonly)
    Feature* kAllowUnsupportedWalletProvidersFeature;
@property(class, nonatomic, readonly) Feature* kBraveAdblockCnameUncloaking;
@property(class, nonatomic, readonly)
    Feature* kBraveAdblockCollapseBlockedElements;
@property(class, nonatomic, readonly) Feature* kBraveAdblockCookieListDefault;
@property(class, nonatomic, readonly) Feature* kBraveAdblockCookieListOptIn;
@property(class, nonatomic, readonly) Feature* kBraveAdblockCosmeticFiltering;
@property(class, nonatomic, readonly) Feature* kBraveAdblockCspRules;
@property(class, nonatomic, readonly) Feature* kBraveAdblockDefault1pBlocking;
@property(class, nonatomic, readonly)
    Feature* kBraveAdblockMobileNotificationsListDefault;
@property(class, nonatomic, readonly) Feature* kBraveAdblockScriptletDebugLogs;
@property(class, nonatomic, readonly) Feature* kBraveDarkModeBlock;
@property(class, nonatomic, readonly) Feature* kBraveDeAMP;
@property(class, nonatomic, readonly) Feature* kBraveDebounce;
@property(class, nonatomic, readonly) Feature* kBraveDomainBlock;
@property(class, nonatomic, readonly) Feature* kBraveDomainBlock1PES;
@property(class, nonatomic, readonly) Feature* kBraveLocalhostAccessPermission;
@property(class, nonatomic, readonly) Feature* kBraveNTPBrandedWallpaper;
@property(class, nonatomic, readonly) Feature* kBraveNTPBrandedWallpaperDemo;
@property(class, nonatomic, readonly) Feature* kBraveNTPSuperReferralWallpaper;
@property(class, nonatomic, readonly) Feature* kBraveNewsCardPeekFeature;
@property(class, nonatomic, readonly) Feature* kBraveNewsFeedUpdate;
@property(class, nonatomic, readonly) Feature* kBraveReduceLanguage;
@property(class, nonatomic, readonly) Feature* kBraveSearchDefaultAPIFeature;
@property(class, nonatomic, readonly) Feature* kBraveShredFeature;
@property(class, nonatomic, readonly) Feature* kBraveShredCacheData;
@property(class, nonatomic, readonly)
    Feature* kBraveShowStrictFingerprintingMode;
@property(class, nonatomic, readonly) Feature* kBraveSync;
@property(class, nonatomic, readonly) Feature* kBraveWalletAnkrBalancesFeature;
@property(class, nonatomic, readonly) Feature* kBraveWalletBitcoinFeature;
@property(class, nonatomic, readonly) Feature* kBraveWalletZCashFeature;
@property(class, nonatomic, readonly) Feature* kConstellation;
@property(class, nonatomic, readonly) Feature* kConstellationEnclaveAttestation;
@property(class, nonatomic, readonly)
    Feature* kCosmeticFilteringExtraPerfMetrics;
@property(class, nonatomic, readonly) Feature* kCosmeticFilteringJsPerformance;
@property(class, nonatomic, readonly) Feature* kCosmeticFilteringSyncLoad;
@property(class, nonatomic, readonly, nullable) Feature* kGeminiFeature;
@property(class, nonatomic, readonly) Feature* kNTP;
@property(class, nonatomic, readonly) Feature* kNativeBraveWalletFeature;
@property(class, nonatomic, readonly) Feature* kOtherJSONDeprecation;
@property(class, nonatomic, readonly) Feature* kSkusFeature;
@property(class, nonatomic, readonly) Feature* kTypicalJSONDeprecation;
@property(class, nonatomic, readonly) Feature* kUseDevUpdaterUrl;
@property(class, nonatomic, readonly) Feature* kVerboseLoggingFeature;
@property(class, nonatomic, readonly) Feature* kNewPlaylistUI;
@property(class, nonatomic, readonly) Feature* kBraveHttpsByDefault;
@property(class, nonatomic, readonly) Feature* kHttpsOnlyMode;
@property(class, nonatomic, readonly) Feature* kBlockAllCookiesToggle;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_FEATURES_FEATURES_H_
