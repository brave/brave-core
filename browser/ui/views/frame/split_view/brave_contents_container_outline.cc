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
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/view_utils.h"

namespace {

// Appends the detour around the mini toolbar, clockwise: |notch| is the
// toolbar's footprint and |path|'s current point must be its top right corner.
// Leaves the current point at its bottom left corner. Always detours the bottom
// right corner; mirroring for RTL is the caller's job.
void AppendMiniToolbarNotch(const gfx::RectF& notch,
                            float corner_radius,
                            SkPathBuilder& path) {
  const SkVector radii(corner_radius, corner_radius);
  path.arcTo(radii, 0, SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCW,
             SkPoint(notch.right() - corner_radius, notch.y() + corner_radius));
  path.lineTo(notch.x() + corner_radius * 2, notch.y() + corner_radius);
  path.arcTo(radii, 0, SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCCW,
             SkPoint(notch.x() + corner_radius, notch.y() + corner_radius * 2));
  path.lineTo(notch.x() + corner_radius, notch.bottom() - corner_radius);
  path.arcTo(radii, 0, SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCW,
             SkPoint(notch.x(), notch.bottom()));
}

}  // namespace

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
    const gfx::RectF& mini_toolbar_notch) {
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

  if (mini_toolbar_notch.IsEmpty()) {
    path.lineTo(local_bounds.right(), local_bounds.bottom() - lower_right);
    path.arcTo(
        SkVector(lower_right, lower_right), 0.0f, SkPathBuilder::kSmall_ArcSize,
        SkPathDirection::kCW,
        SkPoint(local_bounds.right() - lower_right, local_bounds.bottom()));
  } else {
    path.lineTo(mini_toolbar_notch.right(), mini_toolbar_notch.y());
    AppendMiniToolbarNotch(mini_toolbar_notch, mini_toolbar_corner_radius,
                           path);
  }

  path.lineTo(local_bounds.x() + lower_left, local_bounds.bottom());
  path.arcTo(SkVector(lower_left, lower_left), 0.0f,
             SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCW,
             SkPoint(local_bounds.x(), local_bounds.bottom() - lower_left));
  path.close();

  return path.detach();
}

// static
SkPath BraveContentsContainerOutline::MirrorHorizontally(
    const SkPath& path,
    const gfx::RectF& bounds) {
  const gfx::PointF center = bounds.CenterPoint();
  SkMatrix flip;
  flip.setScale(-1, 1, center.x(), center.y());
  return path.makeTransform(flip);
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

float BraveContentsContainerOutline::GetStrokeOffset() const {
  return GetStrokeOuterInset() + GetStrokeThickness(style_) / 2.0f;
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

gfx::RectF BraveContentsContainerOutline::GetMiniToolbarNotch() const {
  // An invisible mini toolbar means no notch, just a plain rounded rect.
  if (!mini_toolbar_ || !mini_toolbar_->GetVisible() ||
      mini_toolbar_->size().IsEmpty()) {
    return gfx::RectF();
  }

  // The ring runs inside the container's edges by the stroke offset, so its
  // detour around the toolbar sits that far above and left of the toolbar
  // itself, meeting the rest of the ring at the container's bottom right.
  gfx::RectF notch(mini_toolbar_->bounds());
  notch.Offset(-GetStrokeOffset(), -GetStrokeOffset());
  return notch;
}

SkPath BraveContentsContainerOutline::BuildRingPath() const {
  const float stroke_offset = GetStrokeOffset();

  gfx::RectF local_bounds(GetLocalBounds());
  local_bounds.Inset(stroke_offset);

  const gfx::RectF notch = GetMiniToolbarNotch();

  // Views mirrors the toolbar's logical bottom right to the physical bottom
  // left in RTL, while we paint unflipped. GetPath() always cuts the notch out
  // of the bottom right, so flip the finished path.
  const bool mirror = !notch.IsEmpty() && base::i18n::IsRTL();

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

  const SkPath path = GetPath(local_bounds, radii, kCornerRadius, notch);
  return mirror ? MirrorHorizontally(path, local_bounds) : path;
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
  gfx::RectF notch = GetMiniToolbarNotch();
  if (notch.IsEmpty()) {
    // The toolbar paints to a layer, where SetClipPath() installs a mask built
    // from the path, so an empty path would hide it entirely rather than clear
    // the clip. Keep the old clip until we are called with usable bounds.
    return;
  }

  // The clip is in the toolbar's coordinates. Upstream builds the notch by
  // insetting the toolbar's own rect, which only lines up with their ring at
  // their own thickness, so we take it from the ring instead.
  const gfx::RectF local_bounds(mini_toolbar_->GetLocalBounds());
  notch.Offset(-mini_toolbar_->x(), -mini_toolbar_->y());

  // Keep the toolbar's side of the detour, closed along its own right and
  // bottom edges. The stroke then straddles the seam, so no sliver of web
  // contents can surface between the toolbar and the ring.
  SkPathBuilder clip;
  clip.moveTo(notch.right(), notch.y());
  AppendMiniToolbarNotch(notch, kCornerRadius, clip);
  clip.lineTo(notch.x(), local_bounds.bottom());
  clip.lineTo(local_bounds.right(), local_bounds.bottom());
  clip.lineTo(local_bounds.right(), notch.y());
  clip.close();

  // Views mirrors the toolbar's bounds in RTL but not what it paints, so the
  // detour has to be flipped to the toolbar's other side.
  SkPath path = clip.detach();
  if (base::i18n::IsRTL()) {
    path = MirrorHorizontally(path, local_bounds);
  }

  mini_toolbar_->SetClipPath(path);
}

BEGIN_METADATA(BraveContentsContainerOutline)
END_METADATA
