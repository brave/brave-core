/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/domain_reliability/service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_payments_features.h"
#include "components/embedder_support/switches.h"
#include "components/language/core/common/language_experiments.h"
#include "components/network_time/network_time_tracker.h"
#include "components/omnibox/common/omnibox_features.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/safe_browsing/core/features.h"
#include "components/security_state/core/features.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "gpu/config/gpu_finch_features.h"
#include "net/base/features.h"
#include "services/device/public/cpp/device_features.h"
#include "services/network/public/cpp/features.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"

using BraveMainDelegateBrowserTest = InProcessBrowserTest;

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

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, DisableHyperlinkAuditing) {
  EXPECT_TRUE(
      base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kNoPings));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
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
      &autofill::features::kAutofillEnableAccountWalletStorage,
      &autofill::features::kAutofillServerCommunication,
      &blink::features::kTextFragmentAnchor,
      &features::kDirectSockets,
      &features::kIdleDetection,
      &features::kLangClientHintHeader,
      &features::kNotificationTriggers,
      &features::kSignedExchangePrefetchCacheForNavigations,
      &features::kSignedExchangeSubresourcePrefetch,
      &features::kSubresourceWebBundles,
      &features::kTabHoverCards,
      &features::kWebOTP,
      &network_time::kNetworkTimeServiceQuerying,
      &safe_browsing::kEnhancedProtection,
      &safe_browsing::kEnhancedProtectionMessageInInterstitials,
  };

  for (const auto* feature : disabled_features)
    EXPECT_FALSE(base::FeatureList::IsEnabled(*feature));
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, EnabledFeatures) {
  const base::Feature* enabled_features[] = {
    &blink::features::kPrefetchPrivacyChanges,
    &password_manager::features::kPasswordImport,
    &blink::features::kReducedReferrerGranularity,
#if defined(OS_WIN)
    &features::kWinrtGeolocationImplementation,
#endif
    &net::features::kLegacyTLSEnforced,
    &security_state::features::kSafetyTipUI,
  };

  for (const auto* feature : enabled_features)
    EXPECT_TRUE(base::FeatureList::IsEnabled(*feature));

  EXPECT_TRUE(features::kDnsOverHttpsShowUiParam.default_value);
}
