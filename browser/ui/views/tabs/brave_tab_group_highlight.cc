/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_highlight.h"

#include "base/check.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/tabs/tab_group_views.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/view.h"

BraveTabGroupHighlight::~BraveTabGroupHighlight() = default;

void BraveTabGroupHighlight::ReparentLayerForUnpinnedScroll(
    ui::Layer* scroll_layer) {
  DCHECK(layer());
  MoveLayerToParent(
      scroll_layer, views::View::LayerOffsetData(layer()->device_scale_factor()));
  MoveLayerToParent(layer(),
                    views::View::LayerOffsetData(layer()->device_scale_factor()));
}

SkPath BraveTabGroupHighlight::GetPath() const {
  if (!tabs::utils::ShouldShowBraveVerticalTabs(
          tab_group_views_->GetBrowserWindowInterface()) &&
      !tabs::HorizontalTabsUpdateEnabled()) {
    return TabGroupHighlight::GetPath();
  }

  return {};
}

BEGIN_METADATA(BraveTabGroupHighlight)
END_METADATA
