/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "brave/app/brave_command_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/base/models/menu_model.h"

namespace {

// Returns true if there exists a command with specified id in the given menu.
// False otherwise.
bool ContainsCommandIdInMenu(int command_id, const ui::MenuModel* menu) {
  CHECK(menu);
  for (size_t index = 0; index < menu->GetItemCount(); index++) {
    if (menu->GetCommandIdAt(index) == command_id) {
      return true;
    }
  }
  return false;
}

}  // namespace

using BraveNonClientHitTestHelperBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveNonClientHitTestHelperBrowserTest,
                       BraveSystemMenuByDefault) {
  // Retrieve system menu.
  const BrowserView* const browser_view =
      BrowserView::GetBrowserViewForBrowser(browser());
  const ui::MenuModel* const menu =
      browser_view->browser_widget()->GetSystemMenuModel();

  // Verify our system menu command availability.
  EXPECT_TRUE(ContainsCommandIdInMenu(IDC_TOGGLE_VERTICAL_TABS, menu));
}
