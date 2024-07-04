/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/run_loop.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"

class SplitViewBrowserTest : public InProcessBrowserTest {
 public:
  SplitViewBrowserTest() : scoped_features_(tabs::features::kBraveSplitView) {}
  ~SplitViewBrowserTest() override = default;

  BraveBrowserView& browser_view() {
    return *static_cast<BraveBrowserView*>(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  views::WebView& secondary_contents_view() {
    return *browser_view().secondary_contents_web_view_;
  }

  TabStripModel& tab_strip_model() { return *(browser()->tab_strip_model()); }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       TilingTwoTabsMakesSecondaryWebViewVisible) {
  // Given secondary web view is hidden as there's no tiled tabs
  ASSERT_FALSE(secondary_contents_view().GetVisible());

  // When tiling tabs and one of them is the active tab,
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = SplitViewBrowserData::FromBrowser(browser());
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(0)));
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(1)));

  // Secondary web view should become visible
  EXPECT_TRUE(secondary_contents_view().GetVisible());
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
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(secondary_contents_view().GetVisible());
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
  ASSERT_TRUE(secondary_contents_view().GetVisible());

  // When activating non-tiled tab
  tab_strip_model().ActivateTabAt(2);

  // Then, the secondary web view should become hidden
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(secondary_contents_view().GetVisible());
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
  ASSERT_FALSE(secondary_contents_view().GetVisible());

  // When activating a tiled tab
  tab_strip_model().ActivateTabAt(0);

  // Then, the secondary web view should show up
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(secondary_contents_view().GetVisible());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       SecondaryWebViewShouldHoldNonActiveTiledTab) {
  // Given that two tabs are tiled
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = SplitViewBrowserData::FromBrowser(browser());
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(0)));
  ASSERT_TRUE(split_view_data->IsTabTiled(tab_strip_model().GetTabHandleAt(1)));
  ASSERT_TRUE(secondary_contents_view().GetVisible());
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
  auto* browser_view = static_cast<BrowserView*>(browser()->window());
  auto* contents_layout_manager = static_cast<BraveContentsLayoutManager*>(
      browser_view->contents_container()->GetLayoutManager());
  constexpr int kSizeDelta = 100;
  contents_layout_manager->set_split_view_size_delta(kSizeDelta);

  // Then these should be persisted during tab activation.
  tab_strip_model().ActivateTabAt(0);
  EXPECT_EQ(0, contents_layout_manager->split_view_size_delta());

  tab_strip_model().ActivateTabAt(3);
  EXPECT_EQ(kSizeDelta, contents_layout_manager->split_view_size_delta());
}
