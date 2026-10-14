// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/widget/gesture_view.h"

#include "brave/components/gestures/core/common/smoothed_path_util.h"
#include "cc/paint/paint_flags.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/point_conversions.h"

namespace gestures {

namespace {

constexpr SkColor kTrailColor = SkColorSetRGB(0xFF, 0x1F, 0x01);
constexpr uint8_t kTrailAlpha = 180;
constexpr SkColor kLabelColor = SK_ColorWHITE;
constexpr SkColor kLabelBackgroundColor = SkColorSetARGB(200, 32, 32, 32);
constexpr float kTrailStrokeWidth = 6.0F;
constexpr int kLabelHorizontalPadding = 10;
constexpr int kLabelVerticalPadding = 6;
constexpr int kLabelCornerRadius = 6;
constexpr int kLabelOffsetBelowCursor = 24;

}  // namespace

GestureView::GestureView() {
  SetCanProcessEventsWithinSubtree(false);
}

GestureView::~GestureView() = default;

void GestureView::SetTrail(bool show_trail,
                           const GesturePath& path,
                           const std::u16string& label) {
  show_trail_ = show_trail;
  path_ = path;
  label_ = label;
  SchedulePaint();
}

void GestureView::OnPaint(gfx::Canvas* canvas) {
  View::OnPaint(canvas);
  DrawTrail(*canvas);
  DrawLabel(*canvas);
}

void GestureView::DrawTrail(gfx::Canvas& canvas) {
  if (!show_trail_ || path_.size() < 2) {
    return;
  }

  cc::PaintFlags paint_flags;
  paint_flags.setColor(kTrailColor);
  paint_flags.setStrokeWidth(kTrailStrokeWidth);
  paint_flags.setStyle(cc::PaintFlags::kStroke_Style);
  paint_flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  paint_flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  paint_flags.setAntiAlias(true);

  // One path, not per-segment lines, avoids beaded joints. Drawn opaque into
  // its own layer so a self-crossing gesture doesn't double its alpha.
  canvas.SaveLayerAlpha(kTrailAlpha);
  canvas.DrawPath(SmoothedPath(path_), paint_flags);
  canvas.Restore();
}

void GestureView::DrawLabel(gfx::Canvas& canvas) {
  if (path_.empty() || label_.empty()) {
    return;
  }

  const gfx::FontList font_list;
  const int string_width = gfx::Canvas::GetStringWidth(label_, font_list);
  const gfx::Size label_size(string_width + kLabelHorizontalPadding * 2,
                             font_list.GetHeight() + kLabelVerticalPadding * 2);
  // Centered on the cursor's x, offset further below it than the trail's
  // own width so the label doesn't sit directly under the cursor tip.
  const gfx::Point cursor = gfx::ToRoundedPoint(path_.back());
  const gfx::Point label_origin(cursor.x() - label_size.width() / 2,
                                cursor.y() + kLabelOffsetBelowCursor);
  const gfx::Rect label_bounds(label_origin, label_size);

  cc::PaintFlags paint_flags;
  paint_flags.setColor(kLabelBackgroundColor);
  paint_flags.setAntiAlias(true);
  canvas.DrawRoundRect(label_bounds, kLabelCornerRadius, paint_flags);
  canvas.DrawStringRectWithFlags(label_, font_list, kLabelColor, label_bounds,
                                 gfx::Canvas::TEXT_ALIGN_CENTER);
}

BEGIN_METADATA(GestureView)
END_METADATA

}  // namespace gestures
