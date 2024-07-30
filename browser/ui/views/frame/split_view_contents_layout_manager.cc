/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view_contents_layout_manager.h"

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "ui/views/view.h"

namespace {

int ClampSplitViewSizeDelta(views::View* contents_view, int size_delta) {
  constexpr int kMinWidth = 144;  // From 144p resolution.
  const auto half_size =
      (contents_view->width() -
       SplitViewContentsLayoutManager::kSpacingBetweenContentsWebViews) /
      2;

  return std::clamp(size_delta, /*min*/ kMinWidth - half_size,
                    /*max*/ half_size - kMinWidth);
}

}  // namespace

SplitViewContentsLayoutManager::SplitViewContentsLayoutManager(
    views::View* devtools_view,
    views::View* contents_view,
    views::View* watermark_view)
    : BraveContentsLayoutManager(devtools_view, contents_view, watermark_view) {
}

SplitViewContentsLayoutManager::~SplitViewContentsLayoutManager() = default;

void SplitViewContentsLayoutManager::SetSplitViewSeparator(
    SplitViewSeparator* split_view_separator) {
  split_view_separator_ = split_view_separator;
  split_view_separator_->set_delegate(this);
}

void SplitViewContentsLayoutManager::SetSecondaryContentsResizingStrategy(
    const DevToolsContentsResizingStrategy& strategy) {
  if (secondary_strategy_.Equals(strategy)) {
    return;
  }

  secondary_strategy_.CopyFrom(strategy);
  if (host_view()) {
    host_view()->InvalidateLayout();
  }
}

void SplitViewContentsLayoutManager::OnDoubleClicked() {
  split_view_size_delta_ = ongoing_split_view_size_delta_ = 0;
  LayoutImpl();
}

void SplitViewContentsLayoutManager::OnResize(int resize_amount,
                                              bool done_resizing) {
  ongoing_split_view_size_delta_ = resize_amount;
  if (done_resizing) {
    split_view_size_delta_ += ongoing_split_view_size_delta_;
    split_view_size_delta_ =
        ClampSplitViewSizeDelta(host_view(), split_view_size_delta_);
    ongoing_split_view_size_delta_ = 0;
  }

  LayoutImpl();
}

void SplitViewContentsLayoutManager::LayoutImpl() {
  const bool is_host_empty = !host_view()->width();
  if (is_host_empty) {
    // When minimizing window, this can happen
    return;
  }

  if (!base::FeatureList::IsEnabled(tabs::features::kBraveSplitView) ||
      !secondary_contents_view_ || !secondary_devtools_view_ ||
      !secondary_contents_view_->GetVisible()) {
    BraveContentsLayoutManager::LayoutImpl();
    return;
  }

  gfx::Rect bounds = host_view()->GetLocalBounds();

  const auto size_delta = ClampSplitViewSizeDelta(
      host_view(), split_view_size_delta_ + ongoing_split_view_size_delta_);
  bounds.set_width((bounds.width() - kSpacingBetweenContentsWebViews) / 2 +
                   size_delta);
  if (show_main_web_contents_at_tail_) {
    LayoutContents(bounds, secondary_contents_view_,
                   contents_reader_mode_toolbar_, secondary_devtools_view_,
                   secondary_strategy_);
  } else {
    LayoutContents(bounds, contents_view_, contents_reader_mode_toolbar_,
                   devtools_view_, strategy_);
  }

  bounds.set_x(bounds.right());
  bounds.set_width(kSpacingBetweenContentsWebViews);
  split_view_separator_->SetBoundsRect(bounds);

  bounds.set_x(bounds.right());
  bounds.set_width(host_view()->width() - bounds.x());
  if (show_main_web_contents_at_tail_) {
    LayoutContents(bounds, contents_view_,
                   secondary_contents_reader_mode_toolbar_, devtools_view_,
                   strategy_);
  } else {
    LayoutContents(bounds, secondary_contents_view_,
                   secondary_contents_reader_mode_toolbar_,
                   secondary_devtools_view_, secondary_strategy_);
  }

  if (browser_view_) {
    browser_view_->NotifyDialogPositionRequiresUpdate();
  }
}
