/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_VIEW_SHADOW_H_
#define BRAVE_BROWSER_UI_VIEWS_VIEW_SHADOW_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/compositor/layer_owner.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/shadow_value.h"
#include "ui/views/view_observer.h"

// Manages a layer that will render a drop shadow under a given view. When an
// instance is constructed for a view, the view will be set to paint to a layer
// (if not already set) and a shadow layer will be added below. To remove the
// shadow, release the `ViewShadow` instance.
//
// Example:
//
//   class ViewWithShadow : public views::View {
//    private:
//     constexpr int kCornerRadius = 8;
//
//     constexpr ViewShadow::ShadowParameters kShadow{
//         .offset_x = 0,
//         .offset_y = 1,
//         .blur_radius = 4,
//         .shadow_color = SkColorSetA(SK_ColorBLACK, 0.07 * 255)};
//
//     ViewShadow shadow_{this, kCornerRadius, kShadow};
//   };
//
class ViewShadow : public ui::LayerDelegate,
                   public ui::LayerOwner::Observer,
                   public views::ViewObserver {
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

  // Sets the insets for the rectangular shadow shape. This allows the shadow
  // and the associated view to have different dimensions.
  void SetInsets(const gfx::Insets& insets);
  const gfx::Insets& insets() const { return insets_; }

  void SetVisible(bool visible);

 protected:
  ui::Layer* shadow_layer() { return layer_owner_.layer(); }
  const ui::Layer* shadow_layer() const { return layer_owner_.layer(); }

  // views::ViewObserver:
  void OnViewLayerBoundsSet(views::View* view) override;
  void OnViewIsDeleting(views::View* view) override;

  // ui::LayerOwner::Observer,
  void OnLayerRecreated(ui::Layer* old_layer) override;

  // LayerDelegate:
  void OnPaintLayer(const ui::PaintContext& context) override;
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override;

 private:
  void UpdateBounds();

  ui::LayerOwner layer_owner_;
  raw_ptr<views::View> view_ = nullptr;
  int corner_radius_ = 0;
  raw_ref<const gfx::ShadowValues> shadow_values_;
  gfx::Insets insets_;

  base::ScopedObservation<ui::LayerOwner, ui::LayerOwner::Observer>
      layer_owner_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_VIEW_SHADOW_H_
