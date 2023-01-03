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

BraveTabGroupUnderline::BraveTabGroupUnderline(
    TabGroupViews* tab_group_views,
    const tab_groups::TabGroupId& group)
    : TabGroupUnderline(tab_group_views, group) {
  SetEnabled(false);
}

BraveTabGroupUnderline::~BraveTabGroupUnderline() = default;

void BraveTabGroupUnderline::UpdateBounds(views::View* leading_view,
                                          views::View* trailing_view) {
  TabGroupUnderline::UpdateBounds(leading_view, trailing_view);
  if (!tabs::features::ShouldShowVerticalTabs(tab_group_views_->GetBrowser()) ||
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

  SetBounds(0, group_bounds.y(),
            std::max(leading_view->width(), trailing_view->width()),
            group_bounds.height());
}

gfx::Insets BraveTabGroupUnderline::GetInsetsForUnderline(
    views::View* sibling_view) const {
  if (!tabs::features::ShouldShowVerticalTabs(tab_group_views_->GetBrowser()))
    return TabGroupUnderline::GetInsetsForUnderline(sibling_view);

  return {};
}

SkPath BraveTabGroupUnderline::GetPath() const {
  if (!tabs::features::ShouldShowVerticalTabs(tab_group_views_->GetBrowser()))
    return TabGroupUnderline::GetPath();

  constexpr SkScalar kRadius = 4;
  auto rect = GetContentsBounds();
  rect.Inset(gfx::Insets().set_left_right(kRadius, kRadius));

  SkPath path;
  path.addRoundRect(gfx::RectToSkRect(rect), kRadius, kRadius);

  return path;
}

void BraveTabGroupUnderline::OnPaint(gfx::Canvas* canvas) {
  if (!tabs::features::ShouldShowVerticalTabs(tab_group_views_->GetBrowser())) {
    TabGroupUnderline::OnPaint(canvas);
    return;
  }

  cc::PaintFlags flags;
  flags.setAntiAlias(true);

  SkColor color = tab_group_views_->GetGroupColor();
  color = color_utils::HSLShift(
      color,
      {.h = -1 /*unchanged*/,
       .s = 0.5 /*unchanged*/,
       .l = GetNativeTheme()->ShouldUseDarkColors() ? 0.7 : 0.8 /*lighter*/});
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawPath(GetPath(), flags);
}
