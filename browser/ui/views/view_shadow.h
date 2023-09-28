/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_VIEW_SHADOW_H_
#define BRAVE_BROWSER_UI_VIEWS_VIEW_SHADOW_H_

#include "base/memory/raw_ptr.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/gfx/shadow_value.h"
#include "ui/views/view_observer.h"

// Manages a shadow layer that will render a drop shadow under a given view.
class ViewShadow : public views::ViewObserver, public ui::LayerDelegate {
 public:
  struct ShadowParameters {
    int offset_x;
    int offset_y;
    int blur_radius;
    SkColor shadow_color;
  };

  ViewShadow(views::View* view,
             int corner_radius,
             const ShadowParameters& params);

  ViewShadow(const ViewShadow&) = delete;
  ViewShadow& operator=(const ViewShadow&) = delete;

  ~ViewShadow() override;

  void SetInsets(const gfx::Insets& insets);
  const gfx::Insets& insets() const { return insets_; }

 protected:
  // views::ViewObserver:
  void OnViewLayerBoundsSet(views::View* view) override;
  void OnViewIsDeleting(views::View* view) override;

  // LayerDelegate:
  void OnPaintLayer(const ui::PaintContext& context) override;
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override;

 private:
  void UpdateBounds();

  raw_ptr<views::View> view_ = nullptr;
  int corner_radius_ = 0;
  gfx::ShadowValues shadow_values_;
  gfx::Insets insets_;
  ui::Layer layer_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_VIEW_SHADOW_H_
