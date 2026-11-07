// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_tab_helpers.h"

#include <memory>

#include "base/location.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/resource_coordinator/lifecycle_unit_state.mojom.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_external.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_source.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace {

// Checks that `web_contents` has every privacy-sensitive helper.
void ExpectPrivacySensitiveTabHelpers(
    content::WebContents* web_contents,
    const base::Location& location = base::Location::Current()) {
  SCOPED_TRACE(location.ToString());
  ASSERT_TRUE(web_contents);
  EXPECT_TRUE(brave_shields::BraveShieldsWebContentsObserver::FromWebContents(
      web_contents));
  EXPECT_TRUE(ephemeral_storage::EphemeralStorageTabHelper::FromWebContents(
      web_contents));
  EXPECT_TRUE(
      content_settings::PageSpecificContentSettings::GetDelegateForWebContents(
          web_contents));
}

}  // namespace

class BraveTabHelpersBrowserTest : public PlatformBrowserTest {
 protected:
  Profile* GetProfile() { return chrome_test_utils::GetProfile(this); }
};

IN_PROC_BROWSER_TEST_F(BraveTabHelpersBrowserTest,
                       TabHasPrivacySensitiveHelpers) {
  content::WebContents* const web_contents =
      chrome_test_utils::GetActiveWebContents(this);
  ASSERT_TRUE(tabs::TabInterface::MaybeGetFromContents(web_contents));
  ExpectPrivacySensitiveTabHelpers(web_contents);
}

// A WebContents that is not a tab, such as one AI Chat or prefetching creates,
// gets the helpers as soon as it is created.
IN_PROC_BROWSER_TEST_F(BraveTabHelpersBrowserTest,
                       WebContentsNotInATabHasPrivacySensitiveHelpers) {
  std::unique_ptr<content::WebContents> web_contents =
      content::WebContents::Create(
          content::WebContents::CreateParams(GetProfile()));
  ASSERT_FALSE(tabs::TabInterface::MaybeGetFromContents(web_contents.get()));
  ExpectPrivacySensitiveTabHelpers(web_contents.get());
}

// Attaching the helpers again, as AI Chat does for the WebContents it creates,
// keeps the existing ones.
IN_PROC_BROWSER_TEST_F(BraveTabHelpersBrowserTest,
                       AttachingAgainKeepsTheExistingHelpers) {
  std::unique_ptr<content::WebContents> web_contents =
      content::WebContents::Create(
          content::WebContents::CreateParams(GetProfile()));
  auto* const shields_observer =
      brave_shields::BraveShieldsWebContentsObserver::FromWebContents(
          web_contents.get());
  auto* const ephemeral_storage_tab_helper =
      ephemeral_storage::EphemeralStorageTabHelper::FromWebContents(
          web_contents.get());
  auto* const content_settings_delegate =
      content_settings::PageSpecificContentSettings::GetDelegateForWebContents(
          web_contents.get());

  brave::AttachPrivacySensitiveTabHelpers(web_contents.get());

  EXPECT_EQ(brave_shields::BraveShieldsWebContentsObserver::FromWebContents(
                web_contents.get()),
            shields_observer);
  EXPECT_EQ(ephemeral_storage::EphemeralStorageTabHelper::FromWebContents(
                web_contents.get()),
            ephemeral_storage_tab_helper);
  EXPECT_EQ(
      content_settings::PageSpecificContentSettings::GetDelegateForWebContents(
          web_contents.get()),
      content_settings_delegate);
}

#if !BUILDFLAG(IS_ANDROID)
IN_PROC_BROWSER_TEST_F(BraveTabHelpersBrowserTest,
                       NewTabHasPrivacySensitiveHelpers) {
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("about:blank"), WindowOpenDisposition::kNewForegroundTab,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_EQ(browser()->tab_strip_model()->count(), 2);
  ExpectPrivacySensitiveTabHelpers(
      browser()->tab_strip_model()->GetActiveWebContents());
}

// Discarding a tab replaces its WebContents, and the replacement gets the
// helpers too.
IN_PROC_BROWSER_TEST_F(BraveTabHelpersBrowserTest,
                       DiscardedTabContentsHasPrivacySensitiveHelpers) {
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("about:blank"), WindowOpenDisposition::kNewBackgroundTab,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  TabStripModel* const tab_strip_model = browser()->tab_strip_model();
  tabs::TabInterface* const tab =
      tab_strip_model->GetTabAtIndex(tab_strip_model->count() - 1);
  content::WebContents* const discarded_contents = tab->GetContents();

  auto* const lifecycle_unit =
      resource_coordinator::TabLifecycleUnitSource::GetTabLifecycleUnitExternal(
          discarded_contents);
  ASSERT_TRUE(lifecycle_unit);
  ASSERT_TRUE(
      lifecycle_unit->DiscardTab(::mojom::LifecycleUnitDiscardReason::URGENT));

  ASSERT_NE(tab->GetContents(), discarded_contents);
  ExpectPrivacySensitiveTabHelpers(tab->GetContents());
}

// The shared Top Chrome WebUI profile has no content settings, which the
// helpers depend on, and hosts no web pages, so its WebContents get none.
IN_PROC_BROWSER_TEST_F(BraveTabHelpersBrowserTest,
                       WebContentsWithoutContentSettingsHasNoHelpers) {
  webui::ScopedTopChromeWebUIProfileForTesting scoped_top_chrome_webui_profile;
  std::unique_ptr<content::WebContents> web_contents =
      content::WebContents::Create(content::WebContents::CreateParams(
          webui::GetTopChromeWebUIProfile()));

  EXPECT_FALSE(brave_shields::BraveShieldsWebContentsObserver::FromWebContents(
      web_contents.get()));
  EXPECT_FALSE(ephemeral_storage::EphemeralStorageTabHelper::FromWebContents(
      web_contents.get()));
  EXPECT_FALSE(
      content_settings::PageSpecificContentSettings::GetDelegateForWebContents(
          web_contents.get()));
}
#endif  // !BUILDFLAG(IS_ANDROID)
