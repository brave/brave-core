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
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_user_gesture_details.h"
#include "chrome/browser/ui/test/test_browser_ui.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "chrome/browser/ui/views/frame/scrim_view.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/views/view_class_properties.h"
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
    return browser_view()->browser_widget()->GetFrameView();
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

  views::View* main_container() { return browser_view()->main_container(); }

  views::View* contents_container() {
    return browser_view()->contents_container();
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
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return infobar_container()->GetVisible(); }));

  auto contents_area_origin = [&]() {
    return gfx::Point(contents_container()->bounds().x(),
                      main_container()->bounds().y());
  };

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return infobar_container()->bounds().bottom_left() ==
           contents_area_origin();
  }));

  // Bookmark bar should be visible with NTP.
  chrome::AddTabAt(browser(), GURL(), -1, true);
  EXPECT_TRUE(bookmark_bar()->GetVisible());
  EXPECT_FALSE(infobar_container()->GetVisible());
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !browser()->window()->IsBookmarkBarAnimating(); }));

  // Check bookmark bar/contents container position.
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().top_right(),
            bookmark_bar()->bounds().origin());
  EXPECT_EQ(bookmark_bar()->bounds().bottom_left(), contents_area_origin());

  // Hide bookmark bar always.
  // Check contents container is positioned right after the vertical tab.
  brave::SetBookmarkState(brave::BookmarkBarState::kNever, prefs);
  EXPECT_EQ(brave::BookmarkBarState::kNever, brave::GetBookmarkBarState(prefs));
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !browser()->window()->IsBookmarkBarAnimating(); }));
  EXPECT_FALSE(bookmark_bar()->GetVisible());
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().top_right(),
            contents_area_origin());

  // Activate non-NTP tab and check contents container is positioned below the
  // infobar.
  browser()->tab_strip_model()->ActivateTabAt(0);
  EXPECT_TRUE(infobar_container()->GetVisible());
  EXPECT_EQ(infobar_container()->bounds().bottom_left(),
            contents_area_origin());

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
            contents_area_origin());

  // Activate NTP tab.
  // Check vertical tab is positioned below the bookmark bar.
  // Check contents container is positioned right after the vertical tab.
  browser()->tab_strip_model()->ActivateTabAt(1);
  EXPECT_FALSE(infobar_container()->GetVisible());
  EXPECT_TRUE(bookmark_bar()->GetVisible());
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().origin(),
            bookmark_bar()->bounds().bottom_left());
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().top_right(),
            contents_area_origin());
}

class BraveBrowserViewWithRoundedCornersTest
    : public BraveBrowserViewTest,
      public testing::WithParamInterface<std::tuple<bool, bool>> {
 public:
  BraveBrowserViewWithRoundedCornersTest() {
    scoped_features_.InitWithFeatureStates(
        {{features::kBraveWebViewRoundedCorners, IsRoundedCornersEnabled()},
         {features::kSideBySide, IsSideBySideEnabled()}});
  }

  void NewSplitTab() {
    chrome::NewSplitTab(browser(),
                        split_tabs::SplitTabCreatedSource::kToolbarButton);
  }

  bool IsRoundedCornersEnabled() const { return std::get<0>(GetParam()); }
  bool IsSideBySideEnabled() const { return std::get<1>(GetParam()); }

 protected:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_P(BraveBrowserViewWithRoundedCornersTest,
                       ContentsBackgroundEventHandleTest) {
  EXPECT_TRUE(brave_browser_view()->contents_background_view_);

  EXPECT_EQ(brave_browser_view()->contents_background_view_->bounds(),
            brave_browser_view()->main_container()->bounds());

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  gfx::Point screen_point = web_contents->GetContainerBounds().CenterPoint();

  // Check contents background is not event handler for web contents region
  // point.
  views::View::ConvertPointFromScreen(browser_view(), &screen_point);
  EXPECT_NE(brave_browser_view()->contents_background_view_,
            browser_view()->GetEventHandlerForPoint(screen_point));
}

IN_PROC_BROWSER_TEST_P(BraveBrowserViewWithRoundedCornersTest,
                       RoundedCornersForContentsTest) {
  if (!IsSideBySideEnabled()) {
    return;
  }

  const gfx::AnimationTestApi::RenderModeResetter disable_rich_animations_ =
      gfx::AnimationTestApi::SetRichAnimationRenderMode(
          gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);

  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->Toggle();
  views::View* contents_container =
      browser_view()->GetContentsContainerForLayoutManager();
  views::View* side_panel = browser_view()->contents_height_side_panel();

  const auto contents_container_bounds = contents_container->bounds();
  const auto rounded_corners_margin = BraveContentsViewUtil::kMarginThickness;

  if (IsRoundedCornersEnabled()) {
    EXPECT_EQ(rounded_corners_margin,
              BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser()));
    EXPECT_EQ(0, side_panel->GetProperty(views::kMarginsKey)->left());
    EXPECT_EQ(rounded_corners_margin,
              side_panel->GetProperty(views::kMarginsKey)->bottom());
    EXPECT_EQ(rounded_corners_margin,
              side_panel->GetProperty(views::kMarginsKey)->right());
  } else {
    EXPECT_EQ(0,
              BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser()));
    EXPECT_EQ(0, side_panel->GetProperty(views::kMarginsKey)->left());
    EXPECT_EQ(0, side_panel->GetProperty(views::kMarginsKey)->bottom());
    EXPECT_EQ(0, side_panel->GetProperty(views::kMarginsKey)->right());
  }

  // Create split tab and check contents container has margin.
  NewSplitTab();
  EXPECT_EQ(rounded_corners_margin,
            BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser()));

  const auto contents_container_bounds_with_active_split_tab =
      contents_container->bounds();
  if (IsRoundedCornersEnabled()) {
    EXPECT_EQ(contents_container_bounds.bottom_left(),
              contents_container_bounds_with_active_split_tab.bottom_left());
    EXPECT_EQ(contents_container_bounds.width(),
              contents_container_bounds_with_active_split_tab.width());
    EXPECT_EQ(0, side_panel->GetProperty(views::kMarginsKey)->left());
    EXPECT_EQ(rounded_corners_margin,
              side_panel->GetProperty(views::kMarginsKey)->bottom());
    EXPECT_EQ(rounded_corners_margin,
              side_panel->GetProperty(views::kMarginsKey)->right());
  } else {
    EXPECT_EQ(
        contents_container_bounds.bottom_left() +
            gfx::Vector2d(rounded_corners_margin, -rounded_corners_margin),
        contents_container_bounds_with_active_split_tab.bottom_left());
    EXPECT_EQ(contents_container_bounds.width(),
              contents_container_bounds_with_active_split_tab.width() +
                  rounded_corners_margin * 2);
    EXPECT_EQ(0, side_panel->GetProperty(views::kMarginsKey)->left());
    EXPECT_EQ(rounded_corners_margin,
              side_panel->GetProperty(views::kMarginsKey)->bottom());
    EXPECT_EQ(rounded_corners_margin,
              side_panel->GetProperty(views::kMarginsKey)->right());
  }

  // Create new active tab to not have split tab as a active tab.
  // Check contents container doesn't have margin when rounded corners is
  // disabled.
  chrome::AddTabAt(browser(), GURL(), -1, true);
  if (IsRoundedCornersEnabled()) {
    EXPECT_EQ(contents_container_bounds.bottom_left(),
              contents_container->bounds().bottom_left());
    EXPECT_EQ(contents_container_bounds.width(),
              contents_container->bounds().width());
    EXPECT_EQ(0, side_panel->GetProperty(views::kMarginsKey)->left());
    EXPECT_EQ(rounded_corners_margin,
              side_panel->GetProperty(views::kMarginsKey)->bottom());
    EXPECT_EQ(rounded_corners_margin,
              side_panel->GetProperty(views::kMarginsKey)->right());
  } else {
    EXPECT_EQ(contents_container_bounds.bottom_left(),
              contents_container->bounds().bottom_left());
    // Find the reason why final width is not set directly.
    ASSERT_TRUE(base::test::RunUntil([&]() {
      return contents_container_bounds.width() ==
             contents_container->bounds().width();
    }));
    EXPECT_EQ(0, side_panel->GetProperty(views::kMarginsKey)->left());
    EXPECT_EQ(0, side_panel->GetProperty(views::kMarginsKey)->bottom());
    EXPECT_EQ(0, side_panel->GetProperty(views::kMarginsKey)->right());
  }
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    BraveBrowserViewWithRoundedCornersTest,
    ::testing::Combine(::testing::Bool(), ::testing::Bool()));

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
