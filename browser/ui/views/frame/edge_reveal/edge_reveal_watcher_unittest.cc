/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/edge_reveal/edge_reveal_watcher.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {

using RevealReason = EdgeRevealWatcher::RevealReason;

}  // namespace

class EdgeRevealWatcherTest : public views::test::WidgetTest {
 protected:
  void SetUp() override {
    WidgetTest::SetUp();

    host_widget_.reset(CreateTopLevelNativeWidget(
        views::Widget::InitParams::CLIENT_OWNS_WIDGET));
    host_widget_->SetSize(gfx::Size(400, 400));

    auto* contents_view = host_widget_->GetClientContentsView();
    CHECK(contents_view);

    edge_view_ = contents_view->AddChildView(std::make_unique<views::View>());
    non_edge_view_ =
        contents_view->AddChildView(std::make_unique<views::View>());
    edge_view_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
    non_edge_view_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

    host_widget_->Show();

    watcher_ = std::make_unique<EdgeRevealWatcher>(
        host_widget_.get(),
        base::BindRepeating(&EdgeRevealWatcherTest::EdgeContainsView,
                            base::Unretained(this)),
        base::BindRepeating(&EdgeRevealWatcherTest::OnReasonsChanged,
                            base::Unretained(this)));
  }

  void TearDown() override {
    watcher_.reset();
    CloseHostWidget();
    WidgetTest::TearDown();
  }

  bool EdgeContainsView(const views::View* view) {
    return edge_view_ && edge_view_->Contains(view);
  }

  void OnReasonsChanged(EdgeRevealWatcher::RevealReasons reasons) {
    reason_changes_.push_back(reasons);
  }

  std::unique_ptr<views::Widget> CreateVisibleChildWidget() {
    std::unique_ptr<views::Widget> widget(CreateChildNativeWidgetWithParent(
        host_widget_.get(), views::Widget::InitParams::CLIENT_OWNS_WIDGET));
    widget->Show();
    return widget;
  }

  std::unique_ptr<views::Widget> CreateBubbleAnchoredTo(views::View* anchor) {
    auto delegate = std::make_unique<views::BubbleDialogDelegate>(
        views::BubbleAnchor(anchor), views::BubbleBorder::TOP_LEFT);
    delegate->SetContentsView(std::make_unique<views::View>());
    delegate->set_close_on_deactivate(false);
    std::unique_ptr<views::Widget> bubble_widget(
        views::BubbleDialogDelegate::CreateBubbleDeprecated(
            std::move(delegate),
            views::Widget::InitParams::CLIENT_OWNS_WIDGET));
    bubble_widget->Show();
    return bubble_widget;
  }

  void CloseHostWidget() {
    edge_view_ = nullptr;
    non_edge_view_ = nullptr;
    host_widget_.reset();
  }

  std::unique_ptr<EdgeRevealWatcher> watcher_;
  std::vector<EdgeRevealWatcher::RevealReasons> reason_changes_;
  std::unique_ptr<views::Widget> host_widget_;
  raw_ptr<views::View> edge_view_ = nullptr;
  raw_ptr<views::View> non_edge_view_ = nullptr;
};

TEST_F(EdgeRevealWatcherTest, InitiallyNoReasons) {
  EXPECT_TRUE(watcher_->reasons().empty());
  EXPECT_TRUE(reason_changes_.empty());
}

TEST_F(EdgeRevealWatcherTest, VisibleNonBubbleChildSetsVisibleChildWidget) {
  auto child = CreateVisibleChildWidget();

  EXPECT_TRUE(watcher_->reasons().Has(RevealReason::kVisibleChildWidget));
  EXPECT_FALSE(watcher_->reasons().Has(RevealReason::kAnchoredBubble));
  EXPECT_EQ(reason_changes_.size(), 1u);
}

TEST_F(EdgeRevealWatcherTest, HidingChildWidgetClearsReason) {
  auto child = CreateVisibleChildWidget();
  ASSERT_TRUE(watcher_->reasons().Has(RevealReason::kVisibleChildWidget));

  child->Hide();
  EXPECT_FALSE(watcher_->reasons().Has(RevealReason::kVisibleChildWidget));
}

TEST_F(EdgeRevealWatcherTest, DestroyingTrackedChildWidgetClearsReason) {
  auto child = CreateVisibleChildWidget();
  ASSERT_TRUE(watcher_->reasons().Has(RevealReason::kVisibleChildWidget));
  child.reset();
  EXPECT_TRUE(watcher_->reasons().empty());
}

TEST_F(EdgeRevealWatcherTest, BubbleAnchoredInEdgeSetsAnchoredBubble) {
  auto bubble = CreateBubbleAnchoredTo(edge_view_);
  EXPECT_TRUE(watcher_->reasons().Has(RevealReason::kAnchoredBubble));
  EXPECT_FALSE(watcher_->reasons().Has(RevealReason::kVisibleChildWidget));
}

TEST_F(EdgeRevealWatcherTest, BubbleAnchoredOutsideEdgeSetsVisibleChildWidget) {
  auto bubble = CreateBubbleAnchoredTo(non_edge_view_);
  EXPECT_TRUE(watcher_->reasons().Has(RevealReason::kVisibleChildWidget));
  EXPECT_FALSE(watcher_->reasons().Has(RevealReason::kAnchoredBubble));
}

TEST_F(EdgeRevealWatcherTest, FocusInEdgeSetsFocusReason) {
  edge_view_->RequestFocus();
  EXPECT_TRUE(watcher_->reasons().Has(RevealReason::kFocus));
}

TEST_F(EdgeRevealWatcherTest, FocusOutsideEdgeDoesNotSetFocusReason) {
  non_edge_view_->RequestFocus();
  EXPECT_FALSE(watcher_->reasons().Has(RevealReason::kFocus));
}

TEST_F(EdgeRevealWatcherTest, FocusMovingOutOfEdgeClearsFocusReason) {
  edge_view_->RequestFocus();
  ASSERT_TRUE(watcher_->reasons().Has(RevealReason::kFocus));

  non_edge_view_->RequestFocus();
  EXPECT_FALSE(watcher_->reasons().Has(RevealReason::kFocus));
}

TEST_F(EdgeRevealWatcherTest, MultipleReasonsCoexist) {
  auto child = CreateVisibleChildWidget();
  edge_view_->RequestFocus();

  EXPECT_TRUE(watcher_->reasons().Has(RevealReason::kFocus));
  EXPECT_TRUE(watcher_->reasons().Has(RevealReason::kVisibleChildWidget));
}

TEST_F(EdgeRevealWatcherTest, NoCallbackWhenReasonsUnchanged) {
  auto a = CreateVisibleChildWidget();
  size_t change_count_after_first = reason_changes_.size();

  auto b = CreateVisibleChildWidget();
  EXPECT_EQ(change_count_after_first, reason_changes_.size());
}

TEST_F(EdgeRevealWatcherTest, HostDestructionWhileWatcherExistsDoesNotCrash) {
  auto child = CreateVisibleChildWidget();
  CloseHostWidget();
}
