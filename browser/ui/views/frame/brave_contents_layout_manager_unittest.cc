/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include <memory>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/split_view/split_view_separator.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/controls/webview/webview.h"

class BraveContentsLayoutManagerUnitTest : public testing::Test {
 public:
  BraveContentsLayoutManagerUnitTest()
      : scoped_feature_(tabs::features::kBraveSplitView) {}
  ~BraveContentsLayoutManagerUnitTest() override = default;

  void Layout() { layout_manager_->Layout(contents_container_.get()); }

  views::View& contents_container() { return *contents_container_; }
  views::WebView& contents_view() { return *contents_view_; }
  views::WebView& secondary_contents_view() {
    return *secondary_contents_view_;
  }
  BraveContentsLayoutManager& layout_manager() { return *layout_manager_; }

  // testing::Test:
  void SetUp() override {
    contents_container_ = std::make_unique<views::View>();
    contents_container_->SetSize(gfx::Size(300, 300));

    contents_view_ = contents_container_->AddChildView(
        std::make_unique<views::WebView>(nullptr));
    devtools_view_ = contents_container_->AddChildView(
        std::make_unique<views::WebView>(nullptr));
    secondary_contents_view_ = contents_container_->AddChildView(
        std::make_unique<views::WebView>(nullptr));
    secondary_devtools_view_ = contents_container_->AddChildView(
        std::make_unique<views::WebView>(nullptr));
    split_view_separator_ = contents_container_->AddChildView(
        std::make_unique<SplitViewSeparator>(nullptr));

    auto layout_manager = std::make_unique<BraveContentsLayoutManager>(
        contents_view_, devtools_view_);
    layout_manager->set_secondary_contents_view(secondary_contents_view_);
    layout_manager->set_secondary_devtools_view(secondary_devtools_view_);
    layout_manager->SetSplitViewSeparator(split_view_separator_);
    layout_manager_ = layout_manager.get();

    contents_container_->SetLayoutManager(std::move(layout_manager));
  }

  void TearDown() override { contents_container_.reset(); }

 private:
  base::test::ScopedFeatureList scoped_feature_;

  std::unique_ptr<views::View> contents_container_;
  raw_ptr<views::WebView> contents_view_ = nullptr;
  raw_ptr<views::WebView> devtools_view_ = nullptr;
  raw_ptr<views::WebView> secondary_contents_view_ = nullptr;
  raw_ptr<views::WebView> secondary_devtools_view_ = nullptr;
  raw_ptr<SplitViewSeparator> split_view_separator_ = nullptr;
  raw_ptr<BraveContentsLayoutManager> layout_manager_ = nullptr;
};

TEST_F(BraveContentsLayoutManagerUnitTest,
       Size_SecondaryContentsViewInvisible) {
  // Given secondary web view is invisible
  secondary_contents_view().SetVisible(false);

  // When
  Layout();

  // Then contents_view_ should take all width
  EXPECT_EQ(contents_container().size(), contents_view().size());
}

TEST_F(BraveContentsLayoutManagerUnitTest, Size_SecondaryContentsViewVisible) {
  // Given secondary web view is visible
  secondary_contents_view().SetVisible(true);

  // When
  Layout();

  // Then contents_view_ should take the half and secondary_contents_view_
  // should the rest of the width.
  EXPECT_EQ((contents_container().width() -
             BraveContentsLayoutManager::kSpacingBetweenContentsWebViews) /
                2,
            contents_view().width());
  EXPECT_EQ(contents_container().width() - contents_view().width() -
                BraveContentsLayoutManager::kSpacingBetweenContentsWebViews,
            secondary_contents_view().width());
}

TEST_F(BraveContentsLayoutManagerUnitTest, MainContentsAtHead) {
  // Given
  ASSERT_TRUE(secondary_contents_view().GetVisible());
  layout_manager().show_main_web_contents_at_tail(false);

  // When
  Layout();

  // Then
  EXPECT_GT(secondary_contents_view().x(), contents_view().x());
}

TEST_F(BraveContentsLayoutManagerUnitTest, MainContentsAtTail) {
  // Given
  ASSERT_TRUE(secondary_contents_view().GetVisible());
  layout_manager().show_main_web_contents_at_tail(true);

  // When
  Layout();

  // Then
  EXPECT_LT(secondary_contents_view().x(),
            contents_view().GetMirroredBounds().x());
}
