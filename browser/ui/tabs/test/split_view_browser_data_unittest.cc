/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/split_view_browser_data.h"

#include <memory>

#include "base/test/gtest_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/chromium_src/chrome/test/base/testing_browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/test_tab_strip_model_delegate.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

class SplitViewBrowserDataUnitTest : public ::testing::Test {
 public:
  SplitViewBrowserDataUnitTest()
      : feature_list_(tabs::features::kBraveSplitView) {}
  ~SplitViewBrowserDataUnitTest() override = default;

  SplitViewBrowserData& data() { return *data_; }

  tabs::TabModel CreateTabModel() {
    auto web_contents =
        content::WebContentsTester::CreateTestWebContents(&profile_, nullptr);
    CHECK(web_contents);
    return tabs::TabModel(std::move(web_contents), tab_strip_model_.get());
  }

  // ::testing::Test:
  void SetUp() override {
    delegate_ = std::make_unique<TestTabStripModelDelegate>();
    tab_strip_model_ =
        std::make_unique<TabStripModel>(delegate_.get(), &profile_);
    data_.reset(new SplitViewBrowserData(nullptr));
    data_->is_testing_ = true;
  }

  void TearDown() override {
    data_.reset();
    tab_strip_model_.reset();
    delegate_.reset();
  }

 private:
  base::test::ScopedFeatureList feature_list_;

  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;

  std::unique_ptr<TestTabStripModelDelegate> delegate_;
  std::unique_ptr<TabStripModel> tab_strip_model_;

  std::unique_ptr<SplitViewBrowserData> data_;
};

TEST_F(SplitViewBrowserDataUnitTest, TileTabs_AddsTile) {
  auto tab_1 = CreateTabModel();
  auto tab_2 = CreateTabModel();
  EXPECT_FALSE(data().IsTabTiled(tab_1.GetHandle()));
  EXPECT_FALSE(data().IsTabTiled(tab_2.GetHandle()));

  data().TileTabs({.first = tab_1.GetHandle(), .second = tab_2.GetHandle()});

  EXPECT_TRUE(data().IsTabTiled(tab_1.GetHandle()));
  EXPECT_TRUE(data().IsTabTiled(tab_2.GetHandle()));
}

TEST_F(SplitViewBrowserDataUnitTest, TileTabs_WithAlreadyTiledTabIsError) {
  auto tab_1 = CreateTabModel();
  auto tab_2 = CreateTabModel();

  EXPECT_FALSE(data().IsTabTiled(tab_1.GetHandle()));
  EXPECT_FALSE(data().IsTabTiled(tab_2.GetHandle()));

  data().TileTabs({.first = tab_1.GetHandle(), .second = tab_2.GetHandle()});

  ASSERT_TRUE(data().IsTabTiled(tab_1.GetHandle()));
  ASSERT_TRUE(data().IsTabTiled(tab_2.GetHandle()));

  auto tab_3 = CreateTabModel();
  EXPECT_DEATH_IF_SUPPORTED(data().TileTabs({.first = tab_1.GetHandle(),
                                             .second = tab_3.GetHandle()}),
                            "");
}

TEST_F(SplitViewBrowserDataUnitTest, BreakTile_RemovesTile) {
  auto tab_1 = CreateTabModel();
  auto tab_2 = CreateTabModel();
  data().TileTabs({.first = tab_1.GetHandle(), .second = tab_2.GetHandle()});

  ASSERT_TRUE(data().IsTabTiled(tab_1.GetHandle()));
  ASSERT_TRUE(data().IsTabTiled(tab_2.GetHandle()));

  data().BreakTile(tab_1.GetHandle());
  EXPECT_FALSE(data().IsTabTiled(tab_1.GetHandle()));
  EXPECT_FALSE(data().IsTabTiled(tab_2.GetHandle()));

  data().TileTabs({.first = tab_1.GetHandle(), .second = tab_2.GetHandle()});
  data().BreakTile(tab_2.GetHandle());
  EXPECT_FALSE(data().IsTabTiled(tab_1.GetHandle()));
  EXPECT_FALSE(data().IsTabTiled(tab_2.GetHandle()));
}

TEST_F(SplitViewBrowserDataUnitTest, BreakTile_WithNonExistingTabIsError) {
  SplitViewBrowserData data = SplitViewBrowserData(nullptr);
  auto tab = CreateTabModel();
  EXPECT_DEATH_IF_SUPPORTED(data.BreakTile(tab.GetHandle()), "");
}

TEST_F(SplitViewBrowserDataUnitTest, FindTile) {
  auto tab_1 = CreateTabModel();
  auto tab_2 = CreateTabModel();
  data().TileTabs({.first = tab_1.GetHandle(), .second = tab_2.GetHandle()});

  EXPECT_EQ(0, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_1.GetHandle())));
  EXPECT_EQ(0, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_2.GetHandle())));

  data().BreakTile(tab_2.GetHandle());
  EXPECT_EQ(data().tiles_.end(), data().FindTile(tab_1.GetHandle()));
  EXPECT_EQ(data().tiles_.end(), data().FindTile(tab_2.GetHandle()));

  auto tab_3 = CreateTabModel();
  auto tab_4 = CreateTabModel();
  data().TileTabs({.first = tab_1.GetHandle(), .second = tab_2.GetHandle()});
  data().TileTabs({.first = tab_3.GetHandle(), .second = tab_4.GetHandle()});
  EXPECT_EQ(1, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_3.GetHandle())));
  EXPECT_EQ(1, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_4.GetHandle())));

  data().BreakTile(tab_1.GetHandle());
  EXPECT_EQ(0, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_3.GetHandle())));
  EXPECT_EQ(0, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_4.GetHandle())));
}
