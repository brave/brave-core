/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/test/pinned_tab_reset_browsertest.h"

#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include "base/test/run_until.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "content/public/test/browser_test.h"

IN_PROC_BROWSER_TEST_F(PinnedTabResetBrowserTest, CommandWResetsPinnedTab) {
  ASSERT_NO_FATAL_FAILURE(SetUpPinnedTabNavigatedAway());

  auto* browser_view =
      static_cast<BrowserView*>(BrowserWindow::FromBrowser(browser()));
  ASSERT_TRUE(ui_test_utils::ShowAndFocusNativeWindow(
      browser_view->GetWidget()->GetNativeWindow()));
  auto* ns_window =
      browser_view->GetWidget()->GetNativeWindow().GetNativeNSWindow();
  ASSERT_TRUE(base::test::RunUntil([&] { return [ns_window isKeyWindow]; }));

  NSEvent* event = [NSEvent keyEventWithType:NSEventTypeKeyDown
                                    location:NSMakePoint(0, 0)
                               modifierFlags:NSEventModifierFlagCommand
                                   timestamp:0
                                windowNumber:[ns_window windowNumber]
                                     context:nil
                                  characters:@"w"
                 charactersIgnoringModifiers:@"w"
                                   isARepeat:NO
                                     keyCode:kVK_ANSI_W];
  // Dispatch to the window directly so the key equivalent is handled with
  // `is_before_first_responder`, as for a real key press.
  EXPECT_TRUE([ns_window performKeyEquivalent:event]);

  EXPECT_EQ(2, model()->count());
  EXPECT_TRUE(model()->IsTabPinned(0));
  EXPECT_EQ(1, model()->active_index());
  ExpectPinnedTabReset();
}
