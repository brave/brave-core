/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/run_loop.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test.h"

namespace {

class TabVisibilityWaiter : public content::WebContentsObserver {
 public:
  explicit TabVisibilityWaiter(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}

  TabVisibilityWaiter(const TabVisibilityWaiter&) = delete;
  TabVisibilityWaiter& operator=(const TabVisibilityWaiter&) = delete;

  ~TabVisibilityWaiter() override = default;

  void WaitForTabToBecomeVisible() {
    if (web_contents()->GetVisibility() == content::Visibility::VISIBLE) {
      return;
    }
    visibility_changed_run_loop_.Run();
  }

  // content::WebContentsObserver:
  void OnVisibilityChanged(content::Visibility visibility) override {
    if (visibility == content::Visibility::VISIBLE) {
      visibility_changed_run_loop_.Quit();
    }
  }

 private:
  base::RunLoop visibility_changed_run_loop_;
};

}  // namespace

using BraveTabStripModelTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveTabStripModelTest, MRUCyclingBasic) {
  TabStripModel* tab_strip_model = browser()->tab_strip_model();

  // Open 3 tabs
  chrome::NewTab(browser());
  chrome::NewTab(browser());
  EXPECT_EQ(tab_strip_model->count(), 3);
  EXPECT_EQ(tab_strip_model->active_index(), 2);

  // Before enabling MRU set up tab visibility waiter to make sure the activated
  // tab's WebContents becomes visible which will update the last active time on
  // which MRU relies to sort the tabs in MR order.
  TabVisibilityWaiter tab_visibility_waiter(
      tab_strip_model->GetWebContentsAt(0));
  // Normal next tab expected by default, 2 -> 0
  chrome::ExecuteCommand(browser(), IDC_SELECT_NEXT_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 0);
  tab_visibility_waiter.WaitForTabToBecomeVisible();

  // Activate MRU cycling
  browser()->profile()->GetPrefs()->SetBoolean(kMRUCyclingEnabled, true);

  // MRU cycling, 0 -> 2
  chrome::ExecuteCommand(browser(), IDC_SELECT_NEXT_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 2);
  // Ctrl is not being released 2 -> 1
  chrome::ExecuteCommand(browser(), IDC_SELECT_NEXT_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 1);
  // 1 -> 2
  chrome::ExecuteCommand(browser(), IDC_SELECT_PREVIOUS_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 2);
}

// Check MRU Cycling is restarted when tab is closed during the mru cycling.
// User can close current tab while cycling like this.
// For example on linux, when user does "Ctrl + tab -> Ctrl + F4 -> Ctrl + tab",
// second Ctrl + tab should restart mru cycling.
IN_PROC_BROWSER_TEST_F(BraveTabStripModelTest, TabClosingWhileMRUCycling) {
  // Activate MRU cycling
  browser()->profile()->GetPrefs()->SetBoolean(kMRUCyclingEnabled, true);

  TabStripModel* tab_strip_model = browser()->tab_strip_model();

  // Open 3 tabs. There's some kind of timing issue or flakiness on MacOS that
  // causes the tabs visibility not to be updated and the last active timestamp
  // doesn't get changed. So we force the visibility here.
  chrome::NewTab(browser());
  tab_strip_model->GetWebContentsAt(1)->WasShown();
  chrome::NewTab(browser());
  tab_strip_model->GetWebContentsAt(2)->WasShown();
  chrome::NewTab(browser());
  tab_strip_model->GetWebContentsAt(3)->WasShown();
  EXPECT_EQ(tab_strip_model->count(), 4);
  EXPECT_EQ(tab_strip_model->active_index(), 3);
  // MRU should be 3 > 2 > 1 > 0

  // MRU cycling, 3 -> 2
  chrome::ExecuteCommand(browser(), IDC_SELECT_NEXT_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 2);
  // Again, force visibility to update its last active timestamp.
  tab_strip_model->GetWebContentsAt(2)->WasShown();
  // MRU should be 2 > 1 > 0 > 3

  // MRU cycling, 2 -> 1
  chrome::ExecuteCommand(browser(), IDC_SELECT_NEXT_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 1);
  // MRU should be 1 > 0 > 3 > 2. No point in updating visibility since we are
  // going to close the tab.

  // Close current tab (index 1).
  chrome::ExecuteCommand(browser(), IDC_CLOSE_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 1);
  // Again, force visibility to update its last active timestamp.
  tab_strip_model->GetWebContentsAt(1)->WasShown();
  // MRU restarts and resorts tabs by last active timestamp.
  // MRU should be 1 (former 2) > 2 (former 3) > 0

  // New MRU cycling is started, 1 -> 2
  chrome::ExecuteCommand(browser(), IDC_SELECT_NEXT_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 2);
  // MRU should be: 2 > 0 > 1
}
