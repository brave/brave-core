/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view.h"

#include <string>

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/bookmark/bookmark_helper.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_user_gesture_details.h"
#include "chrome/browser/ui/test/test_browser_ui.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/scrim_view.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

class BraveBrowserViewTest : public InProcessBrowserTest {
 public:
  BraveBrowserViewTest() = default;
  ~BraveBrowserViewTest() override = default;

  BraveBrowserViewTest(const BraveBrowserViewTest&) = delete;
  BraveBrowserViewTest& operator=(const BraveBrowserViewTest&) = delete;

 protected:
  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view()->DeprecatedLayoutImmediately();
  }

  BrowserNonClientFrameView* browser_non_client_frame_view() {
    return browser_view()->frame()->GetFrameView();
  }

  BraveBrowserView* brave_browser_view() {
    return BraveBrowserView::From(browser_view());
  }

  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  views::View* vertical_tab_strip_host_view() {
    return brave_browser_view()->vertical_tab_strip_host_view_;
  }

  views::View* contents_container() {
    return browser_view()->GetContentsContainerForLayoutManager();
  }

  views::View* infobar_container() {
    return browser_view()->infobar_container();
  }

  BookmarkBarView* bookmark_bar() { return browser_view()->bookmark_bar(); }
};

// Tests bookmark/infobar/contents container layout with vertical tab.
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest, LayoutWithVerticalTabTest) {
  ToggleVerticalTabStrip();

  auto* prefs = browser()->profile()->GetPrefs();

  // Check bookmark only on the NTP is default.
  EXPECT_EQ(brave::BookmarkBarState::kNtp, brave::GetBookmarkBarState(prefs));

  // BookmarkBar not visible as current active tab is not NTP.
  EXPECT_FALSE(bookmark_bar()->GetVisible());

  // Infobar is visible at first run.
  // Update this test if it's not visible at first run.
  // Wait till infobar's positioning is finished.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return infobar_container()->GetVisible() &&
           (infobar_container()->bounds().bottom_left() ==
            contents_container()->bounds().origin());
  }));

  EXPECT_EQ(infobar_container()->bounds().bottom_left(),
            contents_container()->bounds().origin());

  // Bookmark bar should be visible with NTP.
  chrome::AddTabAt(browser(), GURL(), -1, true);
  EXPECT_TRUE(bookmark_bar()->GetVisible());
  EXPECT_FALSE(infobar_container()->GetVisible());
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !browser()->window()->IsBookmarkBarAnimating(); }));

  // Check bookmark bar/contents container position.
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().top_right(),
            bookmark_bar()->bounds().origin());
  EXPECT_EQ(bookmark_bar()->bounds().bottom_left() +
                gfx::Vector2d(0, /*top container separator*/ 1),
            contents_area_origin());

  // Hide bookmark bar always.
  // Check contents container is positioned right after the vertical tab.
  brave::SetBookmarkState(brave::BookmarkBarState::kNever, prefs);
  EXPECT_EQ(brave::BookmarkBarState::kNever, brave::GetBookmarkBarState(prefs));
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !browser()->window()->IsBookmarkBarAnimating(); }));
  EXPECT_FALSE(bookmark_bar()->GetVisible());
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().top_right(),
            contents_container()->bounds().origin());

  // Activate non-NTP tab and check contents container is positioned below the
  // infobar.
  browser()->tab_strip_model()->ActivateTabAt(0);
  EXPECT_TRUE(infobar_container()->GetVisible());
  EXPECT_EQ(infobar_container()->bounds().bottom_left(),
            contents_container()->bounds().origin());

  // Show bookmark bar always.
  // Check vertical tab is positioned below the bookmark bar.
  // Check contents container is positioned below the info bar.
  brave::SetBookmarkState(brave::BookmarkBarState::kAlways, prefs);
  EXPECT_EQ(brave::BookmarkBarState::kAlways,
            brave::GetBookmarkBarState(prefs));
  EXPECT_TRUE(infobar_container()->GetVisible());
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !browser()->window()->IsBookmarkBarAnimating(); }));
  EXPECT_TRUE(bookmark_bar()->GetVisible());
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().origin(),
            bookmark_bar()->bounds().bottom_left() +
                gfx::Vector2d(0, /*contents separator*/ 1));
  EXPECT_EQ(infobar_container()->bounds().bottom_left(),
            contents_container()->bounds().origin());

  // Activate NTP tab.
  // Check vertical tab is positioned below the bookmark bar.
  // Check contents container is positioned right after the vertical tab.
  browser()->tab_strip_model()->ActivateTabAt(1);
  EXPECT_FALSE(infobar_container()->GetVisible());
  EXPECT_TRUE(bookmark_bar()->GetVisible());
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().origin(),
            bookmark_bar()->bounds().bottom_left() +
                gfx::Vector2d(0, /*top container separator*/ 1));
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().top_right(),
            contents_container()->bounds().origin());
}

class BraveBrowserViewWithRoundedCornersTest
    : public BraveBrowserViewTest,
      public testing::WithParamInterface<std::tuple<bool, bool, bool>> {
 public:
  BraveBrowserViewWithRoundedCornersTest() {
    scoped_features_.InitWithFeatureStates(
        {{features::kBraveWebViewRoundedCorners, IsRoundedCornersEnabled()},
         {tabs::features::kBraveSplitView, IsBraveSplitViewEnabled()},
         {features::kSideBySide, IsSideBySideEnabled()}});
  }

  bool IsRoundedCornersEnabled() const { return std::get<0>(GetParam()); }

  bool IsBraveSplitViewEnabled() const { return std::get<1>(GetParam()); }

  bool IsSideBySideEnabled() const { return std::get<1>(GetParam()); }

 protected:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_P(BraveBrowserViewWithRoundedCornersTest,
                       ContentsBackgroundEventHandleTest) {
  if (!IsRoundedCornersEnabled()) {
    EXPECT_FALSE(brave_browser_view()->contents_background_view_);
    return;
  }

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  gfx::Point screen_point = web_contents->GetContainerBounds().CenterPoint();

  // Check contents background is not event handler for web contents region
  // point.
  views::View::ConvertPointFromScreen(browser_view(), &screen_point);
  EXPECT_NE(brave_browser_view()->contents_background_view_,
            browser_view()->GetEventHandlerForPoint(screen_point));
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    BraveBrowserViewWithRoundedCornersTest,
    testing::Values(std::make_tuple(false, false, false),
                    std::make_tuple(true, true, true),
                    std::make_tuple(true, true, false),
                    std::make_tuple(true, false, true),
                    std::make_tuple(true, false, false)));

// MacOS does not need views window scrim. We use sheet to show window modals
// (-[NSWindow beginSheet:]), which natively draws a scrim since macOS 11.
// Tests that a scrim is still disabled when a window modal dialog is active.
#if !BUILDFLAG(IS_MAC)
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       ScrimForBrowserWindowModalDisabledTest) {
  auto child_widget_delegate = std::make_unique<views::WidgetDelegate>();
  auto child_widget = std::make_unique<views::Widget>();
  child_widget_delegate->SetModalType(ui::mojom::ModalType::kWindow);
  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_WINDOW);
  params.delegate = child_widget_delegate.get();
  params.parent = browser_view()->GetWidget()->GetNativeView();
  child_widget->Init(std::move(params));

  // Check scrim view is always not visible.
  child_widget->Show();
  EXPECT_FALSE(browser_view()->window_scrim_view()->GetVisible());
  child_widget->Hide();
  EXPECT_FALSE(browser_view()->window_scrim_view()->GetVisible());
  child_widget->Show();
  EXPECT_FALSE(browser_view()->window_scrim_view()->GetVisible());
  child_widget.reset();
  EXPECT_FALSE(browser_view()->window_scrim_view()->GetVisible());
}
#endif  // !BUILDFLAG(IS_MAC)
