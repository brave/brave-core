/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view.h"

#include <string>

#include "base/functional/callback.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/bookmark/bookmark_helper.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/frame/layout/brave_browser_view_layout_delegate_impl.h"
#include "brave/browser/ui/views/frame/layout/brave_browser_view_tabbed_layout_impl.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_bubble_type.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_user_gesture_details.h"
#include "chrome/browser/ui/test/test_browser_ui.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/frame/browser_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "chrome/browser/ui/views/frame/scrim_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

using views::ShapeContextTokensOverride::kRoundedCornersBorderRadius;
using views::ShapeContextTokensOverride::
    kRoundedCornersBorderRadiusAtWindowCorner;

namespace {

// Delegate that reports no visible top UI (toolbar, bookmark bar) for testing
// GetTopSeparatorType() returns kNone. Subclasses Brave's delegate and only
// overrides the two visibility methods.
class NoVisibleTopUIDelegateForTesting
    : public BraveBrowserViewLayoutDelegateImpl {
 public:
  using BraveBrowserViewLayoutDelegateImpl::BraveBrowserViewLayoutDelegateImpl;

  bool IsToolbarVisible() const override { return false; }
  bool IsBookmarkBarVisible() const override { return false; }
};

}  // namespace

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

  BrowserFrameView* browser_non_client_frame_view() {
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

  const bool rounded_corners =
      BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
          browser());

  // With rounded corners, we need
  // distance(tabs::kMarginForVerticalTabContainers) between vertical tab and
  // contents. Each side has half of it as a margin.
  const int contents_margin =
      rounded_corners ? (tabs::kMarginForVerticalTabContainers / 2) : 0;
  const int top_contents_separator_height = rounded_corners ? 0 : 1;

  auto contents_area_origin = [&]() {
    return gfx::Point(
        browser_view()->contents_container()->bounds().origin().x() -
            contents_margin,
        browser_view()->contents_container()->bounds().origin().y());
  };

#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  // Infobar is visible at first run (P3A notice).
  // Not shown on Origin builds where P3A is disabled by default.
  // Wait till infobar's positioning is finished.
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return infobar_container()->GetVisible(); }));

  // The layout of the Main Container is currently:
  // |----------------------------------------------------------|
  // | Top container                                            |
  // |----------------------------------------------------------|
  // |  Vertical  | Info bar                                    |
  // |  Tab Strip |---------------------------------------------|
  // |            | Contents area                               |

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return infobar_container()->bounds().bottom_left() ==
           contents_area_origin();
  }));
#endif  // !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)

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
                gfx::Vector2d(0, top_contents_separator_height),
            contents_area_origin());

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
#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  EXPECT_TRUE(infobar_container()->GetVisible());
  EXPECT_EQ(infobar_container()->bounds().bottom_left(),
            contents_area_origin());
#endif  // !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)

  // Show bookmark bar always.
  // Check vertical tab is positioned below the bookmark bar.
  // Check contents container is positioned below the info bar.
  brave::SetBookmarkState(brave::BookmarkBarState::kAlways, prefs);
  EXPECT_EQ(brave::BookmarkBarState::kAlways,
            brave::GetBookmarkBarState(prefs));
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !browser()->window()->IsBookmarkBarAnimating(); }));
  EXPECT_TRUE(bookmark_bar()->GetVisible());
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().origin(),
            bookmark_bar()->bounds().bottom_left() +
                gfx::Vector2d(0, top_contents_separator_height));
#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  EXPECT_TRUE(infobar_container()->GetVisible());
  EXPECT_EQ(infobar_container()->bounds().bottom_left(),
            contents_area_origin());
#endif  // !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)

  // Activate NTP tab.
  // Check vertical tab is positioned below the bookmark bar.
  // Check contents container is positioned right after the vertical tab.
  browser()->tab_strip_model()->ActivateTabAt(1);
  EXPECT_FALSE(infobar_container()->GetVisible());
  EXPECT_TRUE(bookmark_bar()->GetVisible());
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().origin(),
            bookmark_bar()->bounds().bottom_left() +
                gfx::Vector2d(0, top_contents_separator_height));
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().top_right(),
            contents_area_origin());
}

class BraveBrowserViewWithRoundedCornersTest
    : public BraveBrowserViewTest,
      public testing::WithParamInterface<bool> {
 public:
  BraveBrowserViewWithRoundedCornersTest()
      : disable_rich_animations_(
            gfx::AnimationTestApi::SetRichAnimationRenderMode(
                gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED)) {}

  void SetUpOnMainThread() override {
    BraveBrowserViewTest::SetUpOnMainThread();
    browser()->profile()->GetPrefs()->SetBoolean(kWebViewRoundedCorners,
                                                 IsRoundedCornersEnabled());
  }

  void NewSplitTab() {
    chrome::NewSplitTab(browser(),
                        split_tabs::SplitTabCreatedSource::kToolbarButton);
  }

  BrowserFrameView* browser_non_client_frame_view() {
    return browser_view()->browser_widget()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view()->DeprecatedLayoutImmediately();
  }

  void SetHideCompletelyWhenCollapsed(bool hide) {
    browser()->profile()->GetPrefs()->SetBoolean(
        brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed, hide);
  }

  BraveVerticalTabStripRegionView* vertical_tab_strip_region() {
    auto* widget_delegate_view =
        BraveBrowserView::From(browser_view())
            ->vertical_tab_strip_widget_delegate_view();

    return widget_delegate_view->vertical_tab_strip_region_view();
  }

  bool IsRoundedCornersEnabled() const { return GetParam(); }

 protected:
  // Helper methods to get metrics
  int GetRoundedCornersBorderRadius() const {
    return views::LayoutProvider::Get()->GetCornerRadiusMetric(
        kRoundedCornersBorderRadius);
  }

  int GetRoundedCornersBorderRadiusAtWindowCorner() const {
    return views::LayoutProvider::Get()->GetCornerRadiusMetric(
        kRoundedCornersBorderRadiusAtWindowCorner);
  }

  // Helper methods for common assertions
  void ExpectContentsContainerMargins(views::View* contents_container,
                                      int expected_margin) {
    EXPECT_EQ(contents_container->bounds().x() - expected_margin,
              browser_view()->GetLocalBounds().x());
    EXPECT_EQ(contents_container->bounds().bottom() + expected_margin,
              browser_view()->GetLocalBounds().bottom());
  }

  void ExpectSidePanelMargins(views::View* side_panel,
                              int bottom_margin,
                              int right_margin) {
    EXPECT_EQ(0, side_panel->GetProperty(views::kMarginsKey)->left());
    EXPECT_EQ(bottom_margin,
              side_panel->GetProperty(views::kMarginsKey)->bottom());
    EXPECT_EQ(right_margin,
              side_panel->GetProperty(views::kMarginsKey)->right());
  }

  void ExpectContentsViewRadii(float upper_left,
                               float upper_right,
                               float lower_left,
                               float lower_right) {
    auto radii = browser_view()
                     ->GetActiveContentsContainerView()
                     ->contents_view()
                     ->GetBackgroundRadii();
    EXPECT_EQ(upper_left, radii.upper_left());
    EXPECT_EQ(upper_right, radii.upper_right());
    EXPECT_EQ(lower_left, radii.lower_left());
    EXPECT_EQ(lower_right, radii.lower_right());
  }

  void ExpectSplitContentsViewRadii(int index,
                                    float lower_left,
                                    float lower_right) {
    auto radii = browser_view()
                     ->GetContentsContainerViews()[index]
                     ->contents_view()
                     ->GetBackgroundRadii();
    EXPECT_EQ(lower_left, radii.lower_left());
    EXPECT_EQ(lower_right, radii.lower_right());
  }

 private:
  const gfx::AnimationTestApi::RenderModeResetter disable_rich_animations_;
};

// Test 1: Rounded corners behavior with side panel toggled
IN_PROC_BROWSER_TEST_P(BraveBrowserViewWithRoundedCornersTest,
                       RoundedCornersWithSidePanelTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->Toggle();
  RunScheduledLayouts();

  views::View* contents_container = browser_view()->contents_container();
  views::View* side_panel = browser_view()->contents_height_side_panel();
  const auto rounded_corners_margin = BraveContentsViewUtil::kMarginThickness;
  const auto rounded_corners_border_radius = GetRoundedCornersBorderRadius();
  const auto rounded_corners_border_radius_at_window_corner =
      GetRoundedCornersBorderRadiusAtWindowCorner();

  if (IsRoundedCornersEnabled()) {
    // Check contents container has margin
    ExpectContentsContainerMargins(contents_container, rounded_corners_margin);
    EXPECT_EQ(rounded_corners_margin,
              BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser()));

    // Check side panel margins and radii
    ExpectSidePanelMargins(side_panel, rounded_corners_margin,
                           rounded_corners_margin);
    EXPECT_EQ(gfx::RoundedCornersF(rounded_corners_border_radius),
              side_panel->layer()->rounded_corner_radii());

    // Check contents view radii
    const auto contents_view_radii = browser_view()
                                         ->GetActiveContentsContainerView()
                                         ->contents_view()
                                         ->GetBackgroundRadii();
    EXPECT_EQ(BraveContentsViewUtil::GetRoundedCornersForContentsView(browser(),
                                                                      nullptr),
              contents_view_radii);
    EXPECT_EQ(rounded_corners_border_radius, contents_view_radii.upper_left());
    EXPECT_EQ(rounded_corners_border_radius, contents_view_radii.upper_right());

    // lower-left radius should be radius around window as there is no ui
    // between browser window border and contents.
    EXPECT_EQ(rounded_corners_border_radius_at_window_corner,
              contents_view_radii.lower_left());
    EXPECT_EQ(rounded_corners_border_radius, contents_view_radii.lower_right());
  } else {
    // Check contents container doesn't have any margin
    EXPECT_EQ(contents_container->bounds().x(),
              browser_view()->GetLocalBounds().x());
    EXPECT_EQ(contents_container->bounds().bottom(),
              browser_view()->GetLocalBounds().bottom());
    EXPECT_EQ(0,
              BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser()));

    // Check side panel has no margins
    ExpectSidePanelMargins(side_panel, 0, 0);

    // Panel doesn't have layer when its shadow is not set.
    EXPECT_FALSE(side_panel->layer());
    EXPECT_EQ(gfx::RoundedCornersF(), browser_view()
                                          ->GetActiveContentsContainerView()
                                          ->contents_view()
                                          ->GetBackgroundRadii());
  }
}

// Test 2: Rounded corners in split tab mode
IN_PROC_BROWSER_TEST_P(BraveBrowserViewWithRoundedCornersTest,
                       RoundedCornersWithSplitTabTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->Toggle();
  RunScheduledLayouts();

  // Create split tab
  NewSplitTab();
  RunScheduledLayouts();

  views::View* contents_container = browser_view()->contents_container();
  views::View* side_panel = browser_view()->contents_height_side_panel();
  const auto rounded_corners_margin = BraveContentsViewUtil::kMarginThickness;
  const auto rounded_corners_border_radius = GetRoundedCornersBorderRadius();
  const auto rounded_corners_border_radius_at_window_corner =
      GetRoundedCornersBorderRadiusAtWindowCorner();

  // Margins are always applied in split tab mode
  EXPECT_EQ(rounded_corners_margin,
            BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser()));
  ExpectContentsContainerMargins(contents_container, rounded_corners_margin);

  // Check side panel margins and radii
  ExpectSidePanelMargins(side_panel, rounded_corners_margin,
                         rounded_corners_margin);
  EXPECT_EQ(gfx::RoundedCornersF(rounded_corners_border_radius),
            side_panel->layer()->rounded_corner_radii());

  // Check radii for both start (left) and end (right) contents views
  ExpectSplitContentsViewRadii(0,
                               rounded_corners_border_radius_at_window_corner,
                               rounded_corners_border_radius);
  ExpectSplitContentsViewRadii(1, rounded_corners_border_radius,
                               rounded_corners_border_radius);
}

// Test 3: Rounded corners after exiting split tab mode
IN_PROC_BROWSER_TEST_P(BraveBrowserViewWithRoundedCornersTest,
                       RoundedCornersAfterExitingSplitTabTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->Toggle();
  RunScheduledLayouts();

  // Create split tab then exit by creating new active tab
  NewSplitTab();
  browser_view()->DeprecatedLayoutImmediately();
  RunScheduledLayouts();

  chrome::AddTabAt(browser(), GURL(), -1, true);
  RunScheduledLayouts();

  views::View* contents_container = browser_view()->contents_container();
  views::View* side_panel = browser_view()->contents_height_side_panel();
  const auto rounded_corners_margin = BraveContentsViewUtil::kMarginThickness;
  const auto rounded_corners_border_radius = GetRoundedCornersBorderRadius();
  const auto rounded_corners_border_radius_at_window_corner =
      GetRoundedCornersBorderRadiusAtWindowCorner();

  // Verify behavior returns to normal after exiting split mode
  if (IsRoundedCornersEnabled()) {
    ExpectContentsContainerMargins(contents_container, rounded_corners_margin);
    ExpectSidePanelMargins(side_panel, rounded_corners_margin,
                           rounded_corners_margin);
    EXPECT_EQ(gfx::RoundedCornersF(rounded_corners_border_radius),
              side_panel->layer()->rounded_corner_radii());

    const auto contents_view_radii = browser_view()
                                         ->GetActiveContentsContainerView()
                                         ->contents_view()
                                         ->GetBackgroundRadii();
    EXPECT_EQ(BraveContentsViewUtil::GetRoundedCornersForContentsView(browser(),
                                                                      nullptr),
              contents_view_radii);
    ExpectContentsViewRadii(rounded_corners_border_radius,
                            rounded_corners_border_radius,
                            rounded_corners_border_radius_at_window_corner,
                            rounded_corners_border_radius);
  } else {
    EXPECT_EQ(contents_container->bounds().x(),
              browser_view()->GetLocalBounds().x());
    EXPECT_EQ(contents_container->bounds().bottom(),
              browser_view()->GetLocalBounds().bottom());
    ExpectSidePanelMargins(side_panel, 0, 0);
    EXPECT_FALSE(side_panel->layer());
    EXPECT_EQ(gfx::RoundedCornersF(), browser_view()
                                          ->GetActiveContentsContainerView()
                                          ->contents_view()
                                          ->GetBackgroundRadii());
  }
}

// Test 4: Rounded corners with vertical tabs (expanded/collapsed states)
IN_PROC_BROWSER_TEST_P(BraveBrowserViewWithRoundedCornersTest,
                       RoundedCornersWithVerticalTabTest) {
  // Only test when rounded corners are enabled
  if (!IsRoundedCornersEnabled()) {
    return;
  }

  const auto rounded_corners_border_radius = GetRoundedCornersBorderRadius();
  const auto rounded_corners_border_radius_at_window_corner =
      GetRoundedCornersBorderRadiusAtWindowCorner();

  // Test with vertical tab
  ToggleVerticalTabStrip();
  RunScheduledLayouts();

  auto contents_view_radii = browser_view()
                                 ->GetActiveContentsContainerView()
                                 ->contents_view()
                                 ->GetBackgroundRadii();

  // Use border radius as it has left side ui (vertical tab)
  EXPECT_EQ(rounded_corners_border_radius, contents_view_radii.lower_left());

  // Test with "hide completely when collapsed" option
  SetHideCompletelyWhenCollapsed(true);
  RunScheduledLayouts();

  auto* region_view = vertical_tab_strip_region();
  contents_view_radii = browser_view()
                            ->GetActiveContentsContainerView()
                            ->contents_view()
                            ->GetBackgroundRadii();

  // Still use border radius as it's in expanded state
  EXPECT_EQ(BraveVerticalTabStripRegionView::State::kExpanded,
            region_view->state());
  EXPECT_EQ(rounded_corners_border_radius, contents_view_radii.lower_left());

  // Collapse vertical tab
  region_view->ToggleState();
  RunScheduledLayouts();
  contents_view_radii = browser_view()
                            ->GetActiveContentsContainerView()
                            ->contents_view()
                            ->GetBackgroundRadii();

  // Use border radius at window as vertical tab is hidden in collapsed state
  EXPECT_EQ(BraveVerticalTabStripRegionView::State::kCollapsed,
            region_view->state());
  EXPECT_EQ(rounded_corners_border_radius_at_window_corner,
            contents_view_radii.lower_left());
}

IN_PROC_BROWSER_TEST_P(BraveBrowserViewWithRoundedCornersTest,
                       ContentsBackgroundEventHandleTest) {
  EXPECT_TRUE(brave_browser_view()->contents_background_view_);

  EXPECT_TRUE(
      brave_browser_view()->contents_background_view_->bounds().Contains(
          brave_browser_view()->contents_container()->bounds()))
      << "Expected contents_background_view_ bounds ("
      << brave_browser_view()->contents_background_view_->bounds().ToString()
      << ") to contain contents_container bounds ("
      << brave_browser_view()->contents_container()->bounds().ToString() << ")";

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
    testing::Bool());

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

// Tests Brave's UpdateExclusiveAccessBubble override: fullscreen exit
// instruction bubble is shown or hidden based on kShowFullscreenReminder pref,
// for both tab-initiated and browser-initiated fullscreen.
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       FullscreenBubbleHiddenWhenPrefDisabled_TabFullscreen) {
  browser()->profile()->GetPrefs()->SetBoolean(kShowFullscreenReminder, false);

  browser()
      ->GetFeatures()
      .exclusive_access_manager()
      ->context()
      ->UpdateExclusiveAccessBubble(
          {
              .origin = url::Origin::Create(GURL("http://www.example.com")),
              .type = ExclusiveAccessBubbleType::
                  EXCLUSIVE_ACCESS_BUBBLE_TYPE_FULLSCREEN_EXIT_INSTRUCTION,
              .force_update = true,
          },
          base::NullCallback());

  EXPECT_FALSE(browser_view()
                   ->GetExclusiveAccessContext()
                   ->IsExclusiveAccessBubbleDisplayed());
}

IN_PROC_BROWSER_TEST_F(
    BraveBrowserViewTest,
    FullscreenBubbleHiddenWhenPrefDisabled_BrowserFullscreen) {
  browser()->profile()->GetPrefs()->SetBoolean(kShowFullscreenReminder, false);

  browser()
      ->GetFeatures()
      .exclusive_access_manager()
      ->context()
      ->UpdateExclusiveAccessBubble(
          {
              .origin = url::Origin::Create(GURL("http://www.example.com")),
              .type = ExclusiveAccessBubbleType::
                  EXCLUSIVE_ACCESS_BUBBLE_TYPE_BROWSER_FULLSCREEN_EXIT_INSTRUCTION,
              .force_update = true,
          },
          base::NullCallback());

  EXPECT_FALSE(browser_view()
                   ->GetExclusiveAccessContext()
                   ->IsExclusiveAccessBubbleDisplayed());
}

IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       FullscreenBubbleShownWhenPrefEnabled_TabFullscreen) {
  browser()->profile()->GetPrefs()->SetBoolean(kShowFullscreenReminder, true);

  browser()
      ->GetFeatures()
      .exclusive_access_manager()
      ->context()
      ->UpdateExclusiveAccessBubble(
          {
              .origin = url::Origin::Create(GURL("http://www.example.com")),
              .type = ExclusiveAccessBubbleType::
                  EXCLUSIVE_ACCESS_BUBBLE_TYPE_FULLSCREEN_EXIT_INSTRUCTION,
              .force_update = true,
          },
          base::NullCallback());

  EXPECT_TRUE(browser_view()
                  ->GetExclusiveAccessContext()
                  ->IsExclusiveAccessBubbleDisplayed());
}

IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       FullscreenBubbleShownWhenPrefEnabled_BrowserFullscreen) {
  browser()->profile()->GetPrefs()->SetBoolean(kShowFullscreenReminder, true);

  browser()
      ->GetFeatures()
      .exclusive_access_manager()
      ->context()
      ->UpdateExclusiveAccessBubble(
          {
              .origin = url::Origin::Create(GURL("http://www.example.com")),
              .type = ExclusiveAccessBubbleType::
                  EXCLUSIVE_ACCESS_BUBBLE_TYPE_BROWSER_FULLSCREEN_EXIT_INSTRUCTION,
              .force_update = true,
          },
          base::NullCallback());

  EXPECT_TRUE(browser_view()
                  ->GetExclusiveAccessContext()
                  ->IsExclusiveAccessBubbleDisplayed());
}

// When there is no visible top UI (toolbar, bookmark bar),
// GetTopSeparatorType() should return kNone so no 1px separator is drawn at the
// top of the contents view (e.g. in browser fullscreen). Test by injecting a
// delegate that reports no visible top UI instead of toggling fullscreen, which
// is flaky.
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       TopSeparatorNoneWhenNoVisibleTopUI) {
  auto* layout = browser_view()->GetBrowserViewLayoutForTesting();
  auto* brave_tabbed_layout =
      static_cast<BraveBrowserViewTabbedLayoutImpl*>(layout);

  // Replace the layout delegate with one that reports no visible toolbar or
  // bookmark bar (simulating the fullscreen case).
  layout->SetDelegateForTesting(
      std::make_unique<NoVisibleTopUIDelegateForTesting>(*browser_view()));

  // TopSeparatorType::kNone (0) - no 1px separator when top UI is hidden.
  EXPECT_EQ(static_cast<int>(brave_tabbed_layout->GetTopSeparatorType()), 0);
}

// Ensure the bubble is shown for extension-initiated fullscreen when the
// preference is enabled.
IN_PROC_BROWSER_TEST_F(
    BraveBrowserViewTest,
    FullscreenBubbleShownWhenPrefEnabled_ExtensionFullscreen) {
  browser()->profile()->GetPrefs()->SetBoolean(kShowFullscreenReminder, true);

  browser()
      ->GetFeatures()
      .exclusive_access_manager()
      ->context()
      ->UpdateExclusiveAccessBubble(
          {
              .origin = url::Origin::Create(GURL("http://www.example.com")),
              .type = ExclusiveAccessBubbleType::
                  EXCLUSIVE_ACCESS_BUBBLE_TYPE_EXTENSION_FULLSCREEN_EXIT_INSTRUCTION,
              .force_update = true,
          },
          base::NullCallback());

  EXPECT_TRUE(browser_view()
                  ->GetExclusiveAccessContext()
                  ->IsExclusiveAccessBubbleDisplayed());
}
