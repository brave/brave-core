/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/pref_names.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

using BraveBrowserViewTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest, MRUCyclingBasic) {
  TabStripModel* tab_strip_model = browser()->tab_strip_model();

  // Open 3 tabs
  chrome::NewTab(browser());
  chrome::NewTab(browser());
  EXPECT_EQ(browser()->tab_strip_model()->count(), 3);
  EXPECT_EQ(browser()->tab_strip_model()->active_index(), 2);

  // Normal next tab expected by default, 2 -> 0
  chrome::ExecuteCommand(browser(), IDC_SELECT_NEXT_TAB);
  EXPECT_EQ(tab_strip_model->active_index(), 0);

  // // Activate MRU cycling
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
