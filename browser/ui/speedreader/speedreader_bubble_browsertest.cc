/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/speedreader/speedreader_bubble_controller.h"
#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "brave/browser/ui/views/speedreader/speedreader_bubble_global.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"

namespace {
class SpeedreaderBubbleBrowserTest : public DialogBrowserTest {
 public:
  SpeedreaderBubbleBrowserTest() = default;
  SpeedreaderBubbleBrowserTest(const SpeedreaderBubbleBrowserTest&) = delete;
  SpeedreaderBubbleBrowserTest& operator=(const SpeedreaderBubbleBrowserTest&) =
      delete;

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    auto* bubble_controller =
        speedreader::SpeedreaderBubbleController::Get(web_contents);
    bubble_controller->ShowBubble(is_enabled_);  // fixme virtual
  }

 protected:
  bool NavigateToNewTab() {
    ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab"));
    return WaitForLoadStop(
        browser()->tab_strip_model()->GetActiveWebContents());
  }

  void SetEnabled() { is_enabled_ = true; }

 private:
  bool is_enabled_ = false;  // is speedreader enabled
};

IN_PROC_BROWSER_TEST_F(SpeedreaderBubbleBrowserTest,
                       InvokeUi_speedreader_single_page_basic) {
  ShowAndVerifyUi();
}

IN_PROC_BROWSER_TEST_F(SpeedreaderBubbleBrowserTest,
                       InvokeUi_speedreader_global_basic) {
  SetEnabled();
  // We need to navigate somewhere so the origin is non-empty. For tests the new
  // tab page is fine.
  NavigateToNewTab();
  ShowAndVerifyUi();
}

}  // anonymous namespace
