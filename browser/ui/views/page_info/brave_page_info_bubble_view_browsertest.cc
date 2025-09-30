/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_info/brave_page_info_bubble_view.h"

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/page_info/features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/location_bar/location_icon_view.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "chrome/browser/ui/views/page_info/page_info_view_factory.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "ui/events/test/test_event.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/view_utils.h"

namespace {

// Opens the page info bubble.
void OpenPageInfoBubble(Browser* browser) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  LocationIconView* location_icon_view =
      browser_view->toolbar()->location_bar()->location_icon_view();
  ASSERT_TRUE(location_icon_view);
  ui::test::TestEvent event;
  location_icon_view->ShowBubble(event);
  views::BubbleDialogDelegateView* page_info =
      PageInfoBubbleView::GetPageInfoBubbleForTesting();
  ASSERT_TRUE(page_info);
  page_info->set_close_on_deactivate(false);
}

BravePageInfoBubbleView* GetBubbleView() {
  return views::AsViewClass<BravePageInfoBubbleView>(
      PageInfoBubbleView::GetPageInfoBubbleForTesting());
}

}  // namespace

class BravePageInfoBubbleViewBrowserTest : public InProcessBrowserTest {
 public:
  BravePageInfoBubbleViewBrowserTest() {
    feature_list_.InitAndEnableFeature(
        page_info::features::kShowBraveShieldsInPageInfo);
  }

  ~BravePageInfoBubbleViewBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Test that the close button is hidden.
IN_PROC_BROWSER_TEST_F(BravePageInfoBubbleViewBrowserTest, CloseButtonHidden) {
  // Navigate to a test page.
  GURL test_url = embedded_test_server()->GetURL("/test.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  // Open the page info bubble.
  OpenPageInfoBubble(browser());
  auto* bubble_view = GetBubbleView();
  ASSERT_TRUE(bubble_view);

  // Verify that the close button exists but is not visible.
  views::View* close_button = bubble_view->GetViewByID(
      PageInfoViewFactory::VIEW_ID_PAGE_INFO_CLOSE_BUTTON);
  ASSERT_TRUE(close_button);
  EXPECT_FALSE(close_button->GetVisible());
}

class BravePageInfoBubbleViewFlagDisabledBrowserTest
    : public InProcessBrowserTest {
 public:
  BravePageInfoBubbleViewFlagDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(
        page_info::features::kShowBraveShieldsInPageInfo);
  }

  ~BravePageInfoBubbleViewFlagDisabledBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Test that the close button is visible when the feature flag is disabled.
IN_PROC_BROWSER_TEST_F(BravePageInfoBubbleViewFlagDisabledBrowserTest,
                       CloseButtonVisible) {
  // Navigate to a test page.
  GURL test_url = embedded_test_server()->GetURL("/test.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  // Open the page info bubble.
  OpenPageInfoBubble(browser());
  auto* bubble_view = GetBubbleView();
  ASSERT_TRUE(bubble_view);

  // Verify that the close button exists and is visible.
  views::View* close_button = bubble_view->GetViewByID(
      PageInfoViewFactory::VIEW_ID_PAGE_INFO_CLOSE_BUTTON);
  ASSERT_TRUE(close_button);
  EXPECT_TRUE(close_button->GetVisible());
}
