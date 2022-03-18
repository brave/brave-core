/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_COMPOSITOR_SCENE_LAYER_STACK_TAB_LIST_SCENE_LAYER_H_
#define BRAVE_BROWSER_ANDROID_COMPOSITOR_SCENE_LAYER_STACK_TAB_LIST_SCENE_LAYER_H_

#include <map>
#include <memory>
#include <set>

#include "base/memory/raw_ptr.h"
#include "cc/layers/layer.h"
#include "cc/layers/ui_resource_layer.h"
#include "chrome/browser/android/compositor/layer/layer.h"
#include "chrome/browser/android/compositor/scene_layer/tab_list_scene_layer.h"
#include "chrome/browser/android/compositor/tab_content_manager.h"
#include "third_party/skia/include/core/SkColor.h"

namespace ui {
class ResourceManager;
}

namespace android {

class TabContentManager;
class TabLayer;

class LayerTitleCache;

class StackTabListSceneLayer : public TabListSceneLayer {
 public:
  StackTabListSceneLayer(JNIEnv* env,
                         const base::android::JavaRef<jobject>& jobj);

  StackTabListSceneLayer(const TabListSceneLayer&) = delete;
  StackTabListSceneLayer& operator=(const TabListSceneLayer&) = delete;

  ~StackTabListSceneLayer() override;

  void PutStackTabLayer(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& jobj,
                        jint id,
                        jint close_button_resource_id,
                        jboolean close_button_on_right,
                        jfloat pivot_x,
                        jfloat pivot_y,
                        jfloat rotation_x,
                        jfloat rotation_y,
                        jfloat close_alpha,
                        jfloat close_btn_width,
                        jfloat close_btn_asset_size,
                        jint close_button_color,
                        jboolean show_tab_title,
                        jint border_resource_id,
                        jboolean incognito,
                        jfloat x,
                        jfloat y,
                        jfloat width,
                        jfloat content_width,
                        jfloat content_height,
                        jfloat alpha,
                        jfloat border_alpha,
                        jfloat border_scale,
                        jint default_theme_color,
                        jboolean inset_border);

  void SetStackDependencies(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jobj,
      const base::android::JavaParamRef<jobject>& jlayer_title_cache);

 private:
  raw_ptr<LayerTitleCache> layer_title_cache_;
};

}  // namespace android

#endif  // BRAVE_BROWSER_ANDROID_COMPOSITOR_SCENE_LAYER_STACK_TAB_LIST_SCENE_LAYER_H_
