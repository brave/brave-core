/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/split_view_tab_strip_model_adapter.h"

#include <utility>

#include "base/run_loop.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/test_tab_strip_model_delegate.h"
#include "chrome/browser/universal_web_contents_observers.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/tab_groups/tab_group_id.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

class SplitViewTabStripModelAdapterUnitTest
    : public testing::Test,
      public content::ContentBrowserClient {
 public:
  SplitViewTabStripModelAdapterUnitTest()
      : feature_list_(tabs::features::kBraveSplitView) {}
  ~SplitViewTabStripModelAdapterUnitTest() override = default;

  TabStripModel& model() const { return *model_; }
  SplitViewBrowserData& data() const { return *split_view_browser_data_; }

  std::unique_ptr<content::WebContents> CreateWebContents() {
    auto contents =
        content::WebContentsTester::CreateTestWebContents(&profile_, nullptr);
    CHECK(contents);
    return contents;
  }

  // testing::Test:
  void SetUp() override {
    client_ = content::SetBrowserClientForTesting(this);

    delegate_ = std::make_unique<TestTabStripModelDelegate>();
    model_ = std::make_unique<TabStripModel>(delegate_.get(), &profile_);

    split_view_browser_data_.reset(new SplitViewBrowserData(nullptr));
    split_view_browser_data_->is_testing_ = true;
    split_view_browser_data_->tab_strip_model_adapter_ =
        std::make_unique<SplitViewTabStripModelAdapter>(
            *split_view_browser_data_, model_.get());
    adapter_ = split_view_browser_data_->tab_strip_model_adapter_.get();
  }

  void TearDown() override {
    split_view_browser_data_.reset();
    model_.reset();
    delegate_.reset();

    content::SetBrowserClientForTesting(client_);
  }

  void OnWebContentsCreated(content::WebContents* web_contents) override {
    content::ContentBrowserClient::OnWebContentsCreated(web_contents);
    AttachUniversalWebContentsObservers(web_contents);
  }

 private:
  base::test::ScopedFeatureList feature_list_;

  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler rvh_test_enabler_;

  TestingProfile profile_;

  raw_ptr<content::ContentBrowserClient> client_;

  std::unique_ptr<TestTabStripModelDelegate> delegate_;
  std::unique_ptr<TabStripModel> model_;
  std::unique_ptr<SplitViewBrowserData> split_view_browser_data_;
  raw_ptr<SplitViewTabStripModelAdapter> adapter_;
};

TEST_F(SplitViewTabStripModelAdapterUnitTest, TilingTabsMakesTabsAdjacent) {
  // Given that there're multiple tabs
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  auto secondary_tab = model().GetTabHandleAt(3);

  // When tiling two tabs that are not adjacent,
  data().TileTabs({model().GetTabHandleAt(0), secondary_tab});
  ASSERT_TRUE(data().IsTabTiled(model().GetTabHandleAt(0)));
  ASSERT_TRUE(data().IsTabTiled(secondary_tab));

  // Then the tabs should get adjacent.
  EXPECT_EQ(1, model().GetIndexOfTab(secondary_tab));
}

TEST_F(SplitViewTabStripModelAdapterUnitTest,
       OnTabInserted_MoveTabWhenInsertedBetweenTile) {
  // Given that two tabs are tiled
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  data().TileTabs({model().GetTabHandleAt(0), model().GetTabHandleAt(1)});
  auto tab1 = model().GetTabHandleAt(0);
  auto tab2 = model().GetTabHandleAt(1);
  ASSERT_TRUE(data().IsTabTiled(tab1));
  ASSERT_TRUE(data().IsTabTiled(tab2));

  // When inserting a tab in the middle of the tile,
  auto new_contents = CreateWebContents();
  auto* new_contents_ptr = new_contents.get();
  model().InsertWebContentsAt(/*index*/ 1, std::move(new_contents),
                              /*add_type*/ 0);
  base::RunLoop().RunUntilIdle();

  // Then the tile should stay.
  EXPECT_TRUE(data().IsTabTiled(tab1));
  EXPECT_TRUE(data().IsTabTiled(tab2));

  EXPECT_EQ(tab1, model().GetTabHandleAt(0));
  EXPECT_EQ(tab2, model().GetTabHandleAt(1));
  EXPECT_EQ(new_contents_ptr, model().GetWebContentsAt(2));
}

TEST_F(SplitViewTabStripModelAdapterUnitTest, OnTabMoved_MovesTiledTab) {
  // Given that two tabs are tiled
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  data().TileTabs({model().GetTabHandleAt(0), model().GetTabHandleAt(1)});
  auto tab1 = model().GetTabHandleAt(0);
  auto tab2 = model().GetTabHandleAt(1);
  ASSERT_TRUE(data().IsTabTiled(tab1));
  ASSERT_TRUE(data().IsTabTiled(tab2));

  // When moving the left tab to the right
  ASSERT_EQ(3, model().MoveWebContentsAt(/*from*/ 0, /*to*/ 3,
                                         /*select_after_move*/ true));

  // Then the the tab should follow.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, model().GetIndexOfTab(tab1));
  EXPECT_EQ(3, model().GetIndexOfTab(tab2));

  // when moving the left tab to the left
  ASSERT_EQ(0, model().MoveWebContentsAt(/*from*/ 2, /*to*/ 0,
                                         /*select_after_move*/ true));

  // Then the the tab should follow.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, model().GetIndexOfTab(tab1));
  EXPECT_EQ(1, model().GetIndexOfTab(tab2));

  // When moving the right tab to the right,
  ASSERT_EQ(2, model().MoveWebContentsAt(/*from*/ 1, /*to*/ 2,
                                         /*select_after_move*/ true));

  // Then the the tab should follow.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, model().GetIndexOfTab(tab1));
  EXPECT_EQ(2, model().GetIndexOfTab(tab2));

  // When moving the right tab to the left,
  ASSERT_EQ(0, model().MoveWebContentsAt(/*from*/ 1, /*to*/ 0,
                                         /*select_after_move*/ true));

  // Then the the tab should follow.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, model().GetIndexOfTab(tab1));
  EXPECT_EQ(1, model().GetIndexOfTab(tab2));
}

TEST_F(SplitViewTabStripModelAdapterUnitTest, OnTabRemoved_BreaksTile) {
  // Given that two tabs are tiled
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  data().TileTabs({model().GetTabHandleAt(0), model().GetTabHandleAt(1)});
  auto tab1 = model().GetTabHandleAt(0);
  auto tab2 = model().GetTabHandleAt(1);
  ASSERT_TRUE(data().IsTabTiled(tab1));
  ASSERT_TRUE(data().IsTabTiled(tab2));

  // When removing one of the tabs,
  model().CloseWebContentsAt(/*index*/ 0, /*close_type*/ 0);

  // Then the tile should be broken.
  EXPECT_FALSE(data().IsTabTiled(tab1));
  EXPECT_FALSE(data().IsTabTiled(tab2));
}

TEST_F(SplitViewTabStripModelAdapterUnitTest,
       TabPinnedStateChanged_PinnedStateIsSynced) {
  // Given that two tabs are tiled
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  data().TileTabs({model().GetTabHandleAt(0), model().GetTabHandleAt(1)});
  auto tab1 = model().GetTabHandleAt(0);
  auto tab2 = model().GetTabHandleAt(1);
  ASSERT_TRUE(data().IsTabTiled(tab1));
  ASSERT_TRUE(data().IsTabTiled(tab2));

  // When one of tab is pinned,
  model().SetTabPinned(model().GetIndexOfTab(tab1), /*pinned*/ true);

  // Then the other tab should be pinned together.
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(model().IsTabPinned(model().GetIndexOfTab(tab2)));

  // Also unpinning is synced too.
  model().SetTabPinned(1, /*pinned*/ false);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(model().IsTabPinned(model().GetIndexOfTab(tab1)));

  // This also should be work the same with the other tab.
  model().SetTabPinned(model().GetIndexOfTab(tab2), /*pinned*/ true);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(model().IsTabPinned(model().GetIndexOfTab(tab1)));

  model().SetTabPinned(model().GetIndexOfTab(tab2), /*pinned*/ false);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(model().IsTabPinned(model().GetIndexOfTab(tab1)));
}

TEST_F(SplitViewTabStripModelAdapterUnitTest,
       TabPinnedStateChanged_IndexIsSynced) {
  // Given that two tabs are tiled
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().AppendWebContents(CreateWebContents(), /*foreground*/ true);
  model().SetTabPinned(0, /*pinned*/ true);
  data().TileTabs({model().GetTabHandleAt(2), model().GetTabHandleAt(3)});
  auto non_tiled_tab = model().GetTabHandleAt(1);
  auto tab1 = model().GetTabHandleAt(2);
  auto tab2 = model().GetTabHandleAt(3);

  // |pin|                           |
  // | x | non_tiled_tab, tab1, tab2 |
  ASSERT_TRUE(data().IsTabTiled(tab1));
  ASSERT_TRUE(data().IsTabTiled(tab2));

  // When one of tab is pinned,
  model().SetTabPinned(model().GetIndexOfTab(tab1), /*pinned*/ true);

  // Then the other tab should be pinned and moved to together.
  // |     pin        |               |
  // | x,  tab1, tab2 | non_tiled_tab |
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, model().GetIndexOfTab(tab1));
  EXPECT_EQ(2, model().GetIndexOfTab(tab2));

  // Also unpinning is synced too.
  model().SetTabPinned(model().GetIndexOfTab(non_tiled_tab), true);
  model().SetTabPinned(model().GetIndexOfTab(tab1), /*pinned*/ false);

  // |        pin       |            |
  // | x, non_tiled_tab | tab1, tab2 |
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, model().GetIndexOfTab(tab1));
  EXPECT_EQ(3, model().GetIndexOfTab(tab2));

  // This should work when the right tab's pinned state changes
  model().SetTabPinned(model().GetIndexOfTab(non_tiled_tab), false);
  model().SetTabPinned(model().GetIndexOfTab(tab2), /*pinned*/ true);

  // |      pin       |               |
  // | x , tab1, tab2 | non_tiled_tab |
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, model().GetIndexOfTab(tab1));
  EXPECT_EQ(2, model().GetIndexOfTab(tab2));

  model().SetTabPinned(3, true);
  model().SetTabPinned(model().GetIndexOfTab(tab2), /*pinned*/ false);

  // |         pin       |            |
  // | x , non_tiled_tab | tab1, tab2 |
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, model().GetIndexOfTab(tab1));
  EXPECT_EQ(3, model().GetIndexOfTab(tab2));
}

TEST_F(SplitViewTabStripModelAdapterUnitTest, TabGroupedStateChanged) {
  // Given that tabs are tiled in a group,
  const auto group_id = tab_groups::TabGroupId::GenerateNew();
  ASSERT_TRUE(model().group_model());
  model().group_model()->AddTabGroup(group_id, std::nullopt);

  model().InsertWebContentsAt(-1, CreateWebContents(), /*add_types*/ 0,
                              group_id);
  model().InsertWebContentsAt(-1, CreateWebContents(), /*add_types*/ 0,
                              group_id);
  model().InsertWebContentsAt(-1, CreateWebContents(), /*add_types*/ 0,
                              group_id);
  data().TileTabs({model().GetTabHandleAt(0), model().GetTabHandleAt(1)});

  auto tab1 = model().GetTabHandleAt(0);
  auto tab2 = model().GetTabHandleAt(1);

  // When removing a tab from a group,
  model().RemoveFromGroup({0});

  // Then the other should be inserted to the group together.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(model().GetTabGroupForTab(model().GetIndexOfTab(tab2)));

  // When adding a tab to a group,
  model().AddToExistingGroup({1}, group_id);

  // Then The other tab should be grouped too
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(model().GetTabGroupForTab(0));
  EXPECT_EQ(0, model().GetIndexOfTab(tab1));
  EXPECT_EQ(1, model().GetIndexOfTab(tab2));
}
