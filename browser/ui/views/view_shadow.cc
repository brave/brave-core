/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/view_shadow.h"

#include <array>
#include <map>
#include <memory>
#include <tuple>

#include "base/check.h"
#include "base/check_op.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/views/view.h"

namespace {

const gfx::ShadowValues& GetCachedShadowValues(
    const ViewShadow::ShadowParameters& params) {
  using ShadowKey = std::tuple<int, int, int, SkColor>;
  static base::NoDestructor<std::map<ShadowKey, gfx::ShadowValues>> map;

  ShadowKey key(params.offset_x, params.offset_y, params.blur_radius,
                params.shadow_color);

  if (auto iter = map->find(key); iter != map->end()) {
    return iter->second;
  }

  // In Skia, the blur value refers to the blur distance both inside and
  // outside of the rectangle. Double the `blur_radius` parameter to convert
  // the CSS-style blur value to the Skia blur value.
  double blur = params.blur_radius * 2;
  gfx::ShadowValue shadow_value({params.offset_x, params.offset_y}, blur,
                                params.shadow_color);

  auto result = map->emplace(key, gfx::ShadowValues{shadow_value});
  return result.first->second;
}

}  // namespace

ViewShadow::ViewShadow(views::View* view,
                       const gfx::RoundedCornersF& corner_radii,
                       const ShadowParameters& params)
    : layer_owner_(std::make_unique<ui::Layer>()),
      view_(view),
      corner_radii_(corner_radii),
      shadow_values_(GetCachedShadowValues(params)) {
  DCHECK(view_);
  if (!view_->layer()) {
    view_->SetPaintToLayer();
  }

  shadow_layer()->set_delegate(this);
  shadow_layer()->SetFillsBoundsOpaquely(false);

  view_->AddLayerToRegion(shadow_layer(), views::LayerRegion::kBelow);
  view_->AddObserver(this);
  layer_owner_observation_.Observe(&layer_owner_);

  OnViewLayerBoundsSet(view_);
}

ViewShadow::~ViewShadow() {
  if (view_) {
    OnViewIsDeleting(view_);
  }
}

void ViewShadow::SetCornerRadii(const gfx::RoundedCornersF& corner_radii) {
  corner_radii_ = corner_radii;
  shadow_layer()->SchedulePaint(gfx::Rect(shadow_layer()->size()));
}

void ViewShadow::SetInsets(const gfx::Insets& insets) {
  insets_ = insets;
  UpdateBounds();
}

void ViewShadow::SetVisible(bool visible) {
  shadow_layer()->SetVisible(visible);
}

void ViewShadow::OnViewLayerBoundsSet(views::View* view) {
  DCHECK(view->layer());
  DCHECK_EQ(view, view_.get());
  UpdateBounds();
}

void ViewShadow::OnViewIsDeleting(views::View* view) {
  layer_owner_observation_.Reset();
  view_->RemoveObserver(this);
  view_ = nullptr;
}

void ViewShadow::OnLayerRecreated(ui::Layer* old_layer) {
  if (!view_) {
    return;
  }

  // During the window closing, shadow layer seems destroyed first
  // before |view_| is destroying in some situation.
  // Crash happens when view tree layouts w/o removing it from layer tree.
  view_->RemoveLayerFromRegionsKeepInLayerTree(old_layer);
  layer_owner_observation_.Reset();
}

void ViewShadow::OnPaintLayer(const ui::PaintContext& context) {
  ui::PaintRecorder recorder(context, shadow_layer()->size());

  // Clear out the canvas so that transparency can be applied properly.
  recorder.canvas()->DrawColor(SK_ColorTRANSPARENT);

  cc::PaintFlags flags;
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  flags.setColor(SK_ColorTRANSPARENT);
  flags.setLooper(gfx::CreateShadowDrawLooper(*shadow_values_));

  gfx::Rect shadow_bounds(shadow_layer()->size());
  shadow_bounds.Inset(gfx::ShadowValue::GetBlurRegion(*shadow_values_));

  gfx::Canvas* canvas = recorder.canvas();
  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();

  const gfx::Rect scaled_bounds =
      gfx::ScaleToEnclosedRect(shadow_bounds, scale);
  gfx::RectF rect_f(scaled_bounds);

  const std::array<SkVector, 4> scaled_radii = {
      {{corner_radii_.upper_left() * scale, corner_radii_.upper_left() * scale},
       {corner_radii_.upper_right() * scale,
        corner_radii_.upper_right() * scale},
       {corner_radii_.lower_right() * scale,
        corner_radii_.lower_right() * scale},
       {corner_radii_.lower_left() * scale,
        corner_radii_.lower_left() * scale}}};

  const SkPath path = SkPath::RRect(
      SkRRect::MakeRectRadii(gfx::RectFToSkRect(rect_f), scaled_radii.data()));
  canvas->DrawPath(path, flags);
}

void ViewShadow::OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                            float new_device_scale_factor) {}

void ViewShadow::UpdateBounds() {
  if (!view_) {
    return;
  }

  // Expand the bounds of the specified view by the blur region.
  gfx::Rect shadow_bounds = view_->layer()->bounds();
  shadow_bounds.Inset(insets_ -
                      gfx::ShadowValue::GetBlurRegion(*shadow_values_));
  shadow_layer()->SetBounds(shadow_bounds);
}
