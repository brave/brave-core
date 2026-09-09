/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check_is_test.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/web_contents.h"

namespace {

// We need to update sidebar UI only when the active tab's URL changes.
void MaybeUpdateSidebarForUrlChange(BrowserWindowInterface& browser,
                                    TabStripModel& tab_strip_model,
                                    content::WebContents* source,
                                    unsigned changed_flags) {
  if (!(changed_flags & content::INVALIDATE_TYPE_URL) ||
      source != tab_strip_model.GetActiveWebContents()) {
    return;
  }

  // sidebar_controller() can return a nullptr in unit tests.
  auto* sidebar_controller = browser.GetFeatures().sidebar_controller();
  if (!sidebar_controller) {
    return;
  }

  if (sidebar_controller->sidebar()) {
    sidebar_controller->sidebar()->UpdateSidebarItemsState();
  } else {
    CHECK_IS_TEST();
  }
}

}  // namespace

#include <chrome/browser/ui/browser_ui_controller/browser_ui_controller.cc>
