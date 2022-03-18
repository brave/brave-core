/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/android/compositor/decoration_title.h"
#include "chrome/browser/android/compositor/layer_title_cache.h"

#include "src/chrome/browser/android/compositor/layer/tab_layer.cc"

namespace android {
void TabLayer::SetStackProperties(int id,
                                  int border_resource_id,
                                  float x,
                                  float y,
                                  float width,
                                  float alpha,
                                  float border_alpha,
                                  float border_scale,
                                  float content_width,
                                  float content_height,
                                  int default_theme_color,
                                  bool inset_border,
                                  int close_button_resource_id,
                                  bool close_button_on_right,
                                  float pivot_x,
                                  float pivot_y,
                                  float rotation_x,
                                  float rotation_y,
                                  float close_alpha,
                                  float close_btn_width,
                                  float close_btn_asset_size,
                                  int close_button_color,
                                  bool show_tab_title) {
  if (!title_ || !close_button_)
    return;

  // Global Transform
  if (rotation_x != 0 || rotation_y != 0) {
    gfx::PointF pivot_origin(pivot_y, pivot_x);

    gfx::Transform transform;
    // Apply screen perspective if there are rotations.
    transform.Translate(content_width / 2, content_height / 2);
    transform.ApplyPerspectiveDepth(
        content_width > content_height ? content_width : content_height);
    transform.Translate(-content_width / 2, -content_height / 2);

    // Translate to correct position on the screen
    transform.Translate(x, y);

    // Apply pivot rotations
    transform.Translate(pivot_origin.x(), pivot_origin.y());
    transform.RotateAboutYAxis(rotation_y);
    transform.RotateAboutXAxis(-rotation_x);
    transform.Translate(-pivot_origin.x(), -pivot_origin.y());
    transform.Scale(border_scale, border_scale);
    layer_->SetTransform(transform);
  }

  // Close button and title
  ui::Resource* close_btn_resource =
      resource_manager_->GetStaticResourceWithTint(close_button_resource_id,
                                                   close_button_color);
  DecorationTitle* title_layer = nullptr;

  const float close_btn_effective_width = close_btn_width * close_alpha;
  close_alpha *= alpha;

  ui::NinePatchResource* border_resource =
      ui::NinePatchResource::From(resource_manager_->GetStaticResourceWithTint(
          border_resource_id, default_theme_color));
  const gfx::RectF border_padding(border_resource->padding());
  gfx::Size close_button_size(close_btn_width, border_padding.y());
  gfx::Size title_size(width - close_btn_effective_width, border_padding.y());

  gfx::PointF close_button_position;
  gfx::PointF title_position;

  close_button_position.set_y(-border_padding.y());
  title_position.set_y(-border_padding.y());
  if (close_button_on_right)
    close_button_position.set_x(width - close_button_size.width());
  else
    title_position.set_x(close_btn_effective_width);

  //----------------------------------------------------------------------------
  // Center Specific Assets in the Rects
  //----------------------------------------------------------------------------
  close_button_position.Offset(
      (close_button_size.width() - close_btn_asset_size) / 2.f,
      (close_button_size.height() - close_btn_asset_size) / 2.f);
  close_button_size.SetSize(close_btn_asset_size, close_btn_asset_size);

  if (inset_border) {
    float inset_diff = inset_border ? border_padding.y() : 0.f;
    close_button_position.set_y(close_button_position.y() + inset_diff);
    title_position.set_y(title_position.y() + inset_diff);
  }

  bool title_visible = border_alpha > 0.f && show_tab_title;
  bool close_btn_visible = title_visible;

  title_position.Offset(0.5f, 0.5f);
  close_button_position.Offset(0.5f, 0.5f);

  if (title_visible && layer_title_cache_)
    title_layer = layer_title_cache_->GetTitleLayer(id);
  SetTitle(title_layer);

  close_button_->SetUIResourceId(close_btn_resource->ui_resource()->id());

  if (title_layer) {
    gfx::PointF vertically_centered_position(
        title_position.x(),
        title_position.y() +
            (title_size.height() - title_layer->size().height()) / 2.f);

    title_->SetPosition(vertically_centered_position);
    title_layer->setBounds(title_size);
    title_layer->setOpacity(border_alpha);
  }

  close_button_->SetHideLayerAndSubtree(!close_btn_visible);
  if (close_btn_visible) {
    close_button_->SetPosition(close_button_position);
    close_button_->SetBounds(close_button_size);
    // Non-linear alpha looks better.
    close_button_->SetOpacity(close_alpha * close_alpha * border_alpha);
  }
}

void TabLayer::SetTitle(DecorationTitle* title) {
  scoped_refptr<cc::Layer> layer = title ? title->layer() : nullptr;

  if (!layer.get()) {
    title_->RemoveAllChildren();
  } else {
    const cc::LayerList& children = title_->children();
    if (children.size() == 0 || children[0]->id() != layer->id()) {
      title_->RemoveAllChildren();
      title_->AddChild(layer);
    }
  }

  if (title)
    title->SetUIResourceIds();
}

void TabLayer::InitStack(LayerTitleCache* layer_title_cache) {
  // We need to init it only once.
  if (title_ || close_button_)
    return;

  layer_title_cache_ = layer_title_cache;
  title_ = cc::Layer::Create();
  close_button_ = cc::UIResourceLayer::Create();
  layer_->AddChild(title_.get());
  layer_->AddChild(close_button_);
  close_button_->SetIsDrawable(true);
}
}  // namespace android
