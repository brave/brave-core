/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/infobars/brave_confirm_infobar_creator.h"
#include "brave/browser/ui/bookmark/bookmark_helper.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/views/brave_layout_provider.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_container_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/screenshot_button.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/infobars/confirm_infobar_creation_allowlist.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_bubble_type.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/test/test_browser_ui.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/frame/browser_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "chrome/browser/ui/views/frame/scrim_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/window_feature_controller/window_feature_controller.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/bookmarks/common/bookmark_bar_visibility_state.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

#if BUILDFLAG(IS_MAC)
#include "chrome/browser/ui/fullscreen_util_mac.h"
#endif  // BUILDFLAG(IS_MAC)

namespace {
class TestInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  // Helper to construct and inject the infobar
  static void Create(infobars::ContentInfoBarManager* infobar_manager) {
    infobar_manager->AddInfoBar(CreateConfirmInfoBar(
        ConfirmInfoBarCreationAllowlist::GetPassKeyForTesting(),
        std::make_unique<TestInfoBarDelegate>()));
  }

  TestInfoBarDelegate() = default;
  ~TestInfoBarDelegate() override = default;

  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override {
    return TEST_INFOBAR;
  }

  // The main text displayed on the banner
  std::u16string GetMessageText() const override {
    return u"This is a test InfoBar injected from a browser test.";
  }
};

class StubBraveConfirmInfoBarDelegate : public BraveConfirmInfoBarDelegate {
 public:
  StubBraveConfirmInfoBarDelegate() = default;
  ~StubBraveConfirmInfoBarDelegate() override = default;

  // BraveConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override {
    return TEST_INFOBAR;
  }

  std::u16string GetMessageText() const override { return u"Test message"; }
};

bool HasBraveConfirmInfoBarView(const views::View* infobar_container) {
  return std::ranges::any_of(
      infobar_container->children(), [](const views::View* child) {
        return views::IsViewClass<BraveConfirmInfoBar>(child);
      });
}

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

  ScreenshotButton* screenshot_button() {
    return static_cast<BraveToolbarView*>(browser_view()->toolbar())
        ->screenshot_button();
  }

  int GetRoundedCornersBorderRadius() const {
    return views::LayoutProvider::Get()->GetCornerRadiusMetric(
        kRoundedCornersBorderRadius);
  }

  int GetRoundedCornersBorderRadiusAtWindowCorner() const {
    return views::LayoutProvider::Get()->GetCornerRadiusMetric(
        kRoundedCornersBorderRadiusAtWindowCorner);
  }

};

// Tests bookmark/infobar/contents container layout with vertical tab.
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest, LayoutWithVerticalTabTest) {
  ToggleVerticalTabStrip();

  auto* prefs = browser()->GetProfile()->GetPrefs();

  // Check bookmark only on the NTP is default.
  EXPECT_EQ(
      prefs->GetInteger(bookmarks::prefs::kBookmarkBarVisibilityState),
      static_cast<int>(bookmarks::BookmarkBarVisibilityState::kOnlyShowOnNtp));

  // BookmarkBar not visible as current active tab is not NTP.
  EXPECT_FALSE(bookmark_bar()->GetVisible());

  const bool rounded_corners =
      BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
          browser());

  const int top_contents_separator_height = rounded_corners ? 0 : 1;

  // The vertical tab strip owns the gap between itself and the contents
  // through its own padding, so the contents reserves no margin on that side
  // and starts exactly where the infobar and bookmark bar do.
  auto contents_area_origin = [&]() {
    return browser_view()->contents_container()->bounds().origin();
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
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !BrowserView::GetBrowserViewForBrowser(browser())
                ->IsBookmarkBarAnimating();
  }));

  // Check bookmark bar/contents container position.
  EXPECT_EQ(vertical_tab_strip_host_view()->bounds().top_right(),
            bookmark_bar()->bounds().origin());
  EXPECT_EQ(bookmark_bar()->bounds().bottom_left() +
                gfx::Vector2d(0, top_contents_separator_height),
            contents_area_origin());

  // Hide bookmark bar always.
  // Check contents container is positioned right after the vertical tab.
  prefs->SetInteger(
      bookmarks::prefs::kBookmarkBarVisibilityState,
      static_cast<int>(bookmarks::BookmarkBarVisibilityState::kAlwaysHide));
  EXPECT_EQ(
      prefs->GetInteger(bookmarks::prefs::kBookmarkBarVisibilityState),
      static_cast<int>(bookmarks::BookmarkBarVisibilityState::kAlwaysHide));
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !BrowserView::GetBrowserViewForBrowser(browser())
                ->IsBookmarkBarAnimating();
  }));
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
  prefs->SetInteger(
      bookmarks::prefs::kBookmarkBarVisibilityState,
      static_cast<int>(bookmarks::BookmarkBarVisibilityState::kAlwaysShow));
  EXPECT_EQ(
      prefs->GetInteger(bookmarks::prefs::kBookmarkBarVisibilityState),
      static_cast<int>(bookmarks::BookmarkBarVisibilityState::kAlwaysShow));
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !BrowserView::GetBrowserViewForBrowser(browser())
                ->IsBookmarkBarAnimating();
  }));
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

IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       RemovesBraveConfirmInfoBarViewWhenInfoBarIsRemoved) {
  auto disable_rich_animation =
      gfx::AnimationTestApi::SetRichAnimationRenderMode(
          gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);

  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(
          browser()->tab_strip_model()->GetActiveWebContents());
  infobar_manager->RemoveAllInfoBars(/*animate=*/false);

  infobars::InfoBar* infobar =
      infobar_manager->AddInfoBar(CreateBraveConfirmInfoBar(
          std::make_unique<StubBraveConfirmInfoBarDelegate>()));
  ASSERT_TRUE(infobar);
  ASSERT_TRUE(HasBraveConfirmInfoBarView(infobar_container()));

  infobar_manager->RemoveInfoBar(infobar);

  EXPECT_THAT(infobar_manager->infobars(), testing::IsEmpty());
  EXPECT_FALSE(HasBraveConfirmInfoBarView(infobar_container()));
}

// Cmd/Ctrl+Shift+S (IDC_SHARING_HUB_SCREENSHOT) should open Brave's own
// screenshot bubble instead of upstream's Sharing Hub one, and should
// temporarily reveal the screenshot toolbar button when it's hidden by the
// "show screenshot button" pref, hiding it again once the bubble closes.
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       ScreenshotCommandShowsBubbleAndTogglesButton) {
  ASSERT_FALSE(
      browser()->GetProfile()->GetPrefs()->GetBoolean(kShowScreenshotButton));

  auto* button = screenshot_button();
  ASSERT_TRUE(button);
  EXPECT_FALSE(button->GetVisible());

  const ui::Accelerator screenshot_accelerator(
      ui::VKEY_S, ui::EF_PLATFORM_ACCELERATOR | ui::EF_SHIFT_DOWN);

  // Pressing the accelerator is handled by Brave (not passed through to
  // upstream), reveals the button, and shows the bubble.
  EXPECT_TRUE(browser_view()->AcceleratorPressed(screenshot_accelerator));
  EXPECT_TRUE(button->GetVisible());

  // The pref itself must remain untouched by the temporary reveal.
  EXPECT_FALSE(
      browser()->GetProfile()->GetPrefs()->GetBoolean(kShowScreenshotButton));

  // Pressing it again toggles the bubble closed, and the button should hide
  // again since it was only shown for the shortcut.
  EXPECT_TRUE(browser_view()->AcceleratorPressed(screenshot_accelerator));
  ASSERT_TRUE(base::test::RunUntil([&]() { return !button->GetVisible(); }));

  // Other paths that trigger the same command should also show the bubble and
  // reveal the button temporarily.
  chrome::BrowserCommandController::From(browser())->ExecuteCommand(
      IDC_SHARING_HUB_SCREENSHOT);
  EXPECT_TRUE(button->GetVisible());
}

// If the user has the screenshot button permanently shown via the pref, the
// accelerator must not hide it afterwards.
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       ScreenshotAcceleratorLeavesButtonVisibleWhenPrefOn) {
  browser()->GetProfile()->GetPrefs()->SetBoolean(kShowScreenshotButton, true);

  auto* button = screenshot_button();
  ASSERT_TRUE(button);
  ASSERT_TRUE(button->GetVisible());

  const ui::Accelerator screenshot_accelerator(
      ui::VKEY_S, ui::EF_PLATFORM_ACCELERATOR | ui::EF_SHIFT_DOWN);
  EXPECT_TRUE(browser_view()->AcceleratorPressed(screenshot_accelerator));
  EXPECT_TRUE(button->GetVisible());

  // Close the bubble and confirm the button, still pref-controlled, stays
  // visible.
  EXPECT_TRUE(browser_view()->AcceleratorPressed(screenshot_accelerator));
  EXPECT_TRUE(button->GetVisible());
}

// Regression test: BraveBrowserView's constructor unconditionally hides the
// window icon for normal browser windows (SetShowIcon(false)), because brave
// supports Browser::WindowFeature::kFeatureTitleBar (unlike upstream), which
// would otherwise make ShouldShowWindowIcon() return true whenever vertical
// tabs are enabled. The PRE_ test persists the pref so the following test's
// browser() is actually launched with vertical tabs already on.
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       PRE_ShouldNotShowWindowIconWithVerticalTabTest) {
  ASSERT_FALSE(
      VerticalTabController::From(browser())->ShouldShowBraveVerticalTabs());
  EXPECT_FALSE(browser_view()->ShouldShowWindowIcon());

  browser()->GetProfile()->GetPrefs()->SetBoolean(
      brave_tabs::kVerticalTabsEnabled, true);
}

IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       ShouldNotShowWindowIconWithVerticalTabTest) {
  // Browser is launched with vertical tab mode already on.
  ASSERT_TRUE(
      VerticalTabController::From(browser())->ShouldShowBraveVerticalTabs());
  EXPECT_FALSE(browser_view()->ShouldShowWindowIcon());
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
    browser()->GetProfile()->GetPrefs()->SetBoolean(kWebViewRoundedCorners,
                                                    IsRoundedCornersEnabled());
  }

  BrowserFrameView* browser_non_client_frame_view() {
    return browser_view()->browser_widget()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view()->DeprecatedLayoutImmediately();
  }

  void SetHideCompletelyWhenCollapsed(bool hide) {
    browser()->GetProfile()->GetPrefs()->SetBoolean(
        brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed, hide);
  }

  BraveVerticalTabStripRegionView* vertical_tab_strip_region() {
    auto* container_view = BraveBrowserView::From(browser_view())
                               ->vertical_tab_strip_container_view();

    return container_view->vertical_tab_strip_region_view();
  }

  bool IsRoundedCornersEnabled() const { return GetParam(); }

 private:
  const gfx::AnimationTestApi::RenderModeResetter disable_rich_animations_;
};

// Rounded corners with vertical tabs (expanded/collapsed states)
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
                       MainBackgroundRegionEventHandleTest) {
  RunScheduledLayouts();

  auto* main_background_region =
      brave_browser_view()->main_background_region_for_testing();
  // main_background_region is only forced visible when rounded corners are
  // in effect.
  EXPECT_EQ(IsRoundedCornersEnabled(), main_background_region->GetVisible());

  if (!main_background_region->GetVisible()) {
    GTEST_SKIP();
  }

  EXPECT_TRUE(main_background_region->bounds().Contains(
      brave_browser_view()->contents_container()->bounds()))
      << "Expected main_background_region bounds ("
      << main_background_region->bounds().ToString()
      << ") to contain contents_container bounds ("
      << brave_browser_view()->contents_container()->bounds().ToString() << ")";

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  gfx::Point screen_point = web_contents->GetContainerBounds().CenterPoint();

  // Check main background region is not event handler for web contents region
  // point.
  views::View::ConvertPointFromScreen(browser_view(), &screen_point);
  EXPECT_NE(main_background_region,
            browser_view()->GetEventHandlerForPoint(screen_point));
}

IN_PROC_BROWSER_TEST_P(BraveBrowserViewWithRoundedCornersTest,
                       TopSeparatorWithPanelTest) {
  // An infobar is what upstream draws the top container separator for. With
  // rounded corners the infobar is a card with its own outline, so the
  // separator would double up with it and is suppressed instead.
  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(
          browser()->tab_strip_model()->GetActiveWebContents());
  TestInfoBarDelegate::Create(infobar_manager);
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return infobar_container()->GetVisible(); }));
  EXPECT_GE(infobar_manager->infobars().size(), 1u);

  const bool separator_expected = !IsRoundedCornersEnabled();
  auto* separator = brave_browser_view()->top_container_separator_for_testing();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return separator_expected == separator->GetVisible(); }));

  // Check separator is hidden after panel opens.
  auto* panel_ui = SidePanelUI::From(browser());
  panel_ui->Toggle();
  auto* side_panel = browser_view()->side_panel();
  ASSERT_TRUE(base::test::RunUntil([&]() { return side_panel->GetVisible(); }));
  RunScheduledLayouts();
  EXPECT_FALSE(brave_browser_view()
                   ->top_container_separator_for_testing()
                   ->GetVisible());
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    BraveBrowserViewWithRoundedCornersTest,
    testing::Bool());

IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       BoundingBoxStableWithSidePanelTest) {
  auto* panel_ui = SidePanelUI::From(browser());
  auto* brave_view = brave_browser_view();

  // The box must span the full browser width regardless of panel state -- that
  // is the regression this guards -- and run from the bottom of the top
  // container to the bottom of the window. Its top edge does follow the top
  // container, which an open panel shortens by hiding the 1px top separator.
  auto verify_bounds = [&](const base::Location& loc = FROM_HERE) {
    SCOPED_TRACE(loc.ToString());
    const gfx::Rect bounds =
        brave_view->GetBoundingBoxInScreenForMouseOverHandling();
    EXPECT_EQ(brave_view->GetBoundsInScreen().x(), bounds.x());
    EXPECT_EQ(browser_view()->width(), bounds.width());
    EXPECT_EQ(browser_view()->top_container()->GetBoundsInScreen().bottom(),
              bounds.y());
    EXPECT_EQ(brave_view->GetBoundsInScreen().bottom(), bounds.bottom());
  };

  verify_bounds();

  panel_ui->Toggle();
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return panel_ui->IsSidePanelShowing(); }));
  RunScheduledLayouts();
  verify_bounds();

  panel_ui->Toggle();
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !panel_ui->IsSidePanelShowing(); }));
  RunScheduledLayouts();
  verify_bounds();
}

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
  browser()->GetProfile()->GetPrefs()->SetBoolean(kShowFullscreenReminder,
                                                  false);

  ExclusiveAccessManager::From(browser())
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
  browser()->GetProfile()->GetPrefs()->SetBoolean(kShowFullscreenReminder,
                                                  false);

  ExclusiveAccessManager::From(browser())->context()->UpdateExclusiveAccessBubble(
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
  browser()->GetProfile()->GetPrefs()->SetBoolean(kShowFullscreenReminder,
                                                  true);

  ExclusiveAccessManager::From(browser())
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
  browser()->GetProfile()->GetPrefs()->SetBoolean(kShowFullscreenReminder,
                                                  true);

  ExclusiveAccessManager::From(browser())->context()->UpdateExclusiveAccessBubble(
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

// Ensure the bubble is shown for extension-initiated fullscreen when the
// preference is enabled.
IN_PROC_BROWSER_TEST_F(
    BraveBrowserViewTest,
    FullscreenBubbleShownWhenPrefEnabled_ExtensionFullscreen) {
  browser()->GetProfile()->GetPrefs()->SetBoolean(kShowFullscreenReminder,
                                                  true);

  ExclusiveAccessManager::From(browser())->context()->UpdateExclusiveAccessBubble(
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

#if BUILDFLAG(IS_MAC)
// Immersive fullscreen: (1) when vertical tabs are off at startup, immersive
// is available and is disabled at runtime when vertical tabs are turned on;
// (2) when vertical tabs were on at startup, immersive stays disabled for the
// window's lifetime (essential immersive objects are not created then).
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       ImmersiveModeAndVerticalTabsAtStartup) {
  // Default browser: vertical tabs off at startup.
  ASSERT_FALSE(
      VerticalTabController::From(browser())->ShouldShowBraveVerticalTabs());
  EXPECT_TRUE(
      WindowFeatureController::From(browser())->UsesImmersiveFullscreenMode());
  ToggleVerticalTabStrip();
  EXPECT_FALSE(
      WindowFeatureController::From(browser())->UsesImmersiveFullscreenMode());

  // Second window: vertical tabs on at startup.
  browser()->GetProfile()->GetPrefs()->SetBoolean(
      brave_tabs::kVerticalTabsEnabled, true);
  BrowserWindowInterface* browser_with_vertical_at_startup =
      CreateBrowser(browser()->GetProfile());
  BraveBrowserView* view_with_vertical_at_startup = BraveBrowserView::From(
      BrowserView::GetBrowserViewForBrowser(browser_with_vertical_at_startup));
  EXPECT_FALSE(WindowFeatureController::From(browser_with_vertical_at_startup)
                   ->UsesImmersiveFullscreenMode());
  brave::ToggleVerticalTabStrip(browser_with_vertical_at_startup);
  view_with_vertical_at_startup->DeprecatedLayoutImmediately();
  EXPECT_FALSE(WindowFeatureController::From(browser_with_vertical_at_startup)
                   ->UsesImmersiveFullscreenMode());
}

// Immersive fullscreen: same as ImmersiveModeAndVerticalTabsAtStartup above,
// but for compact mode. Compact mode is a local-state pref, so toggling it
// affects every window, but each window's startup snapshot
// (disabled_at_startup_) is only captured on that window's first
// UsesImmersiveFullscreenMode() call.
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       ImmersiveModeAndCompactModeAtStartup) {
  auto* local_state = g_browser_process->local_state();

  // Default browser: compact mode off at startup.
  ASSERT_FALSE(local_state->GetBoolean(brave_tabs::kCompactHorizontalTabs));
  EXPECT_TRUE(
      WindowFeatureController::From(browser())->UsesImmersiveFullscreenMode());
  local_state->SetBoolean(brave_tabs::kCompactHorizontalTabs, true);
  EXPECT_FALSE(
      WindowFeatureController::From(browser())->UsesImmersiveFullscreenMode());
  local_state->SetBoolean(brave_tabs::kCompactHorizontalTabs, false);
  EXPECT_TRUE(
      WindowFeatureController::From(browser())->UsesImmersiveFullscreenMode());

  // Second window: compact mode on at startup.
  local_state->SetBoolean(brave_tabs::kCompactHorizontalTabs, true);
  BrowserWindowInterface* browser_with_compact_at_startup =
      CreateBrowser(browser()->GetProfile());
  EXPECT_FALSE(WindowFeatureController::From(browser_with_compact_at_startup)
                   ->UsesImmersiveFullscreenMode());
  local_state->SetBoolean(brave_tabs::kCompactHorizontalTabs, false);
  EXPECT_FALSE(WindowFeatureController::From(browser_with_compact_at_startup)
                   ->UsesImmersiveFullscreenMode());
}

// Regression test: when a browser starts with horizontal tabs and the user
// switches to vertical tabs at runtime, fullscreen_toolbar_controller_ in the
// base BrowserFrameViewMac is nil (it was skipped at construction because
// UsesImmersiveFullscreenMode() returned true then). Messaging nil returns 0
// which equals TOOLBAR_PRESENT, so without the fix ShouldHideTopUIInFullscreen
// would return false during tab-fullscreen, leaving the toolbar visible.
IN_PROC_BROWSER_TEST_F(
    BraveBrowserViewTest,
    ShouldHideTopUIInTabFullscreenAfterVerticalTabsEnabledAtRuntime) {
  // Verify the precondition that triggers the bug: horizontal tabs at startup
  // means immersive mode is on (and fullscreen_toolbar_controller_ is nil).
  ASSERT_FALSE(
      VerticalTabController::From(browser())->ShouldShowBraveVerticalTabs());
  ASSERT_TRUE(
      WindowFeatureController::From(browser())->UsesImmersiveFullscreenMode());

  // Switch to vertical tabs at runtime.
  ToggleVerticalTabStrip();
  ASSERT_TRUE(
      VerticalTabController::From(browser())->ShouldShowBraveVerticalTabs());
  ASSERT_FALSE(
      WindowFeatureController::From(browser())->UsesImmersiveFullscreenMode());

  // Fake tab (content) fullscreen without triggering any OS fullscreen
  // transition — IsWindowFullscreenForTabOrPending() becomes true immediately.
  auto* fullscreen_controller =
      ExclusiveAccessManager::From(browser())->fullscreen_controller();
  fullscreen_controller->set_is_tab_fullscreen_for_testing(true);
  ASSERT_TRUE(fullscreen_utils::IsInContentFullscreen(browser()));

  // The frame view must report that top UI should be hidden during tab
  // fullscreen, even though fullscreen_toolbar_controller_ is nil.
  EXPECT_TRUE(browser_non_client_frame_view()->ShouldHideTopUIInFullscreen());
}
#endif  // BUILDFLAG(IS_MAC)
