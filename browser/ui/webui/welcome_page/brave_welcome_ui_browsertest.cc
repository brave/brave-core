/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "brave/browser/ui/webui/brave_web_ui_controller_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/startup/launch_mode_recorder.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace {
Browser* OpenNewBrowser(Profile* profile) {
  base::CommandLine dummy(base::CommandLine::NO_PROGRAM);
  StartupBrowserCreator browser_creator;
  StartupBrowserCreatorImpl creator(base::FilePath(), dummy, &browser_creator,
                                    chrome::startup::IsFirstRun::kYes);
  creator.Launch(profile, chrome::startup::IsProcessStartup::kNo,
                 /*restore_tabbed_browser=*/true);
  return chrome::FindBrowserWithProfile(profile);
}
}  // namespace

class BraveWelcomeUIBrowserTest : public InProcessBrowserTest {
 public:
  BraveWelcomeUIBrowserTest() = default;
  ~BraveWelcomeUIBrowserTest() override = default;

  void SetUpDefaultCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpDefaultCommandLine(command_line);
    if (GetTestPreCount() > 0) {
      command_line->RemoveSwitch(switches::kNoFirstRun);
    }
  }
};

// Check whether startup url at first run is our welcome page.
IN_PROC_BROWSER_TEST_F(BraveWelcomeUIBrowserTest, PRE_StartupURLTest) {
  Browser* new_browser = OpenNewBrowser(browser()->profile());
  ASSERT_TRUE(new_browser);
  TabStripModel* tab_strip = new_browser->tab_strip_model();
  ASSERT_EQ(1, tab_strip->count());
  content::WebContents* web_contents = tab_strip->GetWebContentsAt(0);
  content::TestNavigationObserver observer(web_contents, 1);
  observer.Wait();
  EXPECT_STREQ("chrome://welcome/", tab_strip->GetWebContentsAt(0)
                                        ->GetController()
                                        .GetLastCommittedEntry()
                                        ->GetVirtualURL()
                                        .possibly_invalid_spec()
                                        .c_str());
}

// Check wheter startup url is not welcome ui at second run.
IN_PROC_BROWSER_TEST_F(BraveWelcomeUIBrowserTest, StartupURLTest) {
  Browser* new_browser = OpenNewBrowser(browser()->profile());
  ASSERT_TRUE(new_browser);
  TabStripModel* tab_strip = new_browser->tab_strip_model();
  ASSERT_EQ(1, tab_strip->count());
  content::WebContents* web_contents = tab_strip->GetWebContentsAt(0);
  content::TestNavigationObserver observer(web_contents, 1);
  observer.Wait();
  EXPECT_EQ(chrome::kChromeUINewTabURL,
            tab_strip->GetWebContentsAt(0)->GetURL().possibly_invalid_spec());
}
