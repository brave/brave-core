/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ANDROID_COMPOSITOR_LAYER_TAB_LAYER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ANDROID_COMPOSITOR_LAYER_TAB_LAYER_H_

namespace cc {
class UIResourceLayer;
}

namespace android {
class DecorationTitle;
class LayerTitleCache;
}  // namespace android

#define SetProperties                                                          \
  SetStackProperties(int id, int border_resource_id, float x, float y,         \
                     float width, float alpha, float border_alpha,             \
                     float border_scale, float content_width,                  \
                     float content_height, int default_theme_color,            \
                     bool inset_border, int close_button_resource_id,          \
                     bool close_button_on_right, float pivot_x, float pivot_y, \
                     float rotation_x, float rotation_y, float close_alpha,    \
                     float close_btn_width, float close_btn_asset_size,        \
                     int close_button_color, bool show_tab_title);             \
  void SetTitle(DecorationTitle* title);                                       \
  void InitStack(LayerTitleCache* layer_title_cache);                          \
  void SetProperties

#define brightness_                                 \
  Dummy() { return 0; }                             \
  scoped_refptr<cc::Layer> title_;                  \
  scoped_refptr<cc::UIResourceLayer> close_button_; \
  raw_ptr<LayerTitleCache> layer_title_cache_;      \
  float brightness_

#include "src/chrome/browser/android/compositor/layer/tab_layer.h"

#undef SetProperties
#undef brightness_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ANDROID_COMPOSITOR_LAYER_TAB_LAYER_H_
