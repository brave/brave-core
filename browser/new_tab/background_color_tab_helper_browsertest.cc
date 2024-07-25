// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/scoped_feature_list.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

class BackgroundColorTabHelperBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  BackgroundColorTabHelperBrowserTest() {
    dark_mode::SetUseSystemDarkModeEnabledForTest(false);
    scoped_feature_list_.InitAndEnableFeature(
        features::kBraveWorkaroundNewWindowFlash);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    dark_mode::SetBraveDarkModeType(
        IsDarkMode()
            ? dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK
            : dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  bool IsDarkMode() { return GetParam(); }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(BackgroundColorTabHelperBrowserTest,
                       BackgroundColorIsSet) {
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabPageURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  const auto view_background_color =
      web_contents()->GetPrimaryMainFrame()->GetView()->GetBackgroundColor();

  if (IsDarkMode()) {
    EXPECT_NE(view_background_color, SK_ColorWHITE);
  } else {
    EXPECT_EQ(view_background_color, SK_ColorWHITE);
  }
}

INSTANTIATE_TEST_SUITE_P(,
                         BackgroundColorTabHelperBrowserTest,
                         testing::Bool());
