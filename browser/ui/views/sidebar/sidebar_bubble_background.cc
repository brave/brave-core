/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_bubble_background.h"

#include "brave/browser/ui/views/sidebar/bubble_border_with_arrow.h"
#include "cc/paint/paint_flags.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"
#include "third_party/skia/include/core/SkScalar.h"

SidebarBubbleBackground::SidebarBubbleBackground(BubbleBorderWithArrow* border)
    : border_(border) {}

SidebarBubbleBackground::~SidebarBubbleBackground() = default;

void SidebarBubbleBackground::Paint(
    gfx::Canvas* canvas, views::View* view) const {
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(border_->background_color());

  gfx::RectF bounds(view->GetLocalBounds());
  bounds.Inset(gfx::Insets(border_->GetInsets()));

  // Fill the background.
  canvas->DrawRoundRect(bounds, border_->corner_radius(), flags);

  // Fill the arrow.
  gfx::RectF arrow_bounds = BubbleBorderWithArrow::GetArrowRect(
      bounds, border_->arrow());
  SkPath arrow_path;
  arrow_path.moveTo(SkIntToScalar(arrow_bounds.top_right().x()),
                    SkIntToScalar(arrow_bounds.top_right().y()));
  arrow_path.lineTo(
      SkIntToScalar(arrow_bounds.x()),
      SkIntToScalar(arrow_bounds.y() + arrow_bounds.height() / 2));
  arrow_path.lineTo(SkIntToScalar(arrow_bounds.bottom_right().x()),
                    SkIntToScalar(arrow_bounds.bottom_right().y()));
  arrow_path.lineTo(SkIntToScalar(arrow_bounds.top_right().x()),
                    SkIntToScalar(arrow_bounds.top_right().y()));
  arrow_path.close();
  canvas->DrawPath(arrow_path, flags);
}
