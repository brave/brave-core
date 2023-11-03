/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_highlight.h"

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/views/tabs/tab_group_views.h"
#include "ui/gfx/geometry/skia_conversions.h"

BraveTabGroupHighlight::~BraveTabGroupHighlight() = default;

SkPath BraveTabGroupHighlight::GetPath() const {
  // We don't have to paint a highlight for vertical tabs.
  if (tabs::utils::ShouldShowVerticalTabs(tab_group_views_->GetBrowser())) {
    return {};
  }

  if (!tabs::features::HorizontalTabsUpdateEnabled()) {
    return TabGroupHighlight::GetPath();
  }

  // Draw a rounded rect that encloses the header and all tabs within the
  // group.
  gfx::Rect shape_rect(0, 0, width(), height());
  shape_rect.Inset(gfx::Insets::VH(brave_tabs::kHorizontalTabVerticalSpacing,
                                   brave_tabs::kHorizontalTabInset));
  float radius = brave_tabs::kTabBorderRadius;

  SkPath path;
  path.addRoundRect(gfx::RectToSkRect(shape_rect), radius, radius);
  return path;
}
