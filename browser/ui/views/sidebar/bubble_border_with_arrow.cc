/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/bubble_border_with_arrow.h"

#include "base/logging.h"
#include "cc/paint/paint_flags.h"
#include "ui/gfx/canvas.h"
#include "ui/views/view.h"

// static
gfx::RectF BubbleBorderWithArrow::GetArrowRect(
    const gfx::RectF& contents_bounds, views::BubbleBorder::Arrow arrow) {
  // sidebar bubble border only support these two arrow types now.
  DCHECK(arrow == views::BubbleBorder::LEFT_TOP ||
         arrow == views::BubbleBorder::LEFT_CENTER);

  gfx::RectF arrow_rect(
      gfx::SizeF(kBubbleArrowBoundsWidth, kBubbleArrowBoundsHeight));
  if (arrow == views::BubbleBorder::LEFT_TOP) {
    constexpr int kBubbleArrowOffsetFromTop = 11;
    arrow_rect.set_origin(
        gfx::PointF(contents_bounds.x() - kBubbleArrowBoundsWidth,
                    contents_bounds.y() + kBubbleArrowOffsetFromTop));
  } else {
    arrow_rect.set_origin(gfx::PointF(
        contents_bounds.x() - kBubbleArrowBoundsWidth,
        contents_bounds.y() +
            (contents_bounds.height() - kBubbleArrowBoundsHeight) / 2));
  }
  return arrow_rect;
}

BubbleBorderWithArrow::~BubbleBorderWithArrow() = default;

gfx::Rect BubbleBorderWithArrow::GetBounds(
    const gfx::Rect& anchor_rect, const gfx::Size& contents_size) const {
  gfx::Rect bounds = BubbleBorder::GetBounds(anchor_rect, contents_size);
  bounds.set_x(bounds.x() + kBubbleArrowBoundsWidth);
  return bounds;
}

void BubbleBorderWithArrow::Paint(
    const views::View& view, gfx::Canvas* canvas) {
  DCHECK(shadow() == views::BubbleBorder::NO_SHADOW);
  gfx::RectF bounds(view.GetLocalBounds());
  bounds.Inset(GetInsets());
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(kBorderThicknessDip);
  SkColor kBorderColor = view.GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_BubbleBorder);
  flags.setColor(kBorderColor);
  canvas->DrawRoundRect(bounds, corner_radius(), flags);

  // Draw the arrow border.
  gfx::RectF arrow_bounds = GetArrowRect(bounds, arrow());

  // Erase existing vertical border that meets arrow.
  // TODO(simonhong): Check why vertical line is thicker than configs.
  flags.setStrokeWidth(kBorderThicknessDip * 2);
  flags.setColor(background_color());
  canvas->DrawLine(arrow_bounds.top_right(),
                   arrow_bounds.bottom_right(),
                   flags);

  flags.setColor(kBorderColor);
  flags.setStrokeWidth(kBorderThicknessDip);
  canvas->DrawLine(arrow_bounds.top_right(),
                   gfx::PointF(arrow_bounds.x(),
                               arrow_bounds.y() + arrow_bounds.height() / 2),
                   flags);
  canvas->DrawLine(arrow_bounds.bottom_right(),
                   gfx::PointF(arrow_bounds.x(),
                               arrow_bounds.y() + arrow_bounds.height() / 2),
                   flags);
}
