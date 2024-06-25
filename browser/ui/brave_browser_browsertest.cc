/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"

#include "brave/browser/ui/browser_commands.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/devtools/devtools_window_testing.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/startup/launch_mode_recorder.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/optimization_guide/optimization_guide_internals/webui/url_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"

using BraveBrowserBrowserTest = InProcessBrowserTest;

namespace {
Browser* OpenNewBrowser(Profile* profile) {
  base::CommandLine dummy(base::CommandLine::NO_PROGRAM);
  StartupBrowserCreatorImpl creator(base::FilePath(), dummy,
                                    chrome::startup::IsFirstRun::kYes);
  creator.Launch(profile, chrome::startup::IsProcessStartup::kNo, nullptr,
                 /*restore_tabbed_browser=*/true);
  return chrome::FindBrowserWithProfile(profile);
}
}  // namespace

IN_PROC_BROWSER_TEST_F(BraveBrowserBrowserTest, NTPFaviconTest) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("brave://newtab/")));

  auto* tab_model = browser()->tab_strip_model();
  EXPECT_FALSE(
      browser()->ShouldDisplayFavicon(tab_model->GetActiveWebContents()));
}

IN_PROC_BROWSER_TEST_F(BraveBrowserBrowserTest, LoadWebUIURLWithBadSchemeTest) {
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("http://settings/")));
}

IN_PROC_BROWSER_TEST_F(BraveBrowserBrowserTest, DisabledFeatureURLLoadTest) {
  // We disabled optimization hints but loading related url should not be
  // crashed. See https://bugs.chromium.org/p/chromium/issues/detail?id=1476101
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL(content::GetWebUIURLString(
                     optimization_guide_internals::
                         kChromeUIOptimizationGuideInternalsHost))));
}

IN_PROC_BROWSER_TEST_F(BraveBrowserBrowserTest, OpenNewTabWhenTabStripIsEmpty) {
  ASSERT_TRUE(embedded_test_server()->Start());
  Browser* new_browser = OpenNewBrowser(browser()->profile());
  ASSERT_TRUE(new_browser);
  new_browser->profile()->GetPrefs()->SetBoolean(kEnableClosingLastTab, false);
  TabStripModel* tab_strip = new_browser->tab_strip_model();
  auto page_url = embedded_test_server()->GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(new_browser, page_url));

  ASSERT_EQ(1, tab_strip->count());
  EXPECT_EQ(page_url,
            tab_strip->GetWebContentsAt(0)->GetURL().possibly_invalid_spec());
  auto* devtools_window = DevToolsWindowTesting::OpenDevToolsWindowSync(
      tab_strip->GetActiveWebContents(), false);
  EXPECT_EQ(chrome::GetTotalBrowserCount(), 3u);

  // Close the last tab.
  tab_strip->GetActiveWebContents()->Close();

  ui_test_utils::WaitForBrowserToClose(
      DevToolsWindowTesting::Get(devtools_window)->browser());
  EXPECT_EQ(chrome::GetTotalBrowserCount(), 2u);
  ASSERT_EQ(1, tab_strip->count());

  // Expecting a new tab is opened.
  EXPECT_EQ(new_browser->GetNewTabURL(),
            tab_strip->GetWebContentsAt(0)->GetURL().possibly_invalid_spec());
  // No reentrancy for Ctrl+W
  tab_strip->CloseSelectedTabs();
  base::RunLoop().RunUntilIdle();
  // Expecting a new tab is opened.
  EXPECT_EQ(new_browser->GetNewTabURL(),
            tab_strip->GetWebContentsAt(0)->GetURL().possibly_invalid_spec());

  // Add a couple of more tabs.
  chrome::AddTabAt(new_browser, new_browser->GetNewTabURL(), -1, true);
  chrome::AddTabAt(new_browser, new_browser->GetNewTabURL(), -1, true);
  ASSERT_EQ(3, tab_strip->count());
  EXPECT_EQ(chrome::GetTotalBrowserCount(), 2u);
  // Close the browser window.
  new_browser->window()->Close();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(chrome::GetTotalBrowserCount(), 1u);
}

IN_PROC_BROWSER_TEST_F(BraveBrowserBrowserTest,
                       DoNotOpenNewTabWhenTabStripIsEmpty) {
  ASSERT_TRUE(embedded_test_server()->Start());
  Browser* new_browser = OpenNewBrowser(browser()->profile());
  ASSERT_TRUE(new_browser);
  new_browser->profile()->GetPrefs()->SetBoolean(kEnableClosingLastTab, true);
  TabStripModel* tab_strip = new_browser->tab_strip_model();
  auto page_url = embedded_test_server()->GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(new_browser, page_url));

  ASSERT_EQ(1, tab_strip->count());
  EXPECT_EQ(page_url,
            tab_strip->GetWebContentsAt(0)->GetURL().possibly_invalid_spec());
  EXPECT_EQ(chrome::GetTotalBrowserCount(), 2u);
  // Close the last tab.
  tab_strip->GetActiveWebContents()->Close();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(chrome::GetTotalBrowserCount(), 1u);
}

IN_PROC_BROWSER_TEST_F(BraveBrowserBrowserTest,
                       DoNotOpenNewTabWhenBringingAllTabs) {
  // Given that kEnableClosingLastTab is false, which normally creates a new tab
  // when tab strip is empty.
  ASSERT_TRUE(embedded_test_server()->Start());
  Browser* new_browser = OpenNewBrowser(browser()->profile());
  ASSERT_TRUE(new_browser);
  new_browser->profile()->GetPrefs()->SetBoolean(kEnableClosingLastTab, false);

  // When "Bring all tabs to this window" commands executes
  brave::BringAllTabs(browser());

  // Then other windows should be closed
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(browser()->tab_strip_model()->count(), 2);
  EXPECT_EQ(chrome::GetTotalBrowserCount(), 1u);
}
