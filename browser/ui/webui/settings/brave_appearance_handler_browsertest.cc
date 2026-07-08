/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_appearance_handler.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_web_ui.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace settings {

// Exposes the protected hooks the test needs to drive the handler directly,
// the same way the WebUI framework would.
class TestBraveAppearanceHandler : public BraveAppearanceHandler {
 public:
  using BraveAppearanceHandler::set_web_ui;
  using content::WebUIMessageHandler::RegisterMessages;
};

class BraveAppearanceHandlerBrowserTest : public InProcessBrowserTest {
 protected:
  // Attaches a freshly created handler to `contents` and allows JavaScript,
  // which registers the IDC_TOGGLE_VERTICAL_TABS command observer.
  void AttachHandler(content::WebContents* contents) {
    ASSERT_TRUE(contents);
    // Precondition for the regression below: the tab must resolve to a real
    // BrowserCommandController via the exact path RegisterMessages() uses,
    // otherwise the handler caches a null command_updater_ and the test would
    // pass vacuously without ever exercising the use-after-free path.
    tabs::TabInterface* tab = tabs::TabInterface::GetFromContents(contents);
    ASSERT_TRUE(tab);
    ASSERT_TRUE(tab->GetBrowserWindowInterface()
                    ->GetFeatures()
                    .browser_command_controller());

    web_ui_.set_web_contents(contents);
    handler_.set_web_ui(&web_ui_);
    handler_.RegisterMessages();
    handler_.AllowJavascriptForTesting();
  }

  content::TestWebUI web_ui_;
  TestBraveAppearanceHandler handler_;
};

// Regression test for https://github.com/brave/brave-browser/issues/56696.
//
// On relaunch/shutdown the browser window (and its BrowserCommandController)
// is destroyed before the settings WebUI handler's OnJavascriptDisallowed()
// runs. Before the fix, the cached `command_updater_` raw_ptr dangled and
// OnJavascriptDisallowed() dereferenced freed memory (a use-after-free that
// raw_ptr/ASAN turns into a crash). The fix clears the pointer via
// TabInterface::RegisterWillDetach while the controller is still alive.
IN_PROC_BROWSER_TEST_F(BraveAppearanceHandlerBrowserTest,
                       NoUseAfterFreeWhenWindowTornDownBeforeDisallow) {
  // A dedicated window whose BrowserCommandController will be torn down while
  // the handler is still alive. The test's main browser() keeps the process up.
  Browser* browser2 = CreateBrowser(browser()->profile());
  ASSERT_TRUE(browser2);
  AttachHandler(browser2->tab_strip_model()->GetActiveWebContents());

  // Tearing the window down fires WillDetach (while the controller is alive),
  // which clears the cached command_updater_ pointer.
  CloseBrowserSynchronously(browser2);

  // Without the fix this dereferenced the freed BrowserCommandController. With
  // the fix command_updater_ is null, so this is a safe no-op.
  handler_.DisallowJavascript();

  EXPECT_FALSE(handler_.IsJavascriptAllowed());
}

// While the window stays alive, repeated allow/disallow cycles must keep
// AddCommandObserver/RemoveCommandObserver balanced (a double-remove or stale
// observer would DCHECK/crash).
IN_PROC_BROWSER_TEST_F(BraveAppearanceHandlerBrowserTest,
                       BalancedObserverAcrossAllowDisallowCycles) {
  AttachHandler(browser()->tab_strip_model()->GetActiveWebContents());

  handler_.DisallowJavascript();
  handler_.AllowJavascriptForTesting();
  handler_.DisallowJavascript();

  EXPECT_FALSE(handler_.IsJavascriptAllowed());
}

}  // namespace settings
