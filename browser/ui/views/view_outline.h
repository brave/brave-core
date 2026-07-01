/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_VIEW_OUTLINE_H_
#define BRAVE_BROWSER_UI_VIEWS_VIEW_OUTLINE_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/color/color_id.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/compositor/layer_owner.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/view_observer.h"

// Manages a layer that renders a crisp outline around a given view, following
// its rounded corners. When an instance is constructed for a view, the view is
// set to paint to a layer (if not already) and an outline layer is added just
// below it. The outline is painted as a ring in the transparent margin outside
// the view's rounded rect, so it is not occluded by the (opaque) view on top.
// The color is resolved from the view's ColorProvider at paint time, so it is
// automatically theme (light/dark) aware. To remove the outline, release the
// `ViewOutline` instance.
class ViewOutline : public ui::LayerDelegate,
                    public ui::LayerOwner::Observer,
                    public views::ViewObserver {
 public:
  ViewOutline(views::View* view,
              const gfx::RoundedCornersF& corner_radii,
              ui::ColorId color_id,
              int outline_width);

  ViewOutline(const ViewOutline&) = delete;
  ViewOutline& operator=(const ViewOutline&) = delete;

  ~ViewOutline() override;

  void SetCornerRadii(const gfx::RoundedCornersF& corner_radii);

  // Sets the insets for the rectangular outline shape. This allows the outline
  // and the associated view to have different dimensions.
  void SetInsets(const gfx::Insets& insets);
  const gfx::Insets& insets() const { return insets_; }

  void SetVisible(bool visible);

 protected:
  ui::Layer* outline_layer() { return layer_owner_.layer(); }
  const ui::Layer* outline_layer() const { return layer_owner_.layer(); }

  // views::ViewObserver:
  void OnViewLayerBoundsSet(views::View* view) override;
  void OnViewThemeChanged(views::View* view) override;
  void OnViewIsDeleting(views::View* view) override;

  // ui::LayerOwner::Observer:
  void OnLayerRecreated(ui::Layer* old_layer) override;

  // ui::LayerDelegate:
  void OnPaintLayer(const ui::PaintContext& context) override;
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override;

 private:
  void UpdateBounds();

  ui::LayerOwner layer_owner_;
  raw_ptr<views::View> view_ = nullptr;
  gfx::RoundedCornersF corner_radii_;
  const ui::ColorId color_id_;
  const int outline_width_;
  gfx::Insets insets_;

  base::ScopedObservation<ui::LayerOwner, ui::LayerOwner::Observer>
      layer_owner_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_VIEW_OUTLINE_H_
