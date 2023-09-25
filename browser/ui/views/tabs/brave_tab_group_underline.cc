/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_underline.h"

#include <algorithm>

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/ui/views/tabs/tab_group_style.h"
#include "chrome/browser/ui/views/tabs/tab_group_views.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/skia_conversions.h"

BraveTabGroupUnderline::BraveTabGroupUnderline(
    TabGroupViews* tab_group_views,
    const tab_groups::TabGroupId& group,
    const TabGroupStyle& style)
    : TabGroupUnderline(tab_group_views, group, style) {}

BraveTabGroupUnderline::~BraveTabGroupUnderline() = default;

void BraveTabGroupUnderline::UpdateBounds(const views::View* leading_view,
                                          const views::View* trailing_view) {
  TabGroupUnderline::UpdateBounds(leading_view, trailing_view);
  if (!ShouldShowVerticalTabs() || !GetVisible())
    return;

  const gfx::Rect tab_group_underline_bounds =
      CalculateTabGroupUnderlineBounds(this, leading_view, trailing_view);

  if (tab_group_underline_bounds.height() == 0) {
    SetVisible(false);
    return;
  }

  SetBounds(0, tab_group_underline_bounds.y(),
            TabGroupStyle::kStrokeThicknessForVerticalTabs,
            tab_group_underline_bounds.height());
}

gfx::Insets BraveTabGroupUnderline::GetInsetsForUnderline(
    const views::View* sibling_view) const {
  if (!ShouldShowVerticalTabs())
    return TabGroupUnderline::GetInsetsForUnderline(sibling_view);

  return {};
}

gfx::Rect BraveTabGroupUnderline::CalculateTabGroupUnderlineBounds(
    const views::View* const underline_view,
    const views::View* const leading_view,
    const views::View* const trailing_view) const {
  if (!ShouldShowVerticalTabs()) {
    return TabGroupUnderline::CalculateTabGroupUnderlineBounds(
        underline_view, leading_view, trailing_view);
  }

  // override bounds for vertical tabs mode.
  gfx::RectF leading_bounds = gfx::RectF(leading_view->bounds());
  ConvertRectToTarget(leading_view->parent(), underline_view->parent(),
                      &leading_bounds);
  leading_bounds.Inset(gfx::InsetsF(GetInsetsForUnderline(leading_view)));

  gfx::RectF trailing_bounds = gfx::RectF(trailing_view->bounds());
  ConvertRectToTarget(trailing_view->parent(), underline_view->parent(),
                      &trailing_bounds);
  trailing_bounds.Inset(gfx::InsetsF(GetInsetsForUnderline(trailing_view)));
  if (trailing_bounds.height()) {
    trailing_bounds.set_height(trailing_bounds.height() +
                               BraveTabGroupHeader::kPaddingForGroup);
  }

  gfx::Rect group_bounds = ToEnclosingRect(leading_bounds);
  group_bounds.Union(ToEnclosingRect(trailing_bounds));
  return group_bounds;
}

bool BraveTabGroupUnderline::ShouldShowVerticalTabs() const {
  return tabs::utils::ShouldShowVerticalTabs(tab_group_views_->GetBrowser());
}
