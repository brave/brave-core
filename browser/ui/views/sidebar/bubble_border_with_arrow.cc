/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/bubble_border_with_arrow.h"

#include "base/logging.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/views/view.h"

// static
gfx::RectF BubbleBorderWithArrow::GetArrowRect(
    const gfx::RectF& contents_bounds,
    views::BubbleBorder::Arrow arrow) {
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

BubbleBorderWithArrow::BubbleBorderWithArrow(Arrow arrow,
                                             Shadow shadow,
                                             SkColor color)
    : BubbleBorder(arrow, shadow, color) {
  set_md_shadow_elevation(ChromeLayoutProvider::Get()->GetShadowElevationMetric(
      views::Emphasis::kHigh));
}

BubbleBorderWithArrow::~BubbleBorderWithArrow() = default;

void BubbleBorderWithArrow::Paint(const views::View& view,
                                  gfx::Canvas* canvas) {
  BubbleBorder::Paint(view, canvas);

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(background_color());

  gfx::RectF bounds(view.GetLocalBounds());
  bounds.Inset(GetInsets() + gfx::Insets(0, kBubbleArrowBoundsWidth, 0, 0));
  gfx::RectF arrow_bounds = GetArrowRect(bounds, arrow());

  // Fill arrow bg.
  SkPath arrow_bg_path;
  arrow_bg_path.moveTo(SkIntToScalar(arrow_bounds.top_right().x()),
                       SkIntToScalar(arrow_bounds.top_right().y()));
  arrow_bg_path.lineTo(
      SkIntToScalar(arrow_bounds.x()),
      SkIntToScalar(arrow_bounds.y() + arrow_bounds.height() / 2));
  arrow_bg_path.lineTo(SkIntToScalar(arrow_bounds.bottom_right().x()),
                       SkIntToScalar(arrow_bounds.bottom_right().y()));
  arrow_bg_path.lineTo(SkIntToScalar(arrow_bounds.top_right().x()),
                       SkIntToScalar(arrow_bounds.top_right().y()));
  arrow_bg_path.close();
  canvas->DrawPath(arrow_bg_path, flags);

  // Platform window will draw border & shadow.
  if (shadow() == NO_SHADOW)
    return;

  // Draw the arrow border.
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(kBorderThicknessDip);
  flags.setColor(SkColorSetARGB(0x2E, 0x63, 0x69, 0x6E));
  canvas->DrawLine(arrow_bounds.top_right(),
                   gfx::PointF(arrow_bounds.x(),
                               arrow_bounds.y() + arrow_bounds.height() / 2),
                   flags);
  canvas->DrawLine(arrow_bounds.bottom_right(),
                   gfx::PointF(arrow_bounds.x(),
                               arrow_bounds.y() + arrow_bounds.height() / 2),
                   flags);
}

SkRRect BubbleBorderWithArrow::GetClientRect(const views::View& view) const {
  gfx::RectF bounds(view.GetLocalBounds());
  bounds.Inset(GetInsets() + gfx::Insets(0, kBubbleArrowBoundsWidth, 0, 0));
  return SkRRect::MakeRectXY(gfx::RectFToSkRect(bounds), corner_radius(),
                             corner_radius());
}
