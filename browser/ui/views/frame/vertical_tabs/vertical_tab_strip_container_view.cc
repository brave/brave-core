/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_container_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/check.h"
#include "base/i18n/rtl.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view_utils.h"

BraveVerticalTabStripContainerView::~BraveVerticalTabStripContainerView() {
  // Child views will be deleted after this. Marks `region_view_` nullptr
  // so that they dont' access the `region_view_` via this view.
  region_view_ = nullptr;
}

BraveVerticalTabStripContainerView::BraveVerticalTabStripContainerView(
    BrowserView* browser_view,
    views::View* host)
    : browser_view_(browser_view),
      host_(host),
      region_view_(
          AddChildView(std::make_unique<BraveVerticalTabStripRegionView>(
              browser_view_,
              views::AsViewClass<HorizontalTabStripRegionView>(
                  browser_view_->tab_strip_view())))) {
  // Needs layer to render this over the webview.
  SetPaintToLayer();

  // As we follow user's choice for vertical tab alignment,
  // we don't need to mirror this view.
  SetMirrored(false);
  SetLayoutManager(std::make_unique<views::FillLayout>());

  host_view_observation_.Observe(host_);
  ChildPreferredSizeChanged(region_view_);
}

void BraveVerticalTabStripContainerView::ChildPreferredSizeChanged(
    views::View* child) {
  if (!host_) {
    return;
  }

  // Setting minimum size for |host_| so that we can overlay vertical tabs over
  // the web view.
  host_->SetPreferredSize(region_view_->GetMinimumSize());

  // The position could be changed, so we should lay out again.
  host_->InvalidateLayout();

  // Lay out the widget manually in case the host doesn't arrange it.
  // Ex, expand on mouse hover.
  UpdateVerticalTabBounds();
}

void BraveVerticalTabStripContainerView::OnViewVisibilityChanged(
    views::View* observed_view,
    views::View* starting_view,
    bool visible) {
  UpdateVerticalTabBounds();
}

void BraveVerticalTabStripContainerView::OnViewBoundsChanged(
    views::View* observed_view) {
  UpdateVerticalTabBounds();
}

void BraveVerticalTabStripContainerView::OnViewIsDeleting(
    views::View* observed_view) {
  host_view_observation_.Reset();
  host_ = nullptr;
}

void BraveVerticalTabStripContainerView::UpdateVerticalTabBounds() {
  if (!host_) {
    return;
  }

  const gfx::Rect host_bounds = host_->bounds();
  gfx::Rect strip_bounds = host_bounds;
  strip_bounds.set_width(region_view_->GetPreferredSize().width());

  if (!region_view_->GetVisible() || strip_bounds.IsEmpty()) {
    SetBoundsRect(gfx::Rect());
    return;
  }

  const bool on_right = browser_view_->browser()
                            ->GetFeatures()
                            .vertical_tab_controller()
                            ->IsVerticalTabOnRight();
  if (on_right) {
    strip_bounds.set_x(host_bounds.right() - strip_bounds.width());
  }

  // RTL: when the strip is wider than the host, correct horizontal overflow so
  // the extra width still extends toward the web contents; flip the shift when
  // vertical tabs are on the right.
  if (base::i18n::IsRTL()) {
    auto width_difference = strip_bounds.width() - host_bounds.width();
    width_difference = std::max(width_difference, 0);
    if (on_right) {
      width_difference = -width_difference;
    }
    strip_bounds.set_x(strip_bounds.x() - (width_difference));
  }

  SetBoundsRect(strip_bounds);
}

BEGIN_METADATA(BraveVerticalTabStripContainerView)
END_METADATA
