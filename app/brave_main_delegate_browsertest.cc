/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/domain_reliability/service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_payments_features.h"
#include "components/omnibox/common/omnibox_features.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/unified_consent/feature.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/content_features.h"
#include "content/public/common/web_preferences.h"
#include "extensions/common/extension_features.h"
#include "gpu/config/gpu_finch_features.h"
#include "services/network/public/cpp/features.h"

using BraveMainDelegateBrowserTest = InProcessBrowserTest;

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
  const content::WebPreferences prefs =
      contents->GetRenderViewHost()->GetWebkitPreferences();
  EXPECT_FALSE(prefs.hyperlink_auditing_enabled);
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, DisabledFeatures) {
  const base::Feature* disabled_features[] = {
      &autofill::features::kAutofillServerCommunication,
      &features::kAudioServiceOutOfProcess,
      &features::kDefaultEnableOopRasterization,
      &features::kNotificationTriggers,
      &features::kSmsReceiver,
      &unified_consent::kUnifiedConsent,
      &features::kLookalikeUrlNavigationSuggestionsUI,
      &switches::kSyncUSSBookmarks,
  };

  for (const auto* feature : disabled_features)
    EXPECT_FALSE(base::FeatureList::IsEnabled(*feature));
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, EnabledFeatures) {
  const base::Feature* enabled_features[] = {
#if BUILDFLAG(ENABLE_EXTENSIONS)
    &extensions_features::kNewExtensionUpdaterService,
#endif
    &omnibox::kSimplifyHttpsIndicator,
  };

  for (const auto* feature : enabled_features)
    EXPECT_TRUE(base::FeatureList::IsEnabled(*feature));
}
