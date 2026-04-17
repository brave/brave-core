/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/tab_search_feature.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/actions/actions.h"

using BraveBrowserActionsBrowserTest = InProcessBrowserTest;

// Regression test for https://github.com/brave/brave-browser/issues/54449
//
// kActionTabSearch must be kPinnable immediately after InitializeBrowserActions
// so that ToolbarController::GetDefaultResponsiveElements() includes it in
// responsive_elements_. Without this, the overflow menu is empty when
// kActionTabSearch overflows, causing a crash on click.
IN_PROC_BROWSER_TEST_F(BraveBrowserActionsBrowserTest,
                       TabSearchActionIsPinnableAfterInit) {
  ASSERT_TRUE(features::HasTabSearchToolbarButton())
      << "Tab search toolbar button not enabled";

  auto& action_manager = actions::ActionManager::GetForTesting();
  auto* tab_search_action = action_manager.FindAction(kActionTabSearch);
  ASSERT_NE(tab_search_action, nullptr)
      << "kActionTabSearch not found in ActionManager";

  const actions::ActionPinnableState pinnable_state =
      static_cast<actions::ActionPinnableState>(
          tab_search_action->GetProperty(actions::kActionItemPinnableKey));
  EXPECT_EQ(pinnable_state, actions::ActionPinnableState::kPinnable)
      << "kActionTabSearch must be kPinnable so ToolbarController includes it "
         "in responsive_elements_";
}
