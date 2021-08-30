/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "url/gurl.h"

namespace {
class SpeedreaderBubbleBrowserTest : public DialogBrowserTest {
 public:
  SpeedreaderBubbleBrowserTest() = default;
  SpeedreaderBubbleBrowserTest(const SpeedreaderBubbleBrowserTest&) = delete;
  SpeedreaderBubbleBrowserTest& operator=(const SpeedreaderBubbleBrowserTest&) =
      delete;

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    if (speedreader_bubble_)
      tab_helper()->ShowSpeedreaderBubble();
    else
      tab_helper()->ShowReaderModeBubble();
  }

 protected:
  bool NavigateToNewTab() {
    ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab"));
    return WaitForLoadStop(ActiveWebContents());
  }

  content::WebContents* ActiveWebContents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  speedreader::SpeedreaderTabHelper* tab_helper() {
    speedreader::SpeedreaderTabHelper::CreateForWebContents(
        ActiveWebContents());
    return speedreader::SpeedreaderTabHelper::FromWebContents(
        ActiveWebContents());
  }

  bool speedreader_bubble_ = false;
};

IN_PROC_BROWSER_TEST_F(SpeedreaderBubbleBrowserTest,
                       InvokeUi_reader_mode_bubble_basic) {
  speedreader_bubble_ = false;
  ShowAndVerifyUi();
}

IN_PROC_BROWSER_TEST_F(SpeedreaderBubbleBrowserTest,
                       InvokeUi_speedreader_mode_bubble_basic) {
  speedreader_bubble_ = true;
  // We need to navigate somewhere so the host is non-empty. For tests the new
  // tab page is fine.
  NavigateToNewTab();
  const GURL active_url = ActiveWebContents()->GetLastCommittedURL();
  EXPECT_FALSE(active_url.host().empty());
  ShowAndVerifyUi();
}

}  // anonymous namespace
