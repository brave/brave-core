/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/pref_names.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

using BraveTabStripModelTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveTabStripModelTest, MRUCyclingBasic) {
  TabStripModel* tab_strip_model = browser()->tab_strip_model();

  // Open 3 tabs
  chrome::NewTab(browser());
  chrome::NewTab(browser());
  EXPECT_EQ(browser()->tab_strip_model()->count(), 3);
  EXPECT_EQ(browser()->tab_strip_model()->active_index(), 2);

  // Normal next tab expected by default, 2 -> 0
  chrome::ExecuteCommand(browser(), IDC_SELECT_NEXT_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 0);

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

  // Open 3 tabs
  chrome::NewTab(browser());
  chrome::NewTab(browser());
  chrome::NewTab(browser());
  EXPECT_EQ(browser()->tab_strip_model()->count(), 4);
  EXPECT_EQ(browser()->tab_strip_model()->active_index(), 3);

  // MRU cycling, 3 -> 2
  chrome::ExecuteCommand(browser(), IDC_SELECT_NEXT_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 2);

  // MRU cycling, 2 -> 1
  chrome::ExecuteCommand(browser(), IDC_SELECT_NEXT_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 1);

  // Close current tab (index 1).
  chrome::ExecuteCommand(browser(), IDC_CLOSE_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 1);

  // New MRU cycling is started
  chrome::ExecuteCommand(browser(), IDC_SELECT_NEXT_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 2);
}
