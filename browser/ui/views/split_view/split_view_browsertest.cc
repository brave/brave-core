/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view.h"

#include <utility>

#include "base/test/run_until.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/brave_javascript_tab_modal_dialog_view_views.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/split_view/split_view_layout_manager.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/common/javascript_dialog_type.h"
#include "content/public/test/browser_test.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"

class SplitViewBrowserTest : public InProcessBrowserTest {
 public:
  SplitViewBrowserTest() : scoped_features_(tabs::features::kBraveSplitView) {}
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

  views::WebView& secondary_contents_view() {
    return *browser_view().split_view_->secondary_contents_web_view();
  }
  views::WebView& secondary_dev_tools() {
    return *browser_view().split_view_->secondary_devtools_web_view();
  }

  SplitView& split_view() { return *browser_view().split_view_; }

  TabStripModel& tab_strip_model() { return *(browser()->tab_strip_model()); }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       SplitViewContainsContentsContainer) {
  EXPECT_EQ(browser_view().contents_container()->parent(), &split_view());
  EXPECT_EQ(
      static_cast<BraveBrowserViewLayout*>(browser_view().GetLayoutManager())
          ->contents_container(),
      &split_view());
}

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

  // When tiling tabs and one of them is the active tab,
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = SplitViewBrowserData::FromBrowser(browser());
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(0)));
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(1)));

  // Secondary web view should become visible
  EXPECT_TRUE(secondary_contents_container().GetVisible());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       BreakingTileMakesSecondaryWebViewHidden) {
  // Given there were tiled tabs
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = SplitViewBrowserData::FromBrowser(browser());
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(0)));
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(1)));

  // When breaking the tile
  split_view_data->BreakTile(tab_strip_model().GetTabHandleAt(0));
  ASSERT_FALSE(
      split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(0)));

  // Then, the secondary web view should become hidden
  EXPECT_FALSE(secondary_contents_container().GetVisible());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       ActivateNonTiledTabShouldHideSecondaryWebView) {
  // Given there were tiled tabs and non tiled tab, and split view is visible
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = SplitViewBrowserData::FromBrowser(browser());
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(0)));
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(1)));
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ false);
  ASSERT_TRUE(secondary_contents_container().GetVisible());

  // When activating non-tiled tab
  tab_strip_model().ActivateTabAt(2);

  // Then, the secondary web view should become hidden
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !secondary_contents_container().GetVisible(); }));
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       ActivateTiledTabsShouldShowWebView) {
  // Given there were tiled tabs and non tiled tab, and the non tiled tab is the
  // active tab
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = SplitViewBrowserData::FromBrowser(browser());
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(0)));
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(1)));
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  ASSERT_FALSE(secondary_contents_container().GetVisible());

  // When activating a tiled tab
  tab_strip_model().ActivateTabAt(0);

  // Then, the secondary web view should show up
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return secondary_contents_container().GetVisible(); }));
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       SecondaryWebViewShouldHoldNonActiveTiledTab) {
  // Given that two tabs are tiled
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = SplitViewBrowserData::FromBrowser(browser());
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(0)));
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(1)));
  ASSERT_TRUE(secondary_contents_container().GetVisible());
  auto tile = *split_view_data->GetTile(tab_strip_model().GetTabHandleAt(0));

  // When the tile.first is active contents,
  tab_strip_model().ActivateTabAt(tab_strip_model().GetIndexOfTab(tile.first));
  auto active_tab_handle =
      tab_strip_model().GetTabHandleAt(tab_strip_model().GetIndexOfWebContents(
          tab_strip_model().GetActiveWebContents()));
  ASSERT_EQ(active_tab_handle, tile.first);

  // Then the secondary web view should hold the tile.second
  EXPECT_EQ(tab_strip_model().GetWebContentsAt(
                tab_strip_model().GetIndexOfTab(tile.second)),
            secondary_contents_view().web_contents());

  // On the other hand, when tile.second is active contents,
  tab_strip_model().ActivateTabAt(tab_strip_model().GetIndexOfTab(tile.second));
  active_tab_handle =
      tab_strip_model().GetTabHandleAt(tab_strip_model().GetIndexOfWebContents(
          tab_strip_model().GetActiveWebContents()));
  ASSERT_EQ(active_tab_handle, tile.second);

  // Then the secondary web view should hold the tile.first
  EXPECT_EQ(tab_strip_model().GetWebContentsAt(
                tab_strip_model().GetIndexOfTab(tile.first)),
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
