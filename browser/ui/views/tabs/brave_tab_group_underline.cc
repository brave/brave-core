/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_underline.h"

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/views/tabs/tab_group_views.h"

BraveTabGroupUnderline::~BraveTabGroupUnderline() = default;

void BraveTabGroupUnderline::UpdateBounds(views::View* leading_view,
                                          views::View* trailing_view) {
  TabGroupUnderline::UpdateBounds(leading_view, trailing_view);
  if (!tabs::features::ShouldShowVerticalTabs(
          tab_group_views_->controller().GetBrowser()) ||
      !GetVisible()) {
    return;
  }

  // override bounds for vertical tabs mode.
  gfx::RectF leading_bounds = gfx::RectF(leading_view->bounds());
  ConvertRectToTarget(leading_view->parent(), parent(), &leading_bounds);
  leading_bounds.Inset(gfx::InsetsF(GetInsetsForUnderline(leading_view)));

  gfx::RectF trailing_bounds = gfx::RectF(trailing_view->bounds());
  ConvertRectToTarget(trailing_view->parent(), parent(), &trailing_bounds);
  trailing_bounds.Inset(gfx::InsetsF(GetInsetsForUnderline(trailing_view)));

  gfx::Rect group_bounds = ToEnclosingRect(leading_bounds);
  group_bounds.UnionEvenIfEmpty(ToEnclosingRect(trailing_bounds));
  if (group_bounds.height() == 0) {
    SetVisible(false);
    return;
  }

  SetBounds(0, group_bounds.y(), kStrokeThickness, group_bounds.height());
}

gfx::Insets BraveTabGroupUnderline::GetInsetsForUnderline(
    views::View* sibling_view) const {
  if (!tabs::features::ShouldShowVerticalTabs(
          tab_group_views_->controller().GetBrowser()))
    return TabGroupUnderline::GetInsetsForUnderline(sibling_view);

  constexpr int kStrokeInsetForVerticalTabs = 4;
  return gfx::Insets::VH(kStrokeInsetForVerticalTabs, 0);
}

SkPath BraveTabGroupUnderline::GetPath() const {
  if (!tabs::features::ShouldShowVerticalTabs(
          tab_group_views_->controller().GetBrowser()))
    return TabGroupUnderline::GetPath();

  // In vertical tabs, underline is not actually "underline'. It's vertical line
  // at the left side of the tab group. And it has half rounded corners.
  //
  //  +  group header   | '+' is the underline.
  // ++  tab 1          | Drawing starts from top-right and goes
  // ++  tab 2          | counter-clockwise
  //  +  tab 3          |
  //
  SkPath path;
  path.moveTo(kStrokeThickness, 0);
  path.arcTo(/* rx = */ kStrokeThickness,
             /* ry = */ kStrokeThickness,
             /* angle = */ 180.f, SkPath::kSmall_ArcSize, SkPathDirection::kCCW,
             /* x = */ 0,
             /* y = */ kStrokeThickness);
  path.lineTo(0, height() - kStrokeThickness);
  path.arcTo(/* rx = */ kStrokeThickness,
             /* ry = */ kStrokeThickness,
             /* angle = */ 180.f, SkPath::kSmall_ArcSize, SkPathDirection::kCCW,
             /* x = */ kStrokeThickness,
             /* y = */ height());
  path.close();

  return path;
}
