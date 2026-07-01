/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/view_outline.h"

#include <array>
#include <memory>

#include "base/check.h"
#include "base/check_op.h"
#include "cc/paint/paint_flags.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPathBuilder.h"
#include "third_party/skia/include/core/SkPathTypes.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/view.h"

ViewOutline::ViewOutline(views::View* view,
                         const gfx::RoundedCornersF& corner_radii,
                         ui::ColorId color_id,
                         int outline_width)
    : layer_owner_(std::make_unique<ui::Layer>()),
      view_(view),
      corner_radii_(corner_radii),
      color_id_(color_id),
      outline_width_(outline_width) {
  DCHECK(view_);
  if (!view_->layer()) {
    view_->SetPaintToLayer();
  }

  outline_layer()->set_delegate(this);
  outline_layer()->SetFillsBoundsOpaquely(false);

  view_->AddLayerToRegion(outline_layer(), views::LayerRegion::kBelow);
  view_->AddObserver(this);
  layer_owner_observation_.Observe(&layer_owner_);

  OnViewLayerBoundsSet(view_);
}

ViewOutline::~ViewOutline() {
  if (view_) {
    OnViewIsDeleting(view_);
  }
}

void ViewOutline::SetCornerRadii(const gfx::RoundedCornersF& corner_radii) {
  if (corner_radii_ == corner_radii) {
    return;
  }
  corner_radii_ = corner_radii;
  outline_layer()->SchedulePaint(gfx::Rect(outline_layer()->size()));
}

void ViewOutline::SetInsets(const gfx::Insets& insets) {
  insets_ = insets;
  UpdateBounds();
}

void ViewOutline::SetVisible(bool visible) {
  outline_layer()->SetVisible(visible);
}

void ViewOutline::OnViewLayerBoundsSet(views::View* view) {
  DCHECK(view->layer());
  DCHECK_EQ(view, view_.get());
  UpdateBounds();
}

void ViewOutline::OnViewThemeChanged(views::View* view) {
  outline_layer()->SchedulePaint(gfx::Rect(outline_layer()->size()));
}

void ViewOutline::OnViewIsDeleting(views::View* view) {
  layer_owner_observation_.Reset();
  view_->RemoveObserver(this);
  view_ = nullptr;
}

void ViewOutline::OnLayerRecreated(ui::Layer* old_layer) {
  if (!view_) {
    return;
  }

  // During the window closing, the outline layer seems destroyed first
  // before |view_| is destroying in some situation.
  // Crash happens when view tree layouts w/o removing it from layer tree.
  view_->RemoveLayerFromRegionsKeepInLayerTree(old_layer);
  layer_owner_observation_.Reset();
}

void ViewOutline::OnPaintLayer(const ui::PaintContext& context) {
  if (!view_) {
    return;
  }

  const ui::ColorProvider* color_provider = view_->GetColorProvider();
  if (!color_provider) {
    return;
  }

  ui::PaintRecorder recorder(context, outline_layer()->size());
  gfx::Canvas* canvas = recorder.canvas();

  // Clear out the canvas so that transparency can be applied properly.
  canvas->DrawColor(SK_ColorTRANSPARENT);

  cc::PaintFlags flags;
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  flags.setColor(color_provider->GetColor(color_id_));

  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();

  // The layer is expanded by `outline_width_` on every side (see
  // UpdateBounds()), so the full layer rect is the outer edge of the ring and
  // the view sits inset by `outline_width_`.
  const gfx::Rect outer_bounds =
      gfx::ScaleToEnclosingRect(gfx::Rect(outline_layer()->size()), scale);
  const float width = outline_width_ * scale;

  gfx::RectF outer_rect_f(outer_bounds);
  gfx::RectF inner_rect_f(outer_bounds);
  inner_rect_f.Inset(width);

  // The outer corners are the view's corners grown by the outline width so the
  // ring stays concentric and keeps a uniform thickness.
  const std::array<SkVector, 4> outer_radii = {
      {{corner_radii_.upper_left() * scale + width,
        corner_radii_.upper_left() * scale + width},
       {corner_radii_.upper_right() * scale + width,
        corner_radii_.upper_right() * scale + width},
       {corner_radii_.lower_right() * scale + width,
        corner_radii_.lower_right() * scale + width},
       {corner_radii_.lower_left() * scale + width,
        corner_radii_.lower_left() * scale + width}}};

  const std::array<SkVector, 4> inner_radii = {
      {{corner_radii_.upper_left() * scale, corner_radii_.upper_left() * scale},
       {corner_radii_.upper_right() * scale,
        corner_radii_.upper_right() * scale},
       {corner_radii_.lower_right() * scale,
        corner_radii_.lower_right() * scale},
       {corner_radii_.lower_left() * scale,
        corner_radii_.lower_left() * scale}}};

  // The ring is the area between the outer and inner rounded rects. Use an
  // even-odd fill so the inner rect is subtracted, leaving a uniform outline.
  SkPathBuilder builder;
  builder.setFillType(SkPathFillType::kEvenOdd);
  builder.addRRect(SkRRect::MakeRectRadii(gfx::RectFToSkRect(outer_rect_f),
                                          outer_radii.data()));
  builder.addRRect(SkRRect::MakeRectRadii(gfx::RectFToSkRect(inner_rect_f),
                                          inner_radii.data()));

  canvas->DrawPath(builder.detach(), flags);
}

void ViewOutline::OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                             float new_device_scale_factor) {}

void ViewOutline::UpdateBounds() {
  if (!view_) {
    return;
  }

  // Expand the bounds of the specified view outward by the outline width so the
  // ring is drawn in the margin outside the view's rounded rect.
  gfx::Rect outline_bounds = view_->layer()->bounds();
  outline_bounds.Inset(insets_ - gfx::Insets(outline_width_));
  outline_layer()->SetBounds(outline_bounds);
}
