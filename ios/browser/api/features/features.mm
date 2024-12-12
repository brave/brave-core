// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "features.h"

#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_search/common/features.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_sync/features.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/debounce/core/common/features.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/p3a/features.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/skus/common/features.h"
#include "brave/ios/browser/playlist/features.h"
#include "brave/ios/browser/ui/browser_menu/features.h"
#import "build/blink_buildflags.h"
#include "build/build_config.h"
#include "ios/components/security_interstitials/https_only_mode/feature.h"
#include "net/base/features.h"

@interface Feature () {
  raw_ptr<const base::Feature> _feature;
}
@end

@implementation Feature
- (instancetype)initWithFeature:(const base::Feature*)feature {
  if ((self = [super init])) {
    _feature = feature;
  }
  return self;
}

- (NSString*)name {
  return base::SysUTF8ToNSString(_feature->name);
}

- (bool)enabled {
  return base::FeatureList::IsEnabled(std::cref(*_feature));
}

//- (void)setEnabled:(bool)enabled {
//  std::vector<base::FeatureList::FeatureOverrideInfo> overrides = {
//    {
//        std::cref(*_value), enabled ?
//        base::FeatureList::OverrideState::OVERRIDE_ENABLE_FEATURE :
//        base::FeatureList::OverrideState::OVERRIDE_DISABLE_FEATURE
//    }
//  };
//  base::FeatureList::GetInstance()->RegisterExtraFeatureOverrides(overrides);
//}
@end

@implementation FeatureList

// MARK: - Brave Features

+ (Feature*)kAIChat {
  return [[Feature alloc] initWithFeature:&ai_chat::features::kAIChat];
}

+ (Feature*)kAIChatHistory {
  return [[Feature alloc] initWithFeature:&ai_chat::features::kAIChatHistory];
}

+ (Feature*)kAdblockOverrideRegexDiscardPolicy {
  return
      [[Feature alloc] initWithFeature:&brave_shields::features::
                                           kAdblockOverrideRegexDiscardPolicy];
}

+ (Feature*)kAllowUnsupportedWalletProvidersFeature {
  return [[Feature alloc]
      initWithFeature:&brave_rewards::features::
                          kAllowUnsupportedWalletProvidersFeature];
}

+ (Feature*)kBraveAdblockCnameUncloaking {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveAdblockCnameUncloaking];
}

+ (Feature*)kBraveAdblockCollapseBlockedElements {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::
                          kBraveAdblockCollapseBlockedElements];
}

+ (Feature*)kBraveAdblockCookieListDefault {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveAdblockCookieListDefault];
}

+ (Feature*)kBraveAdblockCookieListOptIn {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveAdblockCookieListOptIn];
}

+ (Feature*)kBraveAdblockCosmeticFiltering {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveAdblockCosmeticFiltering];
}

+ (Feature*)kBraveAdblockCspRules {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveAdblockCspRules];
}

+ (Feature*)kBraveAdblockDefault1pBlocking {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveAdblockDefault1pBlocking];
}

+ (Feature*)kBraveAdblockMobileNotificationsListDefault {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::
                          kBraveAdblockMobileNotificationsListDefault];
}

+ (Feature*)kBraveAdblockScriptletDebugLogs {
  return [[Feature alloc] initWithFeature:&brave_shields::features::
                                              kBraveAdblockScriptletDebugLogs];
}

+ (Feature*)kBraveDarkModeBlock {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveDarkModeBlock];
}

+ (Feature*)kBraveDeAMP {
  return [[Feature alloc] initWithFeature:&de_amp::features::kBraveDeAMP];
}

+ (Feature*)kBraveDebounce {
  return [[Feature alloc] initWithFeature:&debounce::features::kBraveDebounce];
}

+ (Feature*)kBraveDomainBlock {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveDomainBlock];
}

+ (Feature*)kBraveDomainBlock1PES {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveDomainBlock1PES];
}

+ (Feature*)kBraveLocalhostAccessPermission {
  return [[Feature alloc] initWithFeature:&brave_shields::features::
                                              kBraveLocalhostAccessPermission];
}

+ (Feature*)kBraveNTPBrandedWallpaper {
  return [[Feature alloc] initWithFeature:&ntp_background_images::features::
                                              kBraveNTPBrandedWallpaper];
}

+ (Feature*)kBraveNTPBrandedWallpaperDemo {
  return [[Feature alloc] initWithFeature:&ntp_background_images::features::
                                              kBraveNTPBrandedWallpaperDemo];
}

+ (Feature*)kBraveNTPSuperReferralWallpaper {
  return [[Feature alloc] initWithFeature:&ntp_background_images::features::
                                              kBraveNTPSuperReferralWallpaper];
}

+ (Feature*)kBraveNewsCardPeekFeature {
  return [[Feature alloc]
      initWithFeature:&brave_news::features::kBraveNewsCardPeekFeature];
}

+ (Feature*)kBraveNewsFeedUpdate {
  return [[Feature alloc]
      initWithFeature:&brave_news::features::kBraveNewsFeedUpdate];
}

+ (Feature*)kBraveReduceLanguage {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveReduceLanguage];
}

+ (Feature*)kBraveSearchDefaultAPIFeature {
  return [[Feature alloc]
      initWithFeature:&brave_search::features::kBraveSearchDefaultAPIFeature];
}

+ (Feature*)kBraveShredFeature {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveShredFeature];
}

+ (Feature*)kBraveShredCacheData {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveShredCacheData];
}

+ (Feature*)kBraveIOSDebugAdblock {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBraveIOSDebugAdblock];
}

+ (Feature*)kBraveShowStrictFingerprintingMode {
  return
      [[Feature alloc] initWithFeature:&brave_shields::features::
                                           kBraveShowStrictFingerprintingMode];
}

+ (Feature*)kBraveSync {
  return [[Feature alloc] initWithFeature:&brave_sync::features::kBraveSync];
}

+ (Feature*)kBraveWalletAnkrBalancesFeature {
  return [[Feature alloc]
      initWithFeature:&brave_wallet::features::kBraveWalletAnkrBalancesFeature];
}

+ (Feature*)kBraveWalletBitcoinFeature {
  return [[Feature alloc]
      initWithFeature:&brave_wallet::features::kBraveWalletBitcoinFeature];
}

+ (Feature*)kBraveWalletZCashFeature {
  return [[Feature alloc]
      initWithFeature:&brave_wallet::features::kBraveWalletZCashFeature];
}

+ (Feature*)kConstellation {
  return [[Feature alloc] initWithFeature:&p3a::features::kConstellation];
}

+ (Feature*)kConstellationEnclaveAttestation {
  return [[Feature alloc]
      initWithFeature:&p3a::features::kConstellationEnclaveAttestation];
}

+ (Feature*)kCosmeticFilteringExtraPerfMetrics {
  return
      [[Feature alloc] initWithFeature:&brave_shields::features::
                                           kCosmeticFilteringExtraPerfMetrics];
}

+ (Feature*)kCosmeticFilteringJsPerformance {
  return [[Feature alloc] initWithFeature:&brave_shields::features::
                                              kCosmeticFilteringJsPerformance];
}

+ (Feature*)kCosmeticFilteringSyncLoad {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kCosmeticFilteringSyncLoad];
}

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
+ (Feature*)kGeminiFeature {
  return [[Feature alloc] initWithFeature:&kGeminiFeature];
}
#else
+ (Feature*)kGeminiFeature {
  return nil;
}
#endif

+ (Feature*)kNTP {
  return [[Feature alloc]
      initWithFeature:&brave_search_conversion::features::kNTP];
}

+ (Feature*)kNativeBraveWalletFeature {
  return [[Feature alloc]
      initWithFeature:&brave_wallet::features::kNativeBraveWalletFeature];
}

+ (Feature*)kOtherJSONDeprecation {
  return
      [[Feature alloc] initWithFeature:&p3a::features::kOtherJSONDeprecation];
}

+ (Feature*)kSkusFeature {
  return [[Feature alloc] initWithFeature:&skus::features::kSkusFeature];
}

+ (Feature*)kTypicalJSONDeprecation {
  return
      [[Feature alloc] initWithFeature:&p3a::features::kTypicalJSONDeprecation];
}

+ (Feature*)kUseDevUpdaterUrl {
  return [[Feature alloc]
      initWithFeature:&brave_component_updater::kUseDevUpdaterUrl];
}

+ (Feature*)kVerboseLoggingFeature {
  return [[Feature alloc]
      initWithFeature:&brave_rewards::features::kVerboseLoggingFeature];
}

+ (Feature*)kNewPlaylistUI {
  return [[Feature alloc] initWithFeature:&playlist::features::kNewPlaylistUI];
}

+ (Feature*)kBraveHttpsByDefault {
  return [[Feature alloc] initWithFeature:&net::features::kBraveHttpsByDefault];
}

+ (Feature*)kHttpsOnlyMode {
  return [[Feature alloc]
      initWithFeature:&security_interstitials::features::kHttpsOnlyMode];
}

+ (Feature*)kBlockAllCookiesToggle {
  return [[Feature alloc]
      initWithFeature:&brave_shields::features::kBlockAllCookiesToggle];
}

+ (Feature*)kModernBrowserMenuEnabled {
  return [[Feature alloc]
      initWithFeature:&brave::features::kModernBrowserMenuEnabled];
}

@end
