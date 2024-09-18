/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/split_view_tab_strip_model_adapter.h"

#include <utility>

#include "base/run_loop.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/tab_groups/tab_group_id.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"

class SplitViewTabStripModelAdapterBrowserTest : public InProcessBrowserTest {
 public:
  SplitViewTabStripModelAdapterBrowserTest()
      : feature_list_(tabs::features::kBraveSplitView) {}
  ~SplitViewTabStripModelAdapterBrowserTest() override = default;

  TabStripModel* tab_strip_model() const {
    return browser()->tab_strip_model();
  }
  SplitViewBrowserData& data() const { return *split_view_browser_data_; }
  SplitViewTabStripModelAdapter& adapter() const { return *adapter_; }

  std::unique_ptr<content::WebContents> CreateWebContents() {
    content::WebContents::CreateParams params(browser()->profile());
    auto web_contents = content::WebContents::Create(params);
    CHECK(web_contents);
    return web_contents;
  }

  // InProcessBrowserTest:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    split_view_browser_data_.reset(new SplitViewBrowserData(nullptr));
    split_view_browser_data_->is_testing_ = true;
    split_view_browser_data_->tab_strip_model_for_testing_ = tab_strip_model();
    split_view_browser_data_->tab_strip_model_adapter_ =
        std::make_unique<SplitViewTabStripModelAdapter>(
            *split_view_browser_data_, tab_strip_model());

    adapter_ = split_view_browser_data_->tab_strip_model_adapter_.get();
  }

 private:
  base::test::ScopedFeatureList feature_list_;

  std::unique_ptr<SplitViewBrowserData> split_view_browser_data_;
  raw_ptr<SplitViewTabStripModelAdapter> adapter_;
};

IN_PROC_BROWSER_TEST_F(SplitViewTabStripModelAdapterBrowserTest,
                       TilingTabsMakesTabsAdjacent) {
  // Given that there're multiple tabs
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground=*/true);
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground=*/true);
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground=*/true);
  auto secondary_tab = tab_strip_model()->GetTabHandleAt(3);

  // When tiling two tabs that are not adjacent,
  data().TileTabs({tab_strip_model()->GetTabHandleAt(0), secondary_tab});
  ASSERT_TRUE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(0)));
  ASSERT_TRUE(data().IsTabTiled(secondary_tab));

  // Then the tabs should get adjacent.
  EXPECT_EQ(1, tab_strip_model()->GetIndexOfTab(secondary_tab));
}

IN_PROC_BROWSER_TEST_F(SplitViewTabStripModelAdapterBrowserTest,
                       TilingTabsMakesGroupSynchronized_OnlyFirstTabIsGrouped) {
  // Given that a tab is in a group,
  const auto group_id = tab_groups::TabGroupId::GenerateNew();
  tab_strip_model()->group_model()->AddTabGroup(group_id, std::nullopt);
  tab_strip_model()->AddWebContents(CreateWebContents(), -1,
                                    ui::PageTransition::PAGE_TRANSITION_TYPED,
                                    /*add_types=*/0, group_id);

  // When tiling with a non grouped tab
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground*/ true);
  ASSERT_FALSE(tab_strip_model()->GetTabGroupForTab(2));
  data().TileTabs({tab_strip_model()->GetTabHandleAt(1),
                   tab_strip_model()->GetTabHandleAt(2)});
  ASSERT_TRUE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(1)));
  ASSERT_TRUE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(2)));
  base::RunLoop().RunUntilIdle();

  // Then the other tab should be grouped too
  EXPECT_TRUE(tab_strip_model()->GetTabGroupForTab(1));
  EXPECT_EQ(group_id, *tab_strip_model()->GetTabGroupForTab(1));
}

IN_PROC_BROWSER_TEST_F(SplitViewTabStripModelAdapterBrowserTest,
                       TilingTabsMakesGroupSynchronized_InDifferentGroups) {
  // Given that tabs are in different groups
  const auto group_id = tab_groups::TabGroupId::GenerateNew();
  tab_strip_model()->group_model()->AddTabGroup(group_id, std::nullopt);
  tab_strip_model()->AddWebContents(CreateWebContents(), -1,
                                    ui::PageTransition::PAGE_TRANSITION_TYPED,
                                    /*add_types=*/0, group_id);

  const auto second_group_id = tab_groups::TabGroupId::GenerateNew();
  tab_strip_model()->group_model()->AddTabGroup(second_group_id, std::nullopt);
  tab_strip_model()->AddWebContents(CreateWebContents(), -1,
                                    ui::PageTransition::PAGE_TRANSITION_TYPED,
                                    /*add_types=*/0, second_group_id);
  ASSERT_EQ(second_group_id, *tab_strip_model()->GetTabGroupForTab(2));

  // When tiling with a tab in another group,
  data().TileTabs({tab_strip_model()->GetTabHandleAt(1),
                   tab_strip_model()->GetTabHandleAt(2)});
  ASSERT_TRUE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(1)));
  ASSERT_TRUE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(2)));
  base::RunLoop().RunUntilIdle();

  // Then the other tab should be moved to the first tab's group
  EXPECT_EQ(group_id, *tab_strip_model()->GetTabGroupForTab(1));
  EXPECT_EQ(group_id, *tab_strip_model()->GetTabGroupForTab(2));
}

IN_PROC_BROWSER_TEST_F(SplitViewTabStripModelAdapterBrowserTest,
                       TilingTabsSynchronizePinnedState_OnlyOneTabIsPinned) {
  // Given that a tab is pinned
  tab_strip_model()->AddWebContents(CreateWebContents(), -1,
                                    ui::PageTransition::PAGE_TRANSITION_TYPED,
                                    /*add_types=*/0);
  tab_strip_model()->SetTabPinned(0, /*pinned*/ true);

  // When tiling with unpinned tab
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground*/ true);
  ASSERT_FALSE(tab_strip_model()->IsTabPinned(1));
  data().TileTabs({tab_strip_model()->GetTabHandleAt(0),
                   tab_strip_model()->GetTabHandleAt(1)});
  ASSERT_TRUE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(0)));
  ASSERT_TRUE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(1)));
  base::RunLoop().RunUntilIdle();

  // Then the other tab should be pinned too
  EXPECT_TRUE(tab_strip_model()->IsTabPinned(1));
}

IN_PROC_BROWSER_TEST_F(SplitViewTabStripModelAdapterBrowserTest,
                       OnTabInserted_MoveTabWhenInsertedBetweenTile) {
  // Given that two tabs are tiled
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground*/ true);
  data().TileTabs({tab_strip_model()->GetTabHandleAt(0),
                   tab_strip_model()->GetTabHandleAt(1)});
  auto tab1 = tab_strip_model()->GetTabHandleAt(0);
  auto tab2 = tab_strip_model()->GetTabHandleAt(1);
  ASSERT_TRUE(data().IsTabTiled(tab1));
  ASSERT_TRUE(data().IsTabTiled(tab2));

  // When inserting a tab in the middle of the tile,
  auto new_contents = CreateWebContents();
  auto* new_contents_ptr = new_contents.get();
  tab_strip_model()->InsertWebContentsAt(/*index*/ 1, std::move(new_contents),
                                         /*add_type*/ 0);
  base::RunLoop().RunUntilIdle();

  // Then the tile should stay.
  EXPECT_TRUE(data().IsTabTiled(tab1));
  EXPECT_TRUE(data().IsTabTiled(tab2));

  EXPECT_EQ(tab1, tab_strip_model()->GetTabHandleAt(0));
  EXPECT_EQ(tab2, tab_strip_model()->GetTabHandleAt(1));
  EXPECT_EQ(new_contents_ptr, tab_strip_model()->GetWebContentsAt(2));
}

IN_PROC_BROWSER_TEST_F(SplitViewTabStripModelAdapterBrowserTest,
                       OnTabMoved_MovesTiledTab) {
  // Given that two tabs are tiled
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground*/ true);
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground*/ true);
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground*/ true);
  data().TileTabs({tab_strip_model()->GetTabHandleAt(0),
                   tab_strip_model()->GetTabHandleAt(1)});
  auto tab1 = tab_strip_model()->GetTabHandleAt(0);
  auto tab2 = tab_strip_model()->GetTabHandleAt(1);
  ASSERT_TRUE(data().IsTabTiled(tab1));
  ASSERT_TRUE(data().IsTabTiled(tab2));

  // When moving the left tab to the right
  ASSERT_EQ(3,
            tab_strip_model()->MoveWebContentsAt(/*from*/ 0, /*to*/ 3,
                                                 /*select_after_move*/ false));

  // Then the the tab should follow.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, tab_strip_model()->GetIndexOfTab(tab1));
  EXPECT_EQ(3, tab_strip_model()->GetIndexOfTab(tab2));

  // when moving the left tab to the left
  ASSERT_EQ(0,
            tab_strip_model()->MoveWebContentsAt(/*from*/ 2, /*to*/ 0,
                                                 /*select_after_move*/ false));

  // Then the the tab should follow.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, tab_strip_model()->GetIndexOfTab(tab1));
  EXPECT_EQ(1, tab_strip_model()->GetIndexOfTab(tab2));

  // When moving the right tab to the right,
  ASSERT_EQ(2,
            tab_strip_model()->MoveWebContentsAt(/*from*/ 1, /*to*/ 2,
                                                 /*select_after_move*/ false));

  // Then the the tab should follow.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, tab_strip_model()->GetIndexOfTab(tab1));
  EXPECT_EQ(2, tab_strip_model()->GetIndexOfTab(tab2));

  // When moving the right tab to the left,
  ASSERT_EQ(0,
            tab_strip_model()->MoveWebContentsAt(/*from*/ 1, /*to*/ 0,
                                                 /*select_after_move*/ false));

  // Then the the tab should follow.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, tab_strip_model()->GetIndexOfTab(tab1));
  EXPECT_EQ(1, tab_strip_model()->GetIndexOfTab(tab2));
}

IN_PROC_BROWSER_TEST_F(SplitViewTabStripModelAdapterBrowserTest,
                       OnTabRemoved_BreaksTile) {
  // Given that two tabs are tiled
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground*/ true);
  data().TileTabs({tab_strip_model()->GetTabHandleAt(0),
                   tab_strip_model()->GetTabHandleAt(1)});
  auto tab1 = tab_strip_model()->GetTabHandleAt(0);
  auto tab2 = tab_strip_model()->GetTabHandleAt(1);
  ASSERT_TRUE(data().IsTabTiled(tab1));
  ASSERT_TRUE(data().IsTabTiled(tab2));

  // When removing one of the tabs,
  tab_strip_model()->CloseWebContentsAt(/*index*/ 0, /*close_type*/ 0);

  // Then the tile should be broken.
  EXPECT_FALSE(data().IsTabTiled(tab1));
  EXPECT_FALSE(data().IsTabTiled(tab2));
}

IN_PROC_BROWSER_TEST_F(SplitViewTabStripModelAdapterBrowserTest,
                       TabPinnedStateChanged_PinnedStateIsSynced) {
  // Given that two tabs are tiled
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground*/ true);
  data().TileTabs({tab_strip_model()->GetTabHandleAt(0),
                   tab_strip_model()->GetTabHandleAt(1)});
  auto tab1 = tab_strip_model()->GetTabHandleAt(0);
  auto tab2 = tab_strip_model()->GetTabHandleAt(1);
  ASSERT_TRUE(data().IsTabTiled(tab1));
  ASSERT_TRUE(data().IsTabTiled(tab2));

  // When one of tab is pinned,
  tab_strip_model()->SetTabPinned(tab_strip_model()->GetIndexOfTab(tab1),
                                  /*pinned*/ true);

  // Then the other tab should be pinned together.
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(
      tab_strip_model()->IsTabPinned(tab_strip_model()->GetIndexOfTab(tab2)));

  // Also unpinning is synced too.
  tab_strip_model()->SetTabPinned(1, /*pinned*/ false);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(
      tab_strip_model()->IsTabPinned(tab_strip_model()->GetIndexOfTab(tab1)));

  // This also should be work the same with the other tab.
  tab_strip_model()->SetTabPinned(tab_strip_model()->GetIndexOfTab(tab2),
                                  /*pinned*/ true);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(
      tab_strip_model()->IsTabPinned(tab_strip_model()->GetIndexOfTab(tab1)));

  tab_strip_model()->SetTabPinned(tab_strip_model()->GetIndexOfTab(tab2),
                                  /*pinned*/ false);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(
      tab_strip_model()->IsTabPinned(tab_strip_model()->GetIndexOfTab(tab1)));
}

IN_PROC_BROWSER_TEST_F(SplitViewTabStripModelAdapterBrowserTest,
                       TabPinnedStateChanged_IndexIsSynced) {
  // Given that two tabs are tiled
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground*/ true);
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground*/ true);
  tab_strip_model()->AppendWebContents(CreateWebContents(),
                                       /*foreground*/ true);
  tab_strip_model()->SetTabPinned(0, /*pinned*/ true);
  data().TileTabs({tab_strip_model()->GetTabHandleAt(2),
                   tab_strip_model()->GetTabHandleAt(3)});
  auto non_tiled_tab = tab_strip_model()->GetTabHandleAt(1);
  auto tab1 = tab_strip_model()->GetTabHandleAt(2);
  auto tab2 = tab_strip_model()->GetTabHandleAt(3);

  // |pin|                           |
  // | x | non_tiled_tab, tab1, tab2 |
  ASSERT_TRUE(data().IsTabTiled(tab1));
  ASSERT_TRUE(data().IsTabTiled(tab2));

  // When one of tab is pinned,
  tab_strip_model()->SetTabPinned(tab_strip_model()->GetIndexOfTab(tab1),
                                  /*pinned*/ true);

  // Then the other tab should be pinned and moved to together.
  // |     pin        |               |
  // | x,  tab1, tab2 | non_tiled_tab |
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, tab_strip_model()->GetIndexOfTab(tab1));
  EXPECT_EQ(2, tab_strip_model()->GetIndexOfTab(tab2));

  // Also unpinning is synced too.
  tab_strip_model()->SetTabPinned(
      tab_strip_model()->GetIndexOfTab(non_tiled_tab), true);
  tab_strip_model()->SetTabPinned(tab_strip_model()->GetIndexOfTab(tab1),
                                  /*pinned*/ false);

  // |        pin       |            |
  // | x, non_tiled_tab | tab1, tab2 |
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, tab_strip_model()->GetIndexOfTab(tab1));
  EXPECT_EQ(3, tab_strip_model()->GetIndexOfTab(tab2));

  // This should work when the right tab's pinned state changes
  tab_strip_model()->SetTabPinned(
      tab_strip_model()->GetIndexOfTab(non_tiled_tab), false);
  tab_strip_model()->SetTabPinned(tab_strip_model()->GetIndexOfTab(tab2),
                                  /*pinned*/ true);

  // |      pin       |               |
  // | x , tab1, tab2 | non_tiled_tab |
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, tab_strip_model()->GetIndexOfTab(tab1));
  EXPECT_EQ(2, tab_strip_model()->GetIndexOfTab(tab2));

  tab_strip_model()->SetTabPinned(3, true);
  tab_strip_model()->SetTabPinned(tab_strip_model()->GetIndexOfTab(tab2),
                                  /*pinned*/ false);

  // |         pin       |            |
  // | x , non_tiled_tab | tab1, tab2 |
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, tab_strip_model()->GetIndexOfTab(tab1));
  EXPECT_EQ(3, tab_strip_model()->GetIndexOfTab(tab2));
}

IN_PROC_BROWSER_TEST_F(SplitViewTabStripModelAdapterBrowserTest,
                       TabGroupedStateChanged) {
  // Given that tabs are tiled in a group,
  const auto group_id = tab_groups::TabGroupId::GenerateNew();
  ASSERT_TRUE(tab_strip_model()->group_model());
  tab_strip_model()->group_model()->AddTabGroup(group_id, std::nullopt);

  tab_strip_model()->InsertWebContentsAt(-1, CreateWebContents(),
                                         /*add_types*/ 0, group_id);
  tab_strip_model()->InsertWebContentsAt(-1, CreateWebContents(),
                                         /*add_types*/ 0, group_id);
  tab_strip_model()->InsertWebContentsAt(-1, CreateWebContents(),
                                         /*add_types*/ 0, group_id);
  data().TileTabs({tab_strip_model()->GetTabHandleAt(1),
                   tab_strip_model()->GetTabHandleAt(2)});

  auto tab1 = tab_strip_model()->GetTabHandleAt(1);
  auto tab2 = tab_strip_model()->GetTabHandleAt(2);

  // When removing a tab from a group,
  tab_strip_model()->RemoveFromGroup({1});

  // Then the other should be inserted to the group together.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(tab_strip_model()->GetTabGroupForTab(
      tab_strip_model()->GetIndexOfTab(tab2)));

  // When adding a tab to a group,
  tab_strip_model()->AddToExistingGroup({2}, group_id);

  // Then The other tab should be grouped too
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(tab_strip_model()->GetTabGroupForTab(1));
  EXPECT_EQ(1, tab_strip_model()->GetIndexOfTab(tab1));
  EXPECT_EQ(2, tab_strip_model()->GetIndexOfTab(tab2));
}

IN_PROC_BROWSER_TEST_F(SplitViewTabStripModelAdapterBrowserTest,
                       OnTabMoved_TileShouldBeBrokenWhenTabMovedBetweenTile) {
  // Given that two tabs are tiled and there's a non-tiled tab
  tab_strip_model()->AddWebContents(CreateWebContents(), -1,
                                    ui::PageTransition::PAGE_TRANSITION_TYPED,
                                    /*add_types=*/0);
  tab_strip_model()->AddWebContents(CreateWebContents(), -1,
                                    ui::PageTransition::PAGE_TRANSITION_TYPED,
                                    /*add_types=*/0);
  data().TileTabs({tab_strip_model()->GetTabHandleAt(0),
                   tab_strip_model()->GetTabHandleAt(1)});
  ASSERT_TRUE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(0)));
  ASSERT_TRUE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(1)));
  ASSERT_FALSE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(2)));

  // When moving non-tiled tab between tiled tabs
  adapter().TabDragStarted();
  tab_strip_model()->MoveWebContentsAt(2, 1, /*select_after_move*/ false);

  // Then the tile should be kept during drag and drop session
  EXPECT_TRUE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(0)));
  EXPECT_FALSE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(1)));
  EXPECT_TRUE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(2)));

  // When drag-and-drop session ends
  adapter().TabDragEnded();

  // Then the tile should be broken.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(0)));
  EXPECT_FALSE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(1)));
  EXPECT_FALSE(data().IsTabTiled(tab_strip_model()->GetTabHandleAt(2)));
}
