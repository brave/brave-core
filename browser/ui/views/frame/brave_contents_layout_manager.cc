/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "ui/views/view.h"

namespace {

int ClampSplitViewSizeDelta(views::View* contents_view, int size_delta) {
  constexpr int kMinWidth = 144;  // From 144p resolution.
  const auto half_size =
      (contents_view->width() -
       BraveContentsLayoutManager::kSpacingBetweenContentsWebViews) /
      2;

  return std::clamp(size_delta, /*min*/ kMinWidth - half_size,
                    /*max*/ half_size - kMinWidth);
}

}  // namespace

BraveContentsLayoutManager::BraveContentsLayoutManager(
    views::View* devtools_view,
    views::View* contents_view,
    views::View* watermark_view)
    : ContentsLayoutManager(devtools_view, contents_view, watermark_view) {}

BraveContentsLayoutManager::~BraveContentsLayoutManager() = default;

void BraveContentsLayoutManager::SetSplitViewSeparator(
    SplitViewSeparator* split_view_separator) {
  split_view_separator_ = split_view_separator;
  split_view_separator_->set_delegate(this);
}

void BraveContentsLayoutManager::SetSecondaryContentsResizingStrategy(
    const DevToolsContentsResizingStrategy& strategy) {
  if (secondary_strategy_.Equals(strategy)) {
    return;
  }

  secondary_strategy_.CopyFrom(strategy);
  if (host_view()) {
    host_view()->InvalidateLayout();
  }
}

void BraveContentsLayoutManager::OnDoubleClicked() {
  split_view_size_delta_ = ongoing_split_view_size_delta_ = 0;
  LayoutImpl();
}

void BraveContentsLayoutManager::OnResize(int resize_amount,
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

void BraveContentsLayoutManager::LayoutImpl() {
  const bool is_host_empty = !host_view()->width();
  if (is_host_empty) {
    // When minimizing window, this can happen
    return;
  }

  auto layout_web_contents_and_devtools =
      [](gfx::Rect bounds, views::View* contents_view,
         views::View* reader_mode_toolbar, views::View* devtools_view,
         const DevToolsContentsResizingStrategy& strategy) {
        gfx::Rect new_contents_bounds;
        gfx::Rect new_devtools_bounds;
        ApplyDevToolsContentsResizingStrategy(strategy, bounds.size(),
                                              &new_devtools_bounds,
                                              &new_contents_bounds);
        new_contents_bounds.set_x(bounds.x() + new_contents_bounds.x());
        new_devtools_bounds.set_x(bounds.x() + new_devtools_bounds.x());

        if (reader_mode_toolbar && reader_mode_toolbar->GetVisible()) {
          gfx::Rect toolbar_bounds = new_contents_bounds;
          toolbar_bounds.set_height(
              reader_mode_toolbar->GetPreferredSize().height());
          reader_mode_toolbar->SetBoundsRect(toolbar_bounds);
          new_contents_bounds.Inset(
              gfx::Insets::TLBR(toolbar_bounds.height(), 0, 0, 0));
        }

        // TODO(sko) We're ignoring dev tools specific position. Maybe we need
        // to revisit this. On the other hand, I think we shouldn't let devtools
        // on the side of split view as it's too confusing.
        contents_view->SetBoundsRect(new_contents_bounds);
        devtools_view->SetBoundsRect(new_devtools_bounds);
      };

  gfx::Rect bounds = host_view()->GetLocalBounds();

  if (!base::FeatureList::IsEnabled(tabs::features::kBraveSplitView) ||
      !secondary_contents_view_ || !secondary_devtools_view_ ||
      !secondary_contents_view_->GetVisible()) {
    layout_web_contents_and_devtools(bounds, contents_view_,
                                     contents_reader_mode_toolbar_,
                                     devtools_view_, strategy_);
    return;
  }

  const auto size_delta = ClampSplitViewSizeDelta(
      host_view(), split_view_size_delta_ + ongoing_split_view_size_delta_);
  bounds.set_width((bounds.width() - kSpacingBetweenContentsWebViews) / 2 +
                   size_delta);
  if (show_main_web_contents_at_tail_) {
    layout_web_contents_and_devtools(bounds, secondary_contents_view_,
                                     contents_reader_mode_toolbar_,
                                     secondary_devtools_view_,
                                     secondary_strategy_);
  } else {
    layout_web_contents_and_devtools(bounds, contents_view_,
                                     contents_reader_mode_toolbar_,
                                     devtools_view_, strategy_);
  }

  bounds.set_x(bounds.right());
  bounds.set_width(kSpacingBetweenContentsWebViews);
  split_view_separator_->SetBoundsRect(bounds);

  bounds.set_x(bounds.right());
  bounds.set_width(host_view()->width() - bounds.x());
  if (show_main_web_contents_at_tail_) {
    layout_web_contents_and_devtools(bounds, contents_view_,
                                     secondary_contents_reader_mode_toolbar_,
                                     devtools_view_, strategy_);
  } else {
    layout_web_contents_and_devtools(bounds, secondary_contents_view_,
                                     secondary_contents_reader_mode_toolbar_,
                                     secondary_devtools_view_,
                                     secondary_strategy_);
  }

  if (browser_view_) {
    browser_view_->NotifyDialogPositionRequiresUpdate();
  }
}
