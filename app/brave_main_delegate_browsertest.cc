/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/domain_reliability/service_factory.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_payments_features.h"
#include "components/component_updater/component_updater_switches.h"
#include "components/embedder_support/switches.h"
#include "components/federated_learning/features/features.h"
#include "components/language/core/common/language_experiments.h"
#include "components/network_time/network_time_tracker.h"
#include "components/omnibox/common/omnibox_features.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "components/password_manager/core/common/password_manager_features.h"
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

#if defined(OS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/browser/apps/app_discovery_service/app_discovery_features.h"
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
            "url-source=" UPDATER_PROD_ENDPOINT);
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
  const base::Feature* disabled_features[] = {
#if !defined(OS_ANDROID)
    &apps::kAppDiscoveryRemoteUrlSearch,
    &translate::kTFLiteLanguageDetectionEnabled,
#endif
    &autofill::features::kAutofillEnableAccountWalletStorage,
    &autofill::features::kAutofillServerCommunication,
    &blink::features::kAdInterestGroupAPI,
    &blink::features::kComputePressure,
    &blink::features::kConversionMeasurement,
    &blink::features::kFledge,
    &blink::features::kHandwritingRecognitionWebPlatformApiFinch,
    &blink::features::kInterestCohortAPIOriginTrial,
    &blink::features::kInterestCohortFeaturePolicy,
    &blink::features::kInterestGroupStorage,
    &blink::features::kParakeet,
    &blink::features::kPrerender2,
    &blink::features::kSpeculationRulesPrefetchProxy,
    &blink::features::kTextFragmentAnchor,
    &blink::features::kWebSQLInThirdPartyContextEnabled,
#if !defined(OS_ANDROID)
    &features::kCopyLinkToText,
#endif
    &features::kDirectSockets,
    &features::kIdleDetection,
    &features::kNotificationTriggers,
    &features::kSignedExchangeSubresourcePrefetch,
    &features::kSubresourceWebBundles,
    &features::kWebOTP,
    &federated_learning::kFederatedLearningOfCohorts,
    &federated_learning::kFlocIdComputedEventLogging,
    &media::kLiveCaption,
    &net::features::kFirstPartySets,
    &network::features::kTrustTokens,
    &network_time::kNetworkTimeServiceQuerying,
    &optimization_guide::features::kOptimizationHints,
    &optimization_guide::features::kRemoteOptimizationGuideFetching,
    &optimization_guide::features::
        kRemoteOptimizationGuideFetchingAnonymousDataConsent,
    &reading_list::switches::kReadLater,
  };

  for (const auto* feature : disabled_features)
    EXPECT_FALSE(base::FeatureList::IsEnabled(*feature)) << feature->name;
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, EnabledFeatures) {
  const base::Feature* enabled_features[] = {
    &blink::features::kPrefetchPrivacyChanges,
    &blink::features::kReducedReferrerGranularity,
#if defined(OS_WIN)
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
