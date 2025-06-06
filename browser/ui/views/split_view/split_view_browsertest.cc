/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view.h"

#include <utility>

#include "base/test/run_until.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/brave_javascript_tab_modal_dialog_view_views.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "brave/browser/ui/views/split_view/split_view_layout_manager.h"
#include "brave/browser/ui/views/split_view/split_view_separator.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/tabs/public/split_tab_visual_data.h"
#include "content/public/common/javascript_dialog_type.h"
#include "content/public/test/browser_test.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/compositor/layer.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/view_utils.h"

namespace {
ui::MouseEvent GetDummyEvent() {
  return ui::MouseEvent(ui::EventType::kMousePressed, gfx::PointF(),
                        gfx::PointF(), base::TimeTicks::Now(), 0, 0);
}
}  // namespace

class SplitViewDisabledBrowserTest : public InProcessBrowserTest {
 public:
  SplitViewDisabledBrowserTest() {
    scoped_features_.InitAndDisableFeature(tabs::features::kBraveSplitView);
  }
  ~SplitViewDisabledBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(SplitViewDisabledBrowserTest,
                       SplitViewDisabledStateTest) {
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  EXPECT_FALSE(!!split_view_data);
}

class SideBySideEnabledBrowserTest : public InProcessBrowserTest {
 public:
  SideBySideEnabledBrowserTest() {
    scoped_features_.InitWithFeatures(
        /*enabled_features*/ {features::kSideBySide},
        /*disabled_features*/ {features::kBraveWebViewRoundedCorners});
  }
  ~SideBySideEnabledBrowserTest() override = default;

  TabStrip* tab_strip() {
    return BrowserView::GetBrowserViewForBrowser(browser())->tabstrip();
  }

  BraveBrowserView* brave_browser_view() const {
    return BraveBrowserView::From(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  SplitViewSeparator* split_view_separator() const {
    return views::AsViewClass<SplitViewSeparator>(
        brave_multi_contents_view()->resize_area_for_testing());
  }

  BraveMultiContentsView* brave_multi_contents_view() const {
    return static_cast<BraveMultiContentsView*>(
        brave_browser_view()->multi_contents_view_for_testing());
  }

  BrowserNonClientFrameView* browser_non_client_frame_view() {
    return brave_browser_view()->frame()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view()->DeprecatedLayoutImmediately();
  }

  views::Widget* secondary_location_bar_widget() const {
    return brave_multi_contents_view()->secondary_location_bar_widget_.get();
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(SideBySideEnabledBrowserTest,
                       BraveMultiContentsViewTest) {
  // Check SplitView feature is not enabled.
  EXPECT_FALSE(tabs::features::IsBraveSplitViewEnabled());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  EXPECT_FALSE(!!split_view_data);

  // Check MultiContentsView uses our separator and initially hidden.
  EXPECT_FALSE(split_view_separator()->GetVisible());
  EXPECT_FALSE(split_view_separator()->menu_button_widget_->IsVisible());

  // separator should not be empty when split view is closed.
  auto* browser_view = brave_browser_view();
  EXPECT_NE(gfx::Size(),
            browser_view->contents_separator_for_testing()->GetPreferredSize());

  chrome::NewSplitTab(browser());

  // separator should be empty when split view is opened.
  EXPECT_EQ(gfx::Size(),
            browser_view->contents_separator_for_testing()->GetPreferredSize());
  EXPECT_TRUE(split_view_separator()->GetVisible());
  EXPECT_TRUE(split_view_separator()->menu_button_widget_->IsVisible());

  // Check corner radius.
  auto* multi_contents_view = brave_multi_contents_view();
  ASSERT_TRUE(multi_contents_view);

  auto* start_contents_web_view =
      multi_contents_view->start_contents_view_for_testing();
  auto* end_contents_web_view =
      multi_contents_view->end_contents_view_for_testing();
  ASSERT_TRUE(start_contents_web_view);
  ASSERT_TRUE(end_contents_web_view);
  EXPECT_EQ(start_contents_web_view->layer()->rounded_corner_radii(),
            gfx::RoundedCornersF(multi_contents_view->GetCornerRadius()));
  EXPECT_EQ(end_contents_web_view->layer()->rounded_corner_radii(),
            gfx::RoundedCornersF(multi_contents_view->GetCornerRadius()));

  // Check borders.
  auto start_contents_container_view =
      multi_contents_view->contents_container_views_for_testing()[0];
  auto end_contents_container_view =
      multi_contents_view->contents_container_views_for_testing()[1];
  EXPECT_EQ(gfx::Insets(BraveMultiContentsView::kBorderThickness),
            start_contents_container_view->GetBorder()->GetInsets());
  EXPECT_EQ(gfx::Insets(BraveMultiContentsView::kBorderThickness),
            end_contents_container_view->GetBorder()->GetInsets());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return start_contents_web_view->width() == end_contents_web_view->width();
  }));

  split_view_separator()->OnResize(30, false);
  split_view_separator()->OnResize(30, true);

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return start_contents_web_view->width() != end_contents_web_view->width();
  }));

  // Check double click makes both contents view have same width.
  const gfx::Point point(0, 0);
  ui::MouseEvent event(ui::EventType::kMousePressed, point, point,
                       ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                       ui::EF_LEFT_MOUSE_BUTTON);
  event.SetClickCount(2);
  split_view_separator()->OnMousePressed(event);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return start_contents_web_view->width() == end_contents_web_view->width();
  }));
}

IN_PROC_BROWSER_TEST_F(SideBySideEnabledBrowserTest, SelectTabTest) {
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  EXPECT_EQ(2, tab_strip()->GetActiveIndex());

  EXPECT_FALSE(split_view_separator()->GetVisible());
  EXPECT_FALSE(split_view_separator()->menu_button_widget_->IsVisible());

  // Created new tab(at 3) for new split view with existing tab(at 2).
  chrome::NewSplitTab(browser());
  EXPECT_TRUE(tab_strip()->tab_at(2)->split().has_value());
  EXPECT_FALSE(tab_strip()->tab_at(2)->IsActive());
  EXPECT_TRUE(tab_strip()->tab_at(3)->split().has_value());
  EXPECT_TRUE(tab_strip()->tab_at(3)->IsActive());
  EXPECT_TRUE(split_view_separator()->GetVisible());
  EXPECT_TRUE(split_view_separator()->menu_button_widget_->IsVisible());
  EXPECT_TRUE(split_view_separator()->menu_button_widget_->IsStackedAbove(
      secondary_location_bar_widget()->GetNativeView()));

  // Chromium's mini toolbar should be hidden always as we're using own own mini
  // urlbar.
  EXPECT_FALSE(
      brave_multi_contents_view()->mini_toolbar_for_testing(0)->GetVisible());
  EXPECT_FALSE(
      brave_multi_contents_view()->mini_toolbar_for_testing(1)->GetVisible());

  // Activate non split view tab.
  tab_strip()->SelectTab(tab_strip()->tab_at(0), GetDummyEvent());
  EXPECT_EQ(0, tab_strip()->GetActiveIndex());
  EXPECT_FALSE(split_view_separator()->GetVisible());
  EXPECT_FALSE(split_view_separator()->menu_button_widget_->IsVisible());

  // Check selected split tab becomes active tab.
  tab_strip()->SelectTab(tab_strip()->tab_at(2), GetDummyEvent());
  EXPECT_EQ(2, tab_strip()->GetActiveIndex());
  EXPECT_TRUE(tab_strip()->tab_at(2)->IsActive());
  EXPECT_FALSE(tab_strip()->tab_at(3)->IsActive());
  EXPECT_TRUE(split_view_separator()->GetVisible());
  EXPECT_TRUE(split_view_separator()->menu_button_widget_->IsVisible());

  tab_strip()->SelectTab(tab_strip()->tab_at(0), GetDummyEvent());
  EXPECT_EQ(0, tab_strip()->GetActiveIndex());

  tab_strip()->SelectTab(tab_strip()->tab_at(3), GetDummyEvent());
  EXPECT_EQ(3, tab_strip()->GetActiveIndex());
  EXPECT_FALSE(tab_strip()->tab_at(2)->IsActive());
  EXPECT_TRUE(tab_strip()->tab_at(3)->IsActive());

  // Check split tab's border insets to test split tabs related apis in
  // BraveVerticalTabStyle. Its insets is different with normal tabs. Also
  // different between first and second split tab in vertical tab mode.
  ToggleVerticalTabStrip();

  // Create new tab at 4.
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  EXPECT_EQ(4, tab_strip()->GetActiveIndex());

  // Get base insets.
  const auto insets = tab_strip()->tab_at(4)->tab_style()->GetContentsInsets();

  // Check normal tab's border insets.
  EXPECT_EQ(tab_strip()->tab_at(4)->GetBorder()->GetInsets(), insets);

  // Create split tabs with tab at 4 and new tab at 5.
  chrome::NewSplitTab(browser());

  // Check split tab's first & second tabs' insets are different.
  // value 4 here is copied from |kPaddingForVerticalTabInTile| in
  // brave_tab_style_views.inc.cc.
  EXPECT_EQ(tab_strip()->tab_at(4)->GetBorder()->GetInsets(),
            insets + gfx::Insets::TLBR(4, 0, 0, 0));
  EXPECT_EQ(tab_strip()->tab_at(5)->GetBorder()->GetInsets(),
            insets + gfx::Insets::TLBR(0, 0, 4, 0));
}

class SplitViewBrowserTest : public InProcessBrowserTest {
 public:
  SplitViewBrowserTest() = default;
  ~SplitViewBrowserTest() override = default;

  BraveBrowserView& browser_view() {
    return *static_cast<BraveBrowserView*>(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  views::WebView& contents_web_view() {
    return *browser_view().contents_web_view();
  }

  views::View& secondary_contents_container() {
    return *browser_view().split_view_->secondary_contents_container_;
  }

  ScrimView& secondary_contents_scrim_view() {
    return *browser_view().split_view_->secondary_contents_scrim_view_;
  }

  views::WebView& secondary_contents_view() {
    return *browser_view().split_view_->secondary_contents_web_view_;
  }
  views::WebView& secondary_dev_tools() {
    return *browser_view().split_view_->secondary_devtools_web_view_;
  }

  SplitView& split_view() { return *browser_view().split_view_; }

  SplitViewSeparator& split_view_separator() {
    return *browser_view().split_view_->split_view_separator_;
  }

  BrowserNonClientFrameView& browser_non_client_frame_view() {
    return *browser_view().frame()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view().DeprecatedLayoutImmediately();
  }

  TabStrip& tab_strip() {
    return *BrowserView::GetBrowserViewForBrowser(browser())->tabstrip();
  }

  TabStripModel& tab_strip_model() { return *(browser()->tab_strip_model()); }
};

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       SplitViewContainsContentsContainer) {
  EXPECT_EQ(browser_view().contents_container()->parent(), &split_view());
  EXPECT_EQ(
      static_cast<BraveBrowserViewLayout*>(browser_view().GetLayoutManager())
          ->contents_container(),
      &split_view());

  // MultiContentsView is not initialized if we don't enable
  // features::kSideBySide.
  EXPECT_FALSE(browser_view().multi_contents_view_for_testing());
}

// MacOS does not need views window scrim. We use sheet to show window modals
// (-[NSWindow beginSheet:]), which natively draws a scrim since macOS 11.
#if !BUILDFLAG(IS_MAC)
IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest, ScrimForSecondaryContents) {
  if (!base::FeatureList::IsEnabled(features::kScrimForBrowserWindowModal)) {
    GTEST_SKIP();
  }

  brave::NewSplitViewForTab(browser());

  auto child_widget_delegate = std::make_unique<views::WidgetDelegate>();
  auto child_widget = std::make_unique<views::Widget>();
  child_widget_delegate->SetModalType(ui::mojom::ModalType::kWindow);
  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_WINDOW);
  params.delegate = child_widget_delegate.get();
  params.parent = secondary_contents_container().GetWidget()->GetNativeView();
  child_widget->Init(std::move(params));

  child_widget->Show();
  EXPECT_TRUE(secondary_contents_scrim_view().GetVisible());
  child_widget->Hide();
  EXPECT_FALSE(secondary_contents_scrim_view().GetVisible());
  child_widget->Show();
  EXPECT_TRUE(secondary_contents_scrim_view().GetVisible());
  // Destroy the child widget, the parent should be notified about child modal
  // visibility change.
  child_widget.reset();
  EXPECT_FALSE(secondary_contents_scrim_view().GetVisible());
}
#endif  // !BUILDFLAG(IS_MAC)

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       GetAccessiblePaneContainsSecondaryViews) {
  secondary_contents_container().SetVisible(true);
  secondary_contents_view().SetVisible(true);
  secondary_dev_tools().SetVisible(true);
  std::vector<views::View*> panes;
  static_cast<views::WidgetDelegate*>(&browser_view())
      ->GetAccessiblePanes(&panes);
  EXPECT_TRUE(base::Contains(panes, &secondary_contents_view()));
  EXPECT_TRUE(base::Contains(panes, &secondary_dev_tools()));

  secondary_contents_view().SetVisible(false);
  secondary_dev_tools().SetVisible(false);
  panes.clear();
  static_cast<views::WidgetDelegate*>(&browser_view())
      ->GetAccessiblePanes(&panes);
  EXPECT_FALSE(base::Contains(panes, &secondary_contents_view()));
  EXPECT_FALSE(base::Contains(panes, &secondary_dev_tools()));
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       TilingTwoTabsMakesSecondaryWebViewVisible) {
  // Given secondary web view is hidden as there's no tiled tabs
  ASSERT_FALSE(secondary_contents_container().GetVisible());

  EXPECT_FALSE(split_view_separator().GetVisible());
  EXPECT_FALSE(split_view_separator().menu_button_widget_->IsVisible());

  // When tiling tabs and one of them is the active tab,
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(1)->GetHandle()));

  // Secondary web view should become visible
  EXPECT_TRUE(secondary_contents_container().GetVisible());
  EXPECT_TRUE(split_view_separator().GetVisible());
  EXPECT_TRUE(split_view_separator().menu_button_widget_->IsVisible());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       BreakingTileMakesSecondaryWebViewHidden) {
  // Given there were tiled tabs
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(1)->GetHandle()));
  EXPECT_TRUE(split_view_separator().GetVisible());
  EXPECT_TRUE(split_view_separator().menu_button_widget_->IsVisible());

  // When breaking the tile
  split_view_data->BreakTile(tab_strip_model().GetTabAtIndex(0)->GetHandle());
  ASSERT_FALSE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));

  // Then, the secondary web view should become hidden
  EXPECT_FALSE(secondary_contents_container().GetVisible());
  EXPECT_FALSE(split_view_separator().GetVisible());
  EXPECT_FALSE(split_view_separator().menu_button_widget_->IsVisible());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       ActivateNonTiledTabShouldHideSecondaryWebView) {
  // Given there were tiled tabs and non tiled tab, and split view is visible
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(1)->GetHandle()));
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ false);
  ASSERT_TRUE(secondary_contents_container().GetVisible());
  EXPECT_TRUE(split_view_separator().GetVisible());
  EXPECT_TRUE(split_view_separator().menu_button_widget_->IsVisible());

  // When activating non-tiled tab
  tab_strip_model().ActivateTabAt(2);

  // Then, the secondary web view should become hidden
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !secondary_contents_container().GetVisible(); }));
  EXPECT_FALSE(split_view_separator().GetVisible());
  EXPECT_FALSE(split_view_separator().menu_button_widget_->IsVisible());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       ActivateTiledTabsShouldShowWebView) {
  // Given there were tiled tabs and non tiled tab, and the non tiled tab is the
  // active tab
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(1)->GetHandle()));
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  ASSERT_FALSE(secondary_contents_container().GetVisible());

  // When activating a tiled tab
  tab_strip_model().ActivateTabAt(0);

  // Then, the secondary web view should show up
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return secondary_contents_container().GetVisible(); }));

  // Check split tab's border insets to test split tabs related apis in
  // BraveVerticalTabStyle. Its insets is different with normal tabs. Also
  // different between first and second split tab in vertical tab mode.
  ToggleVerticalTabStrip();

  // Create new tab at 3.
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  EXPECT_EQ(3, tab_strip().GetActiveIndex());

  // Get base insets.
  const auto insets = tab_strip().tab_at(3)->tab_style()->GetContentsInsets();

  // Check normal tab's border insets.
  EXPECT_EQ(tab_strip().tab_at(3)->GetBorder()->GetInsets(), insets);

  // Create split tabs with tab at 3 and new tab at 4.
  brave::NewSplitViewForTab(browser());

  // Check split tab's first & second tabs' insets are different.
  // value 4 here is copied from |kPaddingForVerticalTabInTile| in
  // brave_tab_style_views.inc.cc.
  EXPECT_EQ(tab_strip().tab_at(3)->GetBorder()->GetInsets(),
            insets + gfx::Insets::TLBR(4, 0, 0, 0));
  EXPECT_EQ(tab_strip().tab_at(4)->GetBorder()->GetInsets(),
            insets + gfx::Insets::TLBR(0, 0, 4, 0));
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       SecondaryWebViewShouldHoldNonActiveTiledTab) {
  // Given that two tabs are tiled
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(1)->GetHandle()));
  ASSERT_TRUE(secondary_contents_container().GetVisible());
  auto tile = *split_view_data->GetTile(
      tab_strip_model().GetTabAtIndex(0)->GetHandle());

  // When the tile.first is active contents,
  tab_strip_model().ActivateTabAt(
      tab_strip_model().GetIndexOfTab(tile.first.Get()));
  auto active_tab_handle =
      tab_strip_model()
          .GetTabAtIndex(tab_strip_model().GetIndexOfWebContents(
              tab_strip_model().GetActiveWebContents()))
          ->GetHandle();
  ASSERT_EQ(active_tab_handle, tile.first);

  // Then the secondary web view should hold the tile.second
  EXPECT_EQ(tab_strip_model().GetWebContentsAt(
                tab_strip_model().GetIndexOfTab(tile.second.Get())),
            secondary_contents_view().web_contents());

  // On the other hand, when tile.second is active contents,
  tab_strip_model().ActivateTabAt(
      tab_strip_model().GetIndexOfTab(tile.second.Get()));
  active_tab_handle =
      tab_strip_model()
          .GetTabAtIndex(tab_strip_model().GetIndexOfWebContents(
              tab_strip_model().GetActiveWebContents()))
          ->GetHandle();
  ASSERT_EQ(active_tab_handle, tile.second);

  // Then the secondary web view should hold the tile.first
  EXPECT_EQ(tab_strip_model().GetWebContentsAt(
                tab_strip_model().GetIndexOfTab(tile.first.Get())),
            secondary_contents_view().web_contents());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest, SplitViewSizeDelta) {
  // Given there are two tiles
  brave::NewSplitViewForTab(browser());
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  brave::NewSplitViewForTab(browser());

  // When size delta is set
  auto* split_view_layout_manager =
      static_cast<SplitViewLayoutManager*>(split_view().GetLayoutManager());
  constexpr int kSizeDelta = 100;
  split_view_layout_manager->set_split_view_size_delta(kSizeDelta);

  // Then these should be persisted during tab activation.
  tab_strip_model().ActivateTabAt(0);
  EXPECT_EQ(0, split_view_layout_manager->split_view_size_delta());

  tab_strip_model().ActivateTabAt(3);
  EXPECT_EQ(kSizeDelta, split_view_layout_manager->split_view_size_delta());
}

IN_PROC_BROWSER_TEST_F(
    SplitViewBrowserTest,
    JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView) {
  brave::NewSplitViewForTab(browser());
  auto* active_contents = chrome_test_utils::GetActiveWebContents(this);

  auto* dialog = new BraveJavaScriptTabModalDialogViewViews(
      active_contents, active_contents, u"title",
      content::JAVASCRIPT_DIALOG_TYPE_ALERT, u"message", u"default prompt",
      base::DoNothing(), base::DoNothing());
  ASSERT_TRUE(dialog);
  auto* widget = dialog->GetWidget();
  ASSERT_TRUE(widget);

  const auto dialog_bounds = widget->GetWindowBoundsInScreen();

  auto web_view_bounds = contents_web_view().GetLocalBounds();
  views::View::ConvertRectToScreen(&contents_web_view(), &web_view_bounds);

  EXPECT_EQ(web_view_bounds.CenterPoint().x(), dialog_bounds.CenterPoint().x());
}

// This test can be flaky depending on the screen size. Our macOS CI doesn't
// seem to have a large enough screen to run this test.
#if BUILDFLAG(IS_MAC)
#define MAYBE_JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView_InVerticalTab \
  DISABLED_JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView_InVerticalTab
#else
#define MAYBE_JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView_InVerticalTab \
  JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView_InVerticalTab
#endif

IN_PROC_BROWSER_TEST_F(
    SplitViewBrowserTest,
    MAYBE_JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView_InVerticalTab) {
  brave::ToggleVerticalTabStrip(browser());
  brave::NewSplitViewForTab(browser());
  auto* active_contents = chrome_test_utils::GetActiveWebContents(this);

  auto* dialog = new BraveJavaScriptTabModalDialogViewViews(
      active_contents, active_contents, u"title",
      content::JAVASCRIPT_DIALOG_TYPE_ALERT, u"message", u"default prompt",
      base::DoNothing(), base::DoNothing());
  ASSERT_TRUE(dialog);
  auto* widget = dialog->GetWidget();
  ASSERT_TRUE(widget);

  const auto dialog_bounds = widget->GetWindowBoundsInScreen();

  auto web_view_bounds = contents_web_view().GetLocalBounds();
  views::View::ConvertRectToScreen(&contents_web_view(), &web_view_bounds);

  EXPECT_EQ(web_view_bounds.CenterPoint().x(), dialog_bounds.CenterPoint().x());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest, SplitViewTabPathTest) {
  brave::NewSplitViewForTab(browser());
  int active_index = tab_strip_model().active_index();
  ASSERT_NE(TabStripModel::kNoTab, active_index);

  TabStrip* tab_strip = browser_view().tabstrip();
  Tab* tab = tab_strip->tab_at(active_index);

  SkPath mask = tab->tab_style_views()->GetPath(TabStyle::PathType::kFill,
                                                /* scale */ 1.0,
                                                /* force_active */ false,
                                                TabStyle::RenderUnits::kDips);
  SkRegion clip_region;
  clip_region.setRect({0, 0, 200, 200});
  SkRegion mask_region;
  ASSERT_TRUE(mask_region.setPath(mask, clip_region));

  EXPECT_EQ(brave_tabs::kHorizontalSplitViewTabVerticalSpacing,
            mask_region.getBounds().top());
  EXPECT_EQ(brave_tabs::kHorizontalTabInset, mask_region.getBounds().left());
  EXPECT_EQ(GetLayoutConstant(TAB_STRIP_HEIGHT) -
                GetLayoutConstant(TABSTRIP_TOOLBAR_OVERLAP) -
                (brave_tabs::kHorizontalSplitViewTabVerticalSpacing * 2),
            mask_region.getBounds().height());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest, SplitViewFullscreenTest) {
  brave::NewSplitViewForTab(browser());

  // In split view tile, both contents are visible and have its border.
  EXPECT_TRUE(browser_view().contents_container()->GetVisible());
  EXPECT_TRUE(browser_view().contents_container()->GetBorder());
  EXPECT_TRUE(secondary_contents_container().GetVisible());
  EXPECT_TRUE(secondary_contents_container().GetBorder());

  // Simulate tab-fullscreen state change.
  FullscreenController* fullscreen_controller =
      browser()->exclusive_access_manager()->fullscreen_controller();
  fullscreen_controller->set_is_tab_fullscreen_for_testing(true);
  split_view().OnFullscreenStateChanged();

  // In tab full screen, only primary content is visible w/o border.
  EXPECT_TRUE(browser_view().contents_container()->GetVisible());
  EXPECT_FALSE(browser_view().contents_container()->GetBorder());
  EXPECT_FALSE(secondary_contents_container().GetVisible());
  EXPECT_FALSE(secondary_contents_container().GetBorder());

  fullscreen_controller->set_is_tab_fullscreen_for_testing(false);
  split_view().OnFullscreenStateChanged();

  EXPECT_TRUE(browser_view().contents_container()->GetVisible());
  EXPECT_TRUE(browser_view().contents_container()->GetBorder());
  EXPECT_TRUE(secondary_contents_container().GetVisible());
  EXPECT_TRUE(secondary_contents_container().GetBorder());
}
