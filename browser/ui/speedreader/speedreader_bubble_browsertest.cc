/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "brave/components/speedreader/speedreader_service.h"
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
    auto* tab_helper =
        speedreader::SpeedreaderTabHelper::Get(ActiveWebContents());
    tab_helper->ShowBubble();
  }

 protected:
  bool NavigateToNewTab() {
    ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab"));
    return WaitForLoadStop(ActiveWebContents());
  }

  content::WebContents* ActiveWebContents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void ToggleSpeedreader() {
    auto* speedreader_service =
        speedreader::SpeedreaderServiceFactory::GetForProfile(
            browser()->profile());
    speedreader_service->ToggleSpeedreader();
  }
};

IN_PROC_BROWSER_TEST_F(SpeedreaderBubbleBrowserTest,
                       InvokeUi_reader_mode_bubble_basic) {
  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::Get(ActiveWebContents());
  EXPECT_FALSE(tab_helper->IsSpeedreaderEnabled());
  ShowAndVerifyUi();
}

IN_PROC_BROWSER_TEST_F(SpeedreaderBubbleBrowserTest,
                       InvokeUi_speedreader_mode_bubble_basic) {
  ToggleSpeedreader();
  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::Get(ActiveWebContents());
  EXPECT_TRUE(tab_helper->IsSpeedreaderEnabled());
  // We need to navigate somewhere so the host is non-empty. For tests the new
  // tab page is fine.
  NavigateToNewTab();
  const GURL active_url = ActiveWebContents()->GetLastCommittedURL();
  EXPECT_FALSE(active_url.host().empty());
  ShowAndVerifyUi();
}

}  // anonymous namespace
