/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_underline.h"

#include <algorithm>

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/ui/views/tabs/tab_group_views.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/skia_conversions.h"

namespace {
constexpr int kStrokeThicknessForVerticalTabs = 4;
}  // namespace

BraveTabGroupUnderline::BraveTabGroupUnderline(
    TabGroupViews* tab_group_views,
    const tab_groups::TabGroupId& group)
    : TabGroupUnderline(tab_group_views, group) {}

BraveTabGroupUnderline::~BraveTabGroupUnderline() = default;

void BraveTabGroupUnderline::UpdateBounds(views::View* leading_view,
                                          views::View* trailing_view) {
  TabGroupUnderline::UpdateBounds(leading_view, trailing_view);
  if (!ShouldShowVerticalTabs() || !GetVisible())
    return;

  // override bounds for vertical tabs mode.
  gfx::RectF leading_bounds = gfx::RectF(leading_view->bounds());
  ConvertRectToTarget(leading_view->parent(), parent(), &leading_bounds);
  leading_bounds.Inset(gfx::InsetsF(GetInsetsForUnderline(leading_view)));

  gfx::RectF trailing_bounds = gfx::RectF(trailing_view->bounds());
  ConvertRectToTarget(trailing_view->parent(), parent(), &trailing_bounds);
  trailing_bounds.Inset(gfx::InsetsF(GetInsetsForUnderline(trailing_view)));
  if (trailing_bounds.height()) {
    trailing_bounds.set_height(trailing_bounds.height() +
                               BraveTabGroupHeader::kPaddingForGroup);
  }

  gfx::Rect group_bounds = ToEnclosingRect(leading_bounds);
  group_bounds.Union(ToEnclosingRect(trailing_bounds));
  if (group_bounds.height() == 0) {
    SetVisible(false);
    return;
  }

  SetBounds(0, group_bounds.y(), kStrokeThicknessForVerticalTabs,
            group_bounds.height());
}

gfx::Insets BraveTabGroupUnderline::GetInsetsForUnderline(
    views::View* sibling_view) const {
  if (!ShouldShowVerticalTabs())
    return TabGroupUnderline::GetInsetsForUnderline(sibling_view);

  return {};
}

SkPath BraveTabGroupUnderline::GetPath() const {
  if (!ShouldShowVerticalTabs())
    return TabGroupUnderline::GetPath();

  // In vertical tabs, underline is not actually "underline'. It's vertical line
  // at the left side of the tab group. And it has half rounded corners.
  //
  // +   group header   | '+' is the underline.
  // ++  tab 1          | Drawing starts from top-right and goes
  // ++  tab 2          | counter-clockwise
  // +   tab 3          |
  //
  SkPath path;
  path.arcTo(/* rx = */ kStrokeThicknessForVerticalTabs,
             /* ry = */ kStrokeThicknessForVerticalTabs,
             /* angle = */ 180.f, SkPath::kSmall_ArcSize, SkPathDirection::kCW,
             /* x = */ kStrokeThicknessForVerticalTabs,
             /* y = */ kStrokeThicknessForVerticalTabs);
  path.lineTo(kStrokeThicknessForVerticalTabs,
              height() - kStrokeThicknessForVerticalTabs);
  path.arcTo(/* rx = */ kStrokeThicknessForVerticalTabs,
             /* ry = */ kStrokeThicknessForVerticalTabs,
             /* angle = */ 180.f, SkPath::kSmall_ArcSize, SkPathDirection::kCW,
             /* x = */ 0,
             /* y = */ height());
  path.close();

  return path;
}

void BraveTabGroupUnderline::OnPaint(gfx::Canvas* canvas) {
  if (!ShouldShowVerticalTabs()) {
    TabGroupUnderline::OnPaint(canvas);
    return;
  }

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(tab_group_views_->GetGroupColor());
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawPath(GetPath(), flags);
}

bool BraveTabGroupUnderline::ShouldShowVerticalTabs() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return false;

  return tabs::features::ShouldShowVerticalTabs(tab_group_views_->GetBrowser());
}
