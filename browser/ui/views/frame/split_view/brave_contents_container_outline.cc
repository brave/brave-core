/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_contents_container_outline.h"

#include "base/check.h"
#include "base/i18n/rtl.h"
#include "base/notreached.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/ui/layout_constants.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPathBuilder.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size_f.h"

// static
BraveContentsContainerOutline* BraveContentsContainerOutline::From(
    ContentsContainerOutline* view) {
  CHECK(view);
  return static_cast<BraveContentsContainerOutline*>(view);
}

// static
int BraveContentsContainerOutline::GetBorderThickness(Style style) {
  switch (style) {
    case Style::kHidden:
      return 0;
    case Style::kStandalone:
      return kRoundedCornersContentsOutlineThickness;
    case Style::kSplitActive:
    case Style::kSplitInactive:
      return BraveContentsContainerView::kBorderThickness;
  }
  NOTREACHED();
}

// static
gfx::RoundedCornersF BraveContentsContainerOutline::GetOuterCornerRadii(
    Style style,
    const gfx::RoundedCornersF& contents_corner_radii) {
  const int thickness = GetBorderThickness(style);
  if (contents_corner_radii.IsEmpty() || !thickness) {
    return contents_corner_radii;
  }

  return {contents_corner_radii.upper_left() + thickness,
          contents_corner_radii.upper_right() + thickness,
          contents_corner_radii.lower_right() + thickness,
          contents_corner_radii.lower_left() + thickness};
}

// static
SkPath BraveContentsContainerOutline::GetPath(
    const gfx::RectF& local_bounds,
    const gfx::RoundedCornersF& corner_radii,
    float mini_toolbar_corner_radius,
    const gfx::SizeF& mini_toolbar_size) {
  const float upper_left = corner_radii.upper_left();
  const float upper_right = corner_radii.upper_right();
  const float lower_right = corner_radii.lower_right();
  const float lower_left = corner_radii.lower_left();

  // Start on the left edge just below the arc, then either close as a rounded
  // rect or detour around the mini toolbar, clockwise throughout.
  SkPathBuilder path;
  path.moveTo(local_bounds.x(), local_bounds.y() + upper_left);
  path.arcTo(SkVector(upper_left, upper_left), 0.0f,
             SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCW,
             SkPoint(local_bounds.x() + upper_left, local_bounds.y()));
  path.lineTo(local_bounds.right() - upper_right, local_bounds.y());
  path.arcTo(SkVector(upper_right, upper_right), 0.0f,
             SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCW,
             SkPoint(local_bounds.right(), local_bounds.y() + upper_right));

  if (mini_toolbar_size.IsEmpty()) {
    path.lineTo(local_bounds.right(), local_bounds.bottom() - lower_right);
    path.arcTo(
        SkVector(lower_right, lower_right), 0.0f, SkPathBuilder::kSmall_ArcSize,
        SkPathDirection::kCW,
        SkPoint(local_bounds.right() - lower_right, local_bounds.bottom()));
  } else {
    path.lineTo(local_bounds.right(),
                local_bounds.bottom() - mini_toolbar_size.height());
    path.arcTo(SkVector(mini_toolbar_corner_radius, mini_toolbar_corner_radius),
               0, SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCW,
               SkPoint(local_bounds.right() - mini_toolbar_corner_radius,
                       local_bounds.bottom() - mini_toolbar_size.height() +
                           mini_toolbar_corner_radius));
    path.lineTo(local_bounds.right() - mini_toolbar_size.width() +
                    mini_toolbar_corner_radius * 2,
                local_bounds.bottom() - mini_toolbar_size.height() +
                    mini_toolbar_corner_radius);
    path.arcTo(SkVector(mini_toolbar_corner_radius, mini_toolbar_corner_radius),
               0, SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCCW,
               SkPoint(local_bounds.right() - mini_toolbar_size.width() +
                           mini_toolbar_corner_radius,
                       local_bounds.bottom() - mini_toolbar_size.height() +
                           mini_toolbar_corner_radius * 2));
    path.lineTo(local_bounds.right() - mini_toolbar_size.width() +
                    mini_toolbar_corner_radius,
                local_bounds.bottom() - mini_toolbar_corner_radius);
    path.arcTo(SkVector(mini_toolbar_corner_radius, mini_toolbar_corner_radius),
               0, SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCW,
               SkPoint(local_bounds.right() - mini_toolbar_size.width(),
                       local_bounds.bottom()));
  }

  path.lineTo(local_bounds.x() + lower_left, local_bounds.bottom());
  path.arcTo(SkVector(lower_left, lower_left), 0.0f,
             SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCW,
             SkPoint(local_bounds.x(), local_bounds.bottom() - lower_left));
  path.close();

  if (base::i18n::IsRTL()) {
    gfx::PointF center = local_bounds.CenterPoint();
    SkMatrix flip;
    flip.setScale(-1, 1, center.x(), center.y());
    path.transform(flip);
  }

  return path.detach();
}

BraveContentsContainerOutline::~BraveContentsContainerOutline() = default;

void BraveContentsContainerOutline::UpdateStyle(
    Style style,
    const gfx::RoundedCornersF& contents_corner_radii) {
  style_ = style;
  contents_corner_radii_ = contents_corner_radii;
  SetVisible(style_ != Style::kHidden);
  SchedulePaint();
}

int BraveContentsContainerOutline::GetStrokeThickness() const {
  switch (style_) {
    case Style::kHidden:
      return 0;
    // kSplitInactive strokes thin but still reserves kSplitActive's inset, so
    // that activating a pane doesn't shift its contents.
    case Style::kStandalone:
    case Style::kSplitInactive:
      return kRoundedCornersContentsOutlineThickness;
    case Style::kSplitActive:
      return BraveContentsContainerView::kBorderThickness;
  }
  NOTREACHED();
}

int BraveContentsContainerOutline::GetStrokeOuterInset() const {
  return GetBorderThickness(style_) - GetStrokeThickness();
}

ui::ColorId BraveContentsContainerOutline::GetStrokeColorId() const {
  switch (style_) {
    case Style::kHidden:
    case Style::kStandalone:
      return kColorBraveContentsOutline;
    case Style::kSplitActive:
      return kColorBraveSplitViewActiveWebViewBorder;
    case Style::kSplitInactive:
      return kColorBraveSplitViewInactiveWebViewBorder;
  }
  NOTREACHED();
}

void BraveContentsContainerOutline::OnPaint(gfx::Canvas* canvas) {
  if (style_ == Style::kHidden) {
    return;
  }

  const float thickness = GetStrokeThickness();
  const int outer_inset = GetStrokeOuterInset();
  // Distance from the container's edge to the stroke's centre line.
  const float stroke_offset = outer_inset + thickness / 2.0f;

  gfx::RectF local_bounds(GetLocalBounds());
  local_bounds.Inset(stroke_offset);

  // Square contents (rounded corners off, or tab fullscreen) give all-zero
  // outer radii, and gfx::RoundedCornersF clamps the negative result back to
  // zero, so the path stays a plain rectangle.
  const auto outer_radii(GetOuterCornerRadii(style_, contents_corner_radii_));
  gfx::RoundedCornersF radii(outer_radii.upper_left() - stroke_offset,
                             outer_radii.upper_right() - stroke_offset,
                             outer_radii.lower_right() - stroke_offset,
                             outer_radii.lower_left() - stroke_offset);
  if (base::i18n::IsRTL()) {
    // GetPath() builds the path in logical order and then mirrors it, but our
    // radii are already physical. Pre-swap so the mirror restores them.
    radii = gfx::RoundedCornersF(radii.upper_right(), radii.upper_left(),
                                 radii.lower_left(), radii.lower_right());
  }

  // An invisible mini toolbar means no notch, just a plain rounded rect.
  gfx::SizeF mini_toolbar_size;
  if (mini_toolbar_ && mini_toolbar_->GetVisible()) {
    mini_toolbar_size = gfx::SizeF(mini_toolbar_->size());
    // The toolbar clips itself at a fixed radius from its own edges, but our
    // stroke runs |outer_inset| further out. Shrink the size GetPath() detours
    // around by the same amount, or the notch lands short of the clip and the
    // web contents show through the gap.
    mini_toolbar_size.Enlarge(-outer_inset, -outer_inset);
  }

  cc::PaintFlags flags;
  flags.setStrokeWidth(thickness);
  flags.setColor(GetColorProvider()->GetColor(GetStrokeColorId()));
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);

  canvas->DrawPath(
      GetPath(local_bounds, radii, kCornerRadius, mini_toolbar_size), flags);
}

BEGIN_METADATA(BraveContentsContainerOutline)
END_METADATA
