/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/split_view_browser_data.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

class SplitViewBrowserDataBrowserTest : public InProcessBrowserTest {
 public:
  SplitViewBrowserDataBrowserTest()
      : feature_list_(tabs::features::kBraveSplitView) {}
  ~SplitViewBrowserDataBrowserTest() override = default;

  SplitViewBrowserData& data() {
    return *SplitViewBrowserData::FromBrowser(browser());
  }

  tabs::TabModel* CreateTabModel() {
    content::WebContents::CreateParams params(browser()->profile());
    auto web_contents = content::WebContents::Create(params);
    CHECK(web_contents);
    auto tab_model = std::make_unique<tabs::TabModel>(
        std::move(web_contents), browser()->tab_strip_model());
    auto* result = tab_model.get();
    browser()->tab_strip_model()->AppendTab(std::move(tab_model),
                                            /*foreground=*/true);
    return result;
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(SplitViewBrowserDataBrowserTest, TileTabs_AddsTile) {
  auto* tab_1 = CreateTabModel();
  auto* tab_2 = CreateTabModel();
  EXPECT_FALSE(data().IsTabTiled(tab_1->GetHandle()));
  EXPECT_FALSE(data().IsTabTiled(tab_2->GetHandle()));

  data().TileTabs({.first = tab_1->GetHandle(), .second = tab_2->GetHandle()});

  EXPECT_TRUE(data().IsTabTiled(tab_1->GetHandle()));
  EXPECT_TRUE(data().IsTabTiled(tab_2->GetHandle()));
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserDataBrowserTest, BreakTile_RemovesTile) {
  auto* tab_1 = CreateTabModel();
  auto* tab_2 = CreateTabModel();
  data().TileTabs({.first = tab_1->GetHandle(), .second = tab_2->GetHandle()});

  ASSERT_TRUE(data().IsTabTiled(tab_1->GetHandle()));
  ASSERT_TRUE(data().IsTabTiled(tab_2->GetHandle()));

  data().BreakTile(tab_1->GetHandle());
  EXPECT_FALSE(data().IsTabTiled(tab_1->GetHandle()));
  EXPECT_FALSE(data().IsTabTiled(tab_2->GetHandle()));

  data().TileTabs({.first = tab_1->GetHandle(), .second = tab_2->GetHandle()});
  data().BreakTile(tab_2->GetHandle());
  EXPECT_FALSE(data().IsTabTiled(tab_1->GetHandle()));
  EXPECT_FALSE(data().IsTabTiled(tab_2->GetHandle()));
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserDataBrowserTest, FindTile) {
  auto* tab_1 = CreateTabModel();
  auto* tab_2 = CreateTabModel();
  data().TileTabs({.first = tab_1->GetHandle(), .second = tab_2->GetHandle()});

  EXPECT_EQ(0, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_1->GetHandle())));
  EXPECT_EQ(0, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_2->GetHandle())));

  data().BreakTile(tab_2->GetHandle());
  EXPECT_EQ(data().tiles_.end(), data().FindTile(tab_1->GetHandle()));
  EXPECT_EQ(data().tiles_.end(), data().FindTile(tab_2->GetHandle()));

  auto* tab_3 = CreateTabModel();
  auto* tab_4 = CreateTabModel();
  data().TileTabs({.first = tab_1->GetHandle(), .second = tab_2->GetHandle()});
  data().TileTabs({.first = tab_3->GetHandle(), .second = tab_4->GetHandle()});
  EXPECT_EQ(1, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_3->GetHandle())));
  EXPECT_EQ(1, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_4->GetHandle())));

  data().BreakTile(tab_1->GetHandle());
  EXPECT_EQ(0, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_3->GetHandle())));
  EXPECT_EQ(0, std::distance(data().tiles_.begin(),
                             data().FindTile(tab_4->GetHandle())));
}
