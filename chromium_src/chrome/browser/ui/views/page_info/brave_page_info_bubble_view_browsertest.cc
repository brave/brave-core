// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/location_bar/location_icon_view.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "chrome/browser/ui/views/page_info/page_info_view_factory.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/strings/grit/components_branded_strings.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/test/test_event.h"

namespace {

// Clicks the location icon to open the page info bubble.
void OpenPageInfoBubble(Browser* browser) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  ASSERT_TRUE(browser_view);
  LocationIconView* location_icon_view =
      browser_view->toolbar()->location_bar()->location_icon_view();
  ASSERT_TRUE(location_icon_view);
  ui::test::TestEvent event;
  location_icon_view->ShowBubble(event);
  views::BubbleDialogDelegateView* page_info =
      PageInfoBubbleView::GetPageInfoBubbleForTesting();
  EXPECT_NE(nullptr, page_info);
  page_info->set_close_on_deactivate(false);
}

}  // namespace

class BravePageInfoBubbleViewBrowserTest : public InProcessBrowserTest {
 public:
  BravePageInfoBubbleViewBrowserTest() = default;
  ~BravePageInfoBubbleViewBrowserTest() override = default;
  BravePageInfoBubbleViewBrowserTest(
      const BravePageInfoBubbleViewBrowserTest& test) = delete;
  BravePageInfoBubbleViewBrowserTest& operator=(
      const BravePageInfoBubbleViewBrowserTest& test) = delete;

  void SetUp() override { InProcessBrowserTest::SetUp(); }
};

IN_PROC_BROWSER_TEST_F(BravePageInfoBubbleViewBrowserTest, BraveURL) {
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("brave://settings")));
  OpenPageInfoBubble(browser());
  EXPECT_EQ(PageInfoBubbleView::BUBBLE_INTERNAL_PAGE,
            PageInfoBubbleView::GetShownBubbleType());
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_INTERNAL_PAGE),
      PageInfoBubbleView::GetPageInfoBubbleForTesting()->GetWindowTitle());
}
