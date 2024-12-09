/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_layout_manager.h"

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/split_view/split_view_separator.h"
#include "ui/views/view.h"

namespace {

int ClampSplitViewSizeDelta(const views::View* contents_view, int size_delta) {
  constexpr int kMinWidth = 144;  // From 144p resolution.
  const auto half_size =
      (contents_view->width() -
       SplitViewLayoutManager::kSpacingBetweenContentsWebViews) /
      2;

  return std::clamp(size_delta, /*min*/ kMinWidth - half_size,
                    /*max*/ half_size - kMinWidth);
}

}  // namespace

SplitViewLayoutManager::SplitViewLayoutManager(
    views::View* contents_container,
    views::View* secondary_contents_container,
    SplitViewSeparator* split_view_separator)
    : contents_container_(contents_container),
      secondary_contents_container_(secondary_contents_container),
      split_view_separator_(split_view_separator) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveSplitView));
  split_view_separator_->set_delegate(this);
}

SplitViewLayoutManager::~SplitViewLayoutManager() = default;

void SplitViewLayoutManager::OnDoubleClicked() {
  split_view_size_delta_ = ongoing_split_view_size_delta_ = 0;
  InvalidateHost(true);
}

void SplitViewLayoutManager::OnResize(int resize_amount, bool done_resizing) {
  ongoing_split_view_size_delta_ = resize_amount;
  if (done_resizing) {
    split_view_size_delta_ += ongoing_split_view_size_delta_;
    split_view_size_delta_ =
        ClampSplitViewSizeDelta(host_view(), split_view_size_delta_);
    ongoing_split_view_size_delta_ = 0;
  }

  InvalidateHost(true);
}

views::ProposedLayout SplitViewLayoutManager::CalculateProposedLayout(
    const views::SizeBounds& size_bounds) const {
  views::ProposedLayout layouts;
  if (!size_bounds.is_fully_bounded()) {
    return layouts;
  }

  int height = size_bounds.height().value();
  int width = size_bounds.width().value();

  const gfx::Size container_size(width, height);
  layouts.host_size = container_size;
  gfx::Rect bounds(container_size);

  auto add_to_child_layout = [&layouts, &container_size,
                              host_view = host_view()](
                                 views::View* child, const gfx::Rect& bounds) {
    layouts.child_layouts.emplace_back(child, child->GetVisible(),
                                       host_view->GetMirroredRect(bounds),
                                       views::SizeBounds(container_size));
  };

  if (!secondary_contents_container_->GetVisible()) {
    add_to_child_layout(contents_container_.get(), bounds);
    return layouts;
  }

  const auto size_delta = ClampSplitViewSizeDelta(
      host_view(), split_view_size_delta_ + ongoing_split_view_size_delta_);
  bounds.set_width((bounds.width() - kSpacingBetweenContentsWebViews) / 2 +
                   size_delta);
  if (show_main_web_contents_at_tail_) {
    add_to_child_layout(secondary_contents_container_.get(), bounds);
  } else {
    add_to_child_layout(contents_container_.get(), bounds);
  }

  bounds.set_x(bounds.right());
  bounds.set_width(kSpacingBetweenContentsWebViews);
  add_to_child_layout(split_view_separator_.get(), bounds);

  bounds.set_x(bounds.right());
  bounds.set_width(host_view()->width() - bounds.x());
  if (show_main_web_contents_at_tail_) {
    add_to_child_layout(contents_container_.get(), bounds);
  } else {
    add_to_child_layout(secondary_contents_container_.get(), bounds);
  }

  return layouts;
}
