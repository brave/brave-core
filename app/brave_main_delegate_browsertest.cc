/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/update_client/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/domain_reliability/service_factory.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_payments_features.h"
#include "components/commerce/core/commerce_feature_list.h"
#include "components/component_updater/component_updater_switches.h"
#include "components/embedder_support/switches.h"
#include "components/history_clusters/core/features.h"
#include "components/history_clusters/core/on_device_clustering_features.h"
#include "components/language/core/common/language_experiments.h"
#include "components/lens/lens_features.h"
#include "components/network_time/network_time_tracker.h"
#include "components/omnibox/common/omnibox_features.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/permissions/features.h"
#include "components/privacy_sandbox/privacy_sandbox_features.h"
#include "components/reading_list/features/reading_list_switches.h"
#include "components/security_state/core/features.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "gpu/config/gpu_finch_features.h"
#include "media/base/media_switches.h"
#include "net/base/features.h"
#include "services/device/public/cpp/device_features.h"
#include "services/network/public/cpp/features.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/browser/browser_features.h"
#include "chrome/browser/ui/profile_picker.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/translate/core/common/translate_util.h"
#endif

using BraveMainDelegateBrowserTest = PlatformBrowserTest;

const char kBraveOriginTrialsPublicKey[] =
    "bYUKPJoPnCxeNvu72j4EmPuK7tr1PAC7SHh8ld9Mw3E=,"
    "fMS4mpO6buLQ/QMd+zJmxzty/VQ6B1EUZqoCU04zoRU=";

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest,
                       DomainReliabilityServiceDisabled) {
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableDomainReliability));
  EXPECT_FALSE(domain_reliability::DomainReliabilityServiceFactory::
                   ShouldCreateService());
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest,
                       ComponentUpdaterReplacement) {
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kComponentUpdater));
  EXPECT_EQ(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                switches::kComponentUpdater),
            std::string("url-source=") + BUILDFLAG(UPDATER_PROD_ENDPOINT));
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, DisableHyperlinkAuditing) {
  EXPECT_TRUE(
      base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kNoPings));
  content::WebContents* contents =
      chrome_test_utils::GetActiveWebContents(this);
  const blink::web_pref::WebPreferences prefs =
      contents->GetOrCreateWebPreferences();
  EXPECT_FALSE(prefs.hyperlink_auditing_enabled);
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, OriginTrialsTest) {
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      embedder_support::kOriginTrialPublicKey));
  EXPECT_EQ(kBraveOriginTrialsPublicKey,
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                embedder_support::kOriginTrialPublicKey));
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, DisabledFeatures) {
  // Please, keep alphabetized
  const base::Feature* disabled_features[] = {
    &autofill::features::kAutofillEnableAccountWalletStorage,
    &autofill::features::kAutofillEnableOfferNotificationForPromoCodes,
    &autofill::features::kAutofillServerCommunication,
    &blink::features::kAdInterestGroupAPI,
    &blink::features::kBrowsingTopics,
    &blink::features::kAllowURNsInIframes,
    &blink::features::kComputePressure,
    &blink::features::kConversionMeasurement,
    &blink::features::kCssSelectorFragmentAnchor,
    &blink::features::kFencedFrames,
    &blink::features::kFledge,
    &blink::features::kInterestGroupStorage,
    &blink::features::kParakeet,
    &blink::features::kPrerender2,
    &blink::features::kPrivacySandboxAdsAPIs,
    &blink::features::kSpeculationRulesPrefetchProxy,
    &blink::features::kTextFragmentAnchor,
    &commerce::kCommerceDeveloper,
    &commerce::kCommerceMerchantViewer,
    &commerce::kCommercePriceTracking,
    &commerce::kShoppingList,
    &commerce::kShoppingPDPMetrics,
    &commerce::kRetailCoupons,
#if !BUILDFLAG(IS_ANDROID)
    &features::kCopyLinkToText,
#endif
    &features::kDigitalGoodsApi,
    &features::kFedCm,
    &features::kFirstPartySets,
    &features::kIdleDetection,
    &features::kNotificationTriggers,
    &features::kOmniboxTriggerForNoStatePrefetch,
    &features::kPrivacySandboxAdsAPIsOverride,
    &features::kSidePanel,
    &features::kSignedExchangeSubresourcePrefetch,
    &features::kSubresourceWebBundles,
    &features::kWebOTP,
    &history_clusters::features::kOnDeviceClustering,
    &history_clusters::internal::kHistoryClustersInternalsPage,
    &history_clusters::internal::kJourneys,
    &history_clusters::internal::kOmniboxAction,
    &history_clusters::internal::kPersistContextAnnotationsInHistoryDb,
    &lens::features::kLensStandalone,
    &media::kLiveCaption,
    &net::features::kPartitionedCookies,
    &network::features::kTrustTokens,
    &network_time::kNetworkTimeServiceQuerying,
    &optimization_guide::features::kOptimizationHints,
    &optimization_guide::features::kRemoteOptimizationGuideFetching,
    &optimization_guide::features::
        kRemoteOptimizationGuideFetchingAnonymousDataConsent,
    &permissions::features::kPermissionOnDeviceNotificationPredictions,
    &privacy_sandbox::kOverridePrivacySandboxSettingsLocalTesting,
    &privacy_sandbox::kPrivacySandboxSettings3,
    &reading_list::switches::kReadLater,
#if !BUILDFLAG(IS_ANDROID)
    &translate::kTFLiteLanguageDetectionEnabled,
#endif
  };

  for (const auto* feature : disabled_features)
    EXPECT_FALSE(base::FeatureList::IsEnabled(*feature)) << feature->name;
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, EnabledFeatures) {
  const base::Feature* enabled_features[] = {
    &media::kEnableTabMuting,
    &blink::features::kPrefetchPrivacyChanges,
    &blink::features::kReducedReferrerGranularity,
#if BUILDFLAG(IS_WIN)
    &features::kWinrtGeolocationImplementation,
#endif
    &net::features::kPartitionConnectionsByNetworkIsolationKey,
    &net::features::kPartitionExpectCTStateByNetworkIsolationKey,
    &net::features::kPartitionHttpServerPropertiesByNetworkIsolationKey,
    &net::features::kPartitionSSLSessionsByNetworkIsolationKey,
    &net::features::kSplitHostCacheByNetworkIsolationKey,
    &password_manager::features::kPasswordImport,
    &security_state::features::kSafetyTipUI,
  };

  for (const auto* feature : enabled_features)
    EXPECT_TRUE(base::FeatureList::IsEnabled(*feature)) << feature->name;

  EXPECT_TRUE(features::kDnsOverHttpsShowUiParam.default_value);
}
