/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/test/shared_pinned_tab_service_browsertest.h"

#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/toasts/toast_features.h"
#import "chrome/browser/ui/views/frame/browser_native_widget_mac.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/interactive_test_utils.h"

IN_PROC_BROWSER_TEST_F(SharedPinnedTabServiceBrowserTest,
                       CloseTabShortCutShouldBeDisabled) {
  auto* browser = CreateNewBrowser();
  chrome::NewTab(browser, NewTabTypes::kNewTabCommand);

  EXPECT_EQ(browser->tab_strip_model()->count(), 2);
  EXPECT_EQ(browser->tab_strip_model()->active_index(), 1);

  EXPECT_EQ(browser->tab_strip_model()->SetTabPinned(1, true), 0);
  EXPECT_EQ(browser->tab_strip_model()->active_index(), 0);

  auto* browser_view =
      static_cast<BrowserView*>(BrowserWindow::FromBrowser(browser));

  // Make the window key so the "Close Tab" menu item is validated against it.
  ASSERT_TRUE(ui_test_utils::ShowAndFocusNativeWindow(
      browser_view->GetWidget()->GetNativeWindow()));

  auto* ns_window =
      browser_view->GetWidget()->GetNativeWindow().GetNativeNSWindow();

  // When Command + w is pressed
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
  // Dispatch to the window directly instead of via -[NSApplication sendEvent:].
  // The latter falls through to the main menu when the window isn't key at that
  // instant, which runs Close Tab without `is_before_first_responder` and so
  // bypasses the guard under test.
  EXPECT_TRUE([ns_window performKeyEquivalent:event]);

  // Then the tab should not be closed.
  EXPECT_EQ(browser->tab_strip_model()->count(), 2);

  // When other ways to close the tab are tried
  chrome::ExecuteCommand(browser, IDC_CLOSE_TAB);

  // Then tab should be closed
  EXPECT_EQ(browser->tab_strip_model()->count(), 1);
}
