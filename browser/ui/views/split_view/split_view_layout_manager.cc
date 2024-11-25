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

int ClampSplitViewSizeDelta(views::View* contents_view, int size_delta) {
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
  LayoutImpl();
}

void SplitViewLayoutManager::OnResize(int resize_amount, bool done_resizing) {
  ongoing_split_view_size_delta_ = resize_amount;
  if (done_resizing) {
    split_view_size_delta_ += ongoing_split_view_size_delta_;
    split_view_size_delta_ =
        ClampSplitViewSizeDelta(host_view(), split_view_size_delta_);
    ongoing_split_view_size_delta_ = 0;
  }

  LayoutImpl();
}

void SplitViewLayoutManager::LayoutImpl() {
  if (!secondary_contents_container_->GetVisible()) {
    views::FillLayout::LayoutImpl();
    return;
  }

  const bool is_host_empty = !host_view()->width();
  if (is_host_empty) {
    // When minimizing window, this can happen
    return;
  }

  gfx::Rect bounds = host_view()->GetLocalBounds();
  const auto size_delta = ClampSplitViewSizeDelta(
      host_view(), split_view_size_delta_ + ongoing_split_view_size_delta_);
  bounds.set_width((bounds.width() - kSpacingBetweenContentsWebViews) / 2 +
                   size_delta);
  if (show_main_web_contents_at_tail_) {
    secondary_contents_container_->SetBoundsRect(bounds);
  } else {
    contents_container_->SetBoundsRect(bounds);
  }

  bounds.set_x(bounds.right());
  bounds.set_width(kSpacingBetweenContentsWebViews);
  split_view_separator_->SetBoundsRect(bounds);

  bounds.set_x(bounds.right());
  bounds.set_width(host_view()->width() - bounds.x());
  if (show_main_web_contents_at_tail_) {
    contents_container_->SetBoundsRect(bounds);
  } else {
    secondary_contents_container_->SetBoundsRect(bounds);
  }
}
