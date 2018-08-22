/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/browser/domain_reliability/service_factory.h"
#include "components/domain_reliability/service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/web_preferences.h"

using BraveMainDelegateBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, DomainReliabilityServiceDisabled) {
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableDomainReliability));
  ASSERT_EQ(domain_reliability::DomainReliabilityServiceFactory::GetForBrowserContext(
        ((content::BrowserContext *)browser()->profile())), nullptr);
}

IN_PROC_BROWSER_TEST_F(BraveMainDelegateBrowserTest, DisableHyperlinkAuditing) {
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kNoPings));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  const content::WebPreferences prefs =
      contents->GetRenderViewHost()->GetWebkitPreferences();
  EXPECT_FALSE(prefs.hyperlink_auditing_enabled);
}
