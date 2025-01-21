/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_layout_manager.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/split_view/split_view_separator.h"
#include "testing/gtest/include/gtest/gtest.h"

class SplitViewLayoutManagerUnitTest : public testing::Test {
 public:
  SplitViewLayoutManagerUnitTest()
      : scoped_feature_(tabs::features::kBraveSplitView) {}
  ~SplitViewLayoutManagerUnitTest() override = default;

  void Layout() { layout_manager_->Layout(split_view_.get()); }

  views::View& split_view() { return *split_view_; }
  views::View& contents_container() { return *contents_container_; }
  views::View& secondary_contents_container() {
    return *secondary_contents_container_;
  }
  SplitViewLayoutManager& layout_manager() { return *layout_manager_; }

  // testing::Test:
  void SetUp() override {
    split_view_ = std::make_unique<views::View>();
    split_view_->SetSize(gfx::Size(300, 200));
    contents_container_ =
        split_view_->AddChildView(std::make_unique<views::View>());
    secondary_contents_container_ =
        split_view_->AddChildView(std::make_unique<views::View>());
    split_view_separator_ = split_view_->AddChildView(
        std::make_unique<SplitViewSeparator>(nullptr));

    layout_manager_ =
        split_view_->SetLayoutManager(std::make_unique<SplitViewLayoutManager>(
            contents_container_, secondary_contents_container_,
            split_view_separator_));
  }

 private:
  base::test::ScopedFeatureList scoped_feature_;

  std::unique_ptr<views::View> split_view_ = nullptr;
  raw_ptr<views::View> contents_container_ = nullptr;
  raw_ptr<views::View> secondary_contents_container_ = nullptr;
  raw_ptr<SplitViewSeparator> split_view_separator_ = nullptr;

  raw_ptr<SplitViewLayoutManager> layout_manager_ = nullptr;
};

TEST_F(SplitViewLayoutManagerUnitTest, Size_SecondaryContentsViewInvisible) {
  // Given secondary web view is invisible
  secondary_contents_container().SetVisible(false);

  // When
  Layout();

  // Then contents_container should take all width
  EXPECT_EQ(split_view().size(), contents_container().size());
}

TEST_F(SplitViewLayoutManagerUnitTest, Size_SecondaryContentsViewVisible) {
  // Given secondary web view is visible
  secondary_contents_container().SetVisible(true);

  // When
  Layout();

  // Then contents_container_ should take the half and
  // secondary_contents_container_ should the rest of the width.
  EXPECT_EQ((split_view().width() -
             SplitViewLayoutManager::kSpacingBetweenContentsWebViews) /
                2,
            contents_container().width());
  EXPECT_EQ(split_view().width() - contents_container().width() -
                SplitViewLayoutManager::kSpacingBetweenContentsWebViews,
            secondary_contents_container().width());
}

TEST_F(SplitViewLayoutManagerUnitTest, MainContentsAtHead) {
  // Given
  ASSERT_TRUE(secondary_contents_container().GetVisible());
  layout_manager().show_main_web_contents_at_tail(false);

  // When
  Layout();

  // Then
  EXPECT_GT(secondary_contents_container().x(), contents_container().x());
}

TEST_F(SplitViewLayoutManagerUnitTest, MainContentsAtTail) {
  // Given
  ASSERT_TRUE(secondary_contents_container().GetVisible());
  layout_manager().show_main_web_contents_at_tail(true);

  // When
  Layout();

  // Then
  EXPECT_LT(secondary_contents_container().x(),
            contents_container().GetMirroredBounds().x());
}
