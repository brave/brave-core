// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/tabs/tab_model.h"

#include <memory>

#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/test_tab_strip_model_delegate.h"
#include "chrome/browser/ui/tabs/test_util.h"
#include "chrome/browser/universal_web_contents_observers.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tabs {

class TabModelUnitTest : public testing::Test {
 protected:
  void SetUp() override {
    delegate_ = std::make_unique<TestTabStripModelDelegate>();
    tab_strip_model_ =
        std::make_unique<TabStripModel>(delegate_.get(), &profile_);

    auto contents =
        content::WebContentsTester::CreateTestWebContents(&profile_, nullptr);
    // As our unittests doesn't use ChromeContentBrowserClient, we should attach
    // web contents observers manually. This is needed to create TabModel
    // correctly.
    AttachUniversalWebContentsObservers(contents.get());

    auto tab_model =
        std::make_unique<TabModel>(std::move(contents), tab_strip_model_.get());
    tab_model_ = tab_model.get();
    tab_strip_model_->AppendTab(std::move(tab_model), /* foreground = */
                                true);
  }

  base::test::ScopedFeatureList feature_list_{
      containers::features::kBraveContainers};

  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler rvh_test_enabler_;
  TestingProfile profile_;

  std::unique_ptr<TestTabStripModelDelegate> delegate_;
  std::unique_ptr<TabStripModel> tab_strip_model_;
  raw_ptr<TabModel> tab_model_ = nullptr;

  tabs::PreventTabFeatureInitialization prevent_;
};

TEST_F(TabModelUnitTest, DefaultIsNotPartitioned) {
  // By default, IsPartitionedTab should be false.
  EXPECT_FALSE(tab_model_->IsPartitionedTab());
  EXPECT_FALSE(tab_model_->GetPartitionedTabVisualData().has_value());
}

TEST_F(TabModelUnitTest, SetAndGetPartitionedTabVisualData) {
  PartitionedTabVisualData data;
  tab_model_->SetPartitionedTabVisualData(data);
  EXPECT_TRUE(tab_model_->IsPartitionedTab());
  auto result = tab_model_->GetPartitionedTabVisualData();
  EXPECT_TRUE(result.has_value());
}

TEST_F(TabModelUnitTest, ResetPartitionedTabVisualData) {
  PartitionedTabVisualData data;
  tab_model_->SetPartitionedTabVisualData(data);
  EXPECT_TRUE(tab_model_->IsPartitionedTab());

  // Reset to nullopt
  tab_model_->SetPartitionedTabVisualData(std::nullopt);
  EXPECT_FALSE(tab_model_->IsPartitionedTab());
  EXPECT_FALSE(tab_model_->GetPartitionedTabVisualData().has_value());
}

}  // namespace tabs
