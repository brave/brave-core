/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_contents_container_outline.h"

#include <memory>

#include "base/check.h"
#include "base/i18n/rtl.h"
#include "base/notreached.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/ui/layout_constants.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPathBuilder.h"
#include "third_party/skia/include/pathops/SkPathOps.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/view_utils.h"

// Declared by chromium_src/chrome/browser/ui/views/frame/
// contents_container_view.cc so that upstream's constructor builds our outline.
std::unique_ptr<ContentsContainerOutline> CreateBraveContainerOutline(
    views::View* mini_toolbar) {
  return std::make_unique<BraveContentsContainerOutline>(mini_toolbar);
}

// static
BraveContentsContainerOutline* BraveContentsContainerOutline::From(
    ContentsContainerOutline* view) {
  auto* outline = views::AsViewClass<BraveContentsContainerOutline>(view);
  CHECK(outline);
  return outline;
}

// static
int BraveContentsContainerOutline::GetBorderThickness(Style style) {
  switch (style) {
    case Style::kHidden:
    // The mini toolbar overlays the contents, so outlining it doesn't cost the
    // contents any room.
    case Style::kMiniToolbarOnly:
      return 0;
    case Style::kStandalone:
      return kRoundedCornersContentsOutlineThickness;
    case Style::kSplitActive:
    case Style::kSplitInactive:
      return kSplitBorderThickness;
  }
  NOTREACHED();
}

// static
int BraveContentsContainerOutline::GetStrokeThickness(Style style) {
  switch (style) {
    case Style::kHidden:
      return 0;
    // kSplitInactive strokes thin but still reserves kSplitActive's inset, so
    // that activating a pane doesn't shift its contents.
    case Style::kMiniToolbarOnly:
    case Style::kStandalone:
    case Style::kSplitInactive:
      return kRoundedCornersContentsOutlineThickness;
    case Style::kSplitActive:
      return kSplitBorderThickness;
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

  return path.detach();
}

BraveContentsContainerOutline::~BraveContentsContainerOutline() = default;

void BraveContentsContainerOutline::UpdateStyle(
    Style style,
    const gfx::RoundedCornersF& contents_corner_radii) {
  style_ = style;
  contents_corner_radii_ = contents_corner_radii;
  SetVisible(style_ != Style::kHidden);
  // The clip follows the ring, and the ring's thickness and radii come from the
  // style. Upstream only recomputes it when the toolbar's bounds change.
  SetClipPath();
  SchedulePaint();
}

int BraveContentsContainerOutline::GetStrokeOuterInset() const {
  if (style_ == Style::kMiniToolbarOnly) {
    // No inset was reserved, so stroke from the container's edge inwards, over
    // the contents the mini toolbar already overlays.
    return 0;
  }

  return GetBorderThickness(style_) - GetStrokeThickness(style_);
}

ui::ColorId BraveContentsContainerOutline::GetStrokeColorId() const {
  switch (style_) {
    case Style::kHidden:
    case Style::kMiniToolbarOnly:
    case Style::kStandalone:
      return kColorBraveContentsOutline;
    case Style::kSplitActive:
      return kColorBraveSplitViewActiveWebViewBorder;
    case Style::kSplitInactive:
      return kColorBraveSplitViewInactiveWebViewBorder;
  }
  NOTREACHED();
}

SkPath BraveContentsContainerOutline::BuildRingPath() const {
  // Distance from the container's edge to the stroke's centre line.
  const float stroke_offset =
      GetStrokeOuterInset() + GetStrokeThickness(style_) / 2.0f;

  gfx::RectF local_bounds(GetLocalBounds());
  local_bounds.Inset(stroke_offset);

  // An invisible mini toolbar means no notch, just a plain rounded rect.
  gfx::SizeF mini_toolbar_size;
  if (mini_toolbar_ && mini_toolbar_->GetVisible()) {
    mini_toolbar_size = gfx::SizeF(mini_toolbar_->size());
  }

  // The toolbar is laid out at the logical bottom right, which views mirrors to
  // the physical bottom left in RTL, while we paint unflipped. GetPath() always
  // cuts the notch out of the bottom right, so flip the finished path.
  const bool mirror = !mini_toolbar_size.IsEmpty() && base::i18n::IsRTL();

  // Square contents (rounded corners off, or tab fullscreen) give all-zero
  // outer radii, and gfx::RoundedCornersF clamps the negative result back to
  // zero, so the path stays a plain rectangle.
  const auto outer_radii(GetOuterCornerRadii(style_, contents_corner_radii_));
  gfx::RoundedCornersF radii(outer_radii.upper_left() - stroke_offset,
                             outer_radii.upper_right() - stroke_offset,
                             outer_radii.lower_right() - stroke_offset,
                             outer_radii.lower_left() - stroke_offset);
  if (mirror) {
    // Our radii are physical, so pre-swap them for the mirror to undo.
    radii = gfx::RoundedCornersF(radii.upper_right(), radii.upper_left(),
                                 radii.lower_left(), radii.lower_right());
  }

  const SkPath path =
      GetPath(local_bounds, radii, kCornerRadius, mini_toolbar_size);
  if (!mirror) {
    return path;
  }

  const gfx::PointF center = local_bounds.CenterPoint();
  SkMatrix flip;
  flip.setScale(-1, 1, center.x(), center.y());
  return path.makeTransform(flip);
}

void BraveContentsContainerOutline::OnPaint(gfx::Canvas* canvas) {
  if (style_ == Style::kHidden) {
    return;
  }

  cc::PaintFlags flags;
  flags.setStrokeWidth(GetStrokeThickness(style_));
  flags.setColor(GetColorProvider()->GetColor(GetStrokeColorId()));
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);

  gfx::ScopedCanvas scoped_canvas(canvas);
  if (style_ == Style::kMiniToolbarOnly) {
    // The ring detours around the mini toolbar and otherwise runs along the
    // container's edges, so clipping to the toolbar leaves just the detour.
    canvas->ClipRect(mini_toolbar_->GetMirroredBounds());
  }

  canvas->DrawPath(BuildRingPath(), flags);
}

void BraveContentsContainerOutline::SetClipPath() {
  if (!mini_toolbar_ || !mini_toolbar_->GetVisible() ||
      mini_toolbar_->size().IsEmpty()) {
    return;
  }

  // Keep the toolbar to the part of itself lying outside the ring's centre
  // line. The stroke then straddles the seam, so no sliver of web contents can
  // surface between the toolbar and the ring. Upstream instead derives the
  // clip's notch from the toolbar's own rect, which only lines up with the
  // stroke at upstream's thickness.
  //
  // The ring is in our coordinates, which we paint unflipped, so translating by
  // the toolbar's mirrored origin is what lands it in the toolbar's own space.
  const gfx::Rect toolbar_bounds = mini_toolbar_->GetMirroredBounds();
  const SkPath ring =
      BuildRingPath().makeOffset(-toolbar_bounds.x(), -toolbar_bounds.y());
  const SkPath toolbar =
      SkPath::Rect(gfx::RectToSkRect(mini_toolbar_->GetLocalBounds()));

  SkPath clip;
  if (!Op(toolbar, ring, kDifference_SkPathOp, &clip) || clip.isEmpty()) {
    // The toolbar paints to a layer, where SetClipPath() installs a mask built
    // from the path, so an empty path would hide it entirely rather than clear
    // the clip. Keep the old clip until we are called with usable bounds.
    return;
  }

  mini_toolbar_->SetClipPath(clip);
}

BEGIN_METADATA(BraveContentsContainerOutline)
END_METADATA
