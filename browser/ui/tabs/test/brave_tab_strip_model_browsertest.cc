/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"

#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/public/constants.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_live_tab_context.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_renderer_data.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
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

class BraveTabStripModelRenamingTabBrowserTest : public BraveTabStripModelTest {
 public:
  BraveTabStripModelRenamingTabBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        tabs::features::kBraveRenamingTabs);
  }
  ~BraveTabStripModelRenamingTabBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveTabStripModelRenamingTabBrowserTest,
                       SettingCustomTabTitle_TabRendererDataUpdated) {
  BraveTabStripModel* tab_strip_model =
      static_cast<BraveTabStripModel*>(browser()->tab_strip_model());
  ASSERT_EQ(TabRendererData::FromTabInModel(tab_strip_model, 0).title,
            u"about:blank");

  tab_strip_model->SetCustomTitleForTab(0, u"Custom Title");
  EXPECT_EQ(TabRendererData::FromTabInModel(tab_strip_model, 0).title,
            u"Custom Title");

  tab_strip_model->SetCustomTitleForTab(0, u"");
  EXPECT_EQ(TabRendererData::FromTabInModel(tab_strip_model, 0).title,
            u"about:blank");
}

IN_PROC_BROWSER_TEST_F(BraveTabStripModelRenamingTabBrowserTest,
                       SettingCustomTabTitle_LiveTabShouldHaveExtraData) {
  ASSERT_TRUE(browser()->live_tab_context()->GetExtraDataForTab(0).empty());

  BraveTabStripModel* tab_strip_model =
      static_cast<BraveTabStripModel*>(browser()->tab_strip_model());
  tab_strip_model->SetCustomTitleForTab(0, u"Custom Title");

  auto extra_data = browser()->live_tab_context()->GetExtraDataForTab(0);

  auto custom_tab_it = extra_data.find(tabs::kBraveTabCustomTitleExtraDataKey);
  ASSERT_NE(custom_tab_it, extra_data.end());
  EXPECT_EQ(custom_tab_it->second, "Custom Title");
}

IN_PROC_BROWSER_TEST_F(BraveTabStripModelRenamingTabBrowserTest,
                       SettingCustomTabTitle_Session) {
  SessionServiceBase* session_service =
      SessionServiceFactory::GetForProfileIfExisting(browser()->profile());
  auto* command_storage_manager = session_service->command_storage_manager();
  command_storage_manager->Save();
  ASSERT_TRUE(command_storage_manager->pending_commands().empty());

  BraveTabStripModel* tab_strip_model =
      static_cast<BraveTabStripModel*>(browser()->tab_strip_model());
  tab_strip_model->SetCustomTitleForTab(0, u"Custom Title");

  const auto& pending_commands = command_storage_manager->pending_commands();
  EXPECT_EQ(1u, pending_commands.size());
  EXPECT_EQ(/*kCommandAddTabExtraData*/ 33, pending_commands[0]->id());
}
