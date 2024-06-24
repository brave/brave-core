/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/features.h"
#include "ui/views/view.h"

namespace {

int ClampSplitViewSizeDelta(views::View* contents_view,
                            int size_delta,
                            SplitViewBrowserData::Orientation orientation) {
  constexpr int kMinSize = 144;  // From 144p resolution.
  const auto half_size =
      (orientation == SplitViewBrowserData::Orientation::kVertical
           ? contents_view->width()
           : contents_view->height() -
                 BraveContentsLayoutManager::kSpacingBetweenContentsWebViews) /
      2;

  return std::clamp(size_delta, /*min*/ kMinSize - half_size,
                    /*max*/ half_size - kMinSize);
}

}  // namespace

BraveContentsLayoutManager::~BraveContentsLayoutManager() = default;

void BraveContentsLayoutManager::SetSplitViewSeparator(
    SplitViewSeparator* split_view_separator) {
  split_view_separator_ = split_view_separator;
  split_view_separator_->set_delegate(this);
}

void BraveContentsLayoutManager::SetSplitViewOrientation(
    SplitViewBrowserData::Orientation orientation) {
  if (orientation_ == orientation) {
    return;
  }

  orientation_ = orientation;
  if (split_view_separator_) {
    split_view_separator_->SetOrientation(orientation_);
  }
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
    split_view_size_delta_ = ClampSplitViewSizeDelta(
        host_view(), split_view_size_delta_, orientation_);
    ongoing_split_view_size_delta_ = 0;
  }

  LayoutImpl();
}

void BraveContentsLayoutManager::LayoutImpl() {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveSplitView) ||
      !secondary_contents_view_ || !secondary_devtools_view_ ||
      !secondary_contents_view_->GetVisible()) {
    ContentsLayoutManager::LayoutImpl();
    return;
  }

  auto layout_web_contents_and_devtools =
      [](gfx::Rect bounds, views::View* contents_view,
         views::View* devtools_view,
         const DevToolsContentsResizingStrategy& strategy,
         SplitViewBrowserData::Orientation orientation) {
        gfx::Rect new_contents_bounds;
        gfx::Rect new_devtools_bounds;
        ApplyDevToolsContentsResizingStrategy(strategy, bounds.size(),
                                              &new_devtools_bounds,
                                              &new_contents_bounds);
        if (orientation == SplitViewBrowserData::Orientation::kVertical) {
          new_contents_bounds.set_x(bounds.x() + new_contents_bounds.x());
          new_devtools_bounds.set_x(bounds.x() + new_devtools_bounds.x());
        } else {
          new_contents_bounds.set_y(bounds.y() + new_contents_bounds.y());
          new_devtools_bounds.set_y(bounds.y() + new_devtools_bounds.y());
        }

        // TODO(sko) We're ignoring dev tools specific position. Maybe we need
        // to revisit this. On the other hand, I think we shouldn't let devtools
        // on the side of split view as it's too confusing.
        contents_view->SetBoundsRect(new_contents_bounds);
        devtools_view->SetBoundsRect(new_devtools_bounds);
      };

  gfx::Rect bounds = host_view()->GetLocalBounds();
  const auto size_delta = ClampSplitViewSizeDelta(
      host_view(), split_view_size_delta_ + ongoing_split_view_size_delta_,
      orientation_);

  if (orientation_ == SplitViewBrowserData::Orientation::kVertical) {
    bounds.set_width((bounds.width() - kSpacingBetweenContentsWebViews) / 2 +
                     size_delta);
  } else {
    bounds.set_height((bounds.height() - kSpacingBetweenContentsWebViews) / 2 +
                      size_delta);
  }
  if (show_main_web_contents_at_tail_) {
    layout_web_contents_and_devtools(bounds, secondary_contents_view_,
                                     secondary_devtools_view_,
                                     secondary_strategy_, orientation_);
  } else {
    layout_web_contents_and_devtools(bounds, contents_view_, devtools_view_,
                                     strategy_, orientation_);
  }

  if (orientation_ == SplitViewBrowserData::Orientation::kVertical) {
    bounds.set_x(bounds.right());
    bounds.set_width(kSpacingBetweenContentsWebViews);
  } else {
    bounds.set_y(bounds.bottom());
    bounds.set_height(kSpacingBetweenContentsWebViews);
  }
  split_view_separator_->SetBoundsRect(bounds);

  if (orientation_ == SplitViewBrowserData::Orientation::kVertical) {
    bounds.set_x(bounds.right());
    bounds.set_width(host_view()->width() - bounds.x());
  } else {
    bounds.set_y(bounds.bottom());
    bounds.set_height(host_view()->height() - bounds.y());
  }
  if (show_main_web_contents_at_tail_) {
    layout_web_contents_and_devtools(bounds, contents_view_, devtools_view_,
                                     strategy_, orientation_);
  } else {
    layout_web_contents_and_devtools(bounds, secondary_contents_view_,
                                     secondary_devtools_view_,
                                     secondary_strategy_, orientation_);
  }
}
