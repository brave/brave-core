// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/scoped_feature_list.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/compositor.h"
#include "ui/gfx/native_widget_types.h"

class BackgroundColorTabHelperBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  BackgroundColorTabHelperBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        features::kBraveWorkaroundNewWindowFlash);
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  bool IsDarkMode() { return GetParam(); }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(BackgroundColorTabHelperBrowserTest,
                       PRE_BackgroundColorIsSet) {
  dark_mode::SetBraveDarkModeType(
      IsDarkMode() ? dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK
                   : dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
}

IN_PROC_BROWSER_TEST_P(BackgroundColorTabHelperBrowserTest,
                       BackgroundColorIsSet) {
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabPageURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Expected colors:
  const auto expected_tab_background_color =
      web_contents()->GetColorProvider().GetColor(kColorNewTabPageBackground);

  const auto expected_view_host_background_color =
      web_contents()->GetColorProvider().GetColor(kColorToolbar);

  // Actual colors:
  const auto tab_background_color =
      web_contents()->GetTopLevelRenderWidgetHostView()->GetBackgroundColor();

  const auto view_host_background_color =
      BrowserView::GetBrowserViewForBrowser(browser())
          ->GetWidget()
          ->GetNativeView()
          ->GetHost()
          ->compositor()
          ->host_for_testing()
          ->background_color()
          .toSkColor();

  EXPECT_EQ(tab_background_color, expected_tab_background_color);
  EXPECT_EQ(view_host_background_color, expected_view_host_background_color);
}

INSTANTIATE_TEST_SUITE_P(,
                         BackgroundColorTabHelperBrowserTest,
                         testing::Bool());
