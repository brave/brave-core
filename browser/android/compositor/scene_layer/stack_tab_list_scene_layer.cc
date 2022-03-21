/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/compositor/scene_layer/stack_tab_list_scene_layer.h"

#include "brave/build/android/jni_headers/StackTabListSceneLayer_jni.h"
#include "chrome/browser/android/compositor/layer/tab_layer.h"
#include "chrome/browser/android/compositor/layer_title_cache.h"

using base::android::JavaParamRef;
using base::android::JavaRef;

namespace android {

StackTabListSceneLayer::StackTabListSceneLayer(JNIEnv* env,
                                               const JavaRef<jobject>& jobj)
    : TabListSceneLayer(env, jobj), layer_title_cache_(nullptr) {}

StackTabListSceneLayer::~StackTabListSceneLayer() {}

void StackTabListSceneLayer::PutStackTabLayer(
    JNIEnv* env,
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
    jboolean inset_border) {
  if (!layer_title_cache_)
    return;

  scoped_refptr<TabLayer> layer;
  auto iter = tab_map_.find(id);
  if (iter != tab_map_.end()) {
    layer = iter->second;
  } else {
    layer =
        TabLayer::Create(incognito, resource_manager_, tab_content_manager_);
    tab_map_.insert(
        std::map<int, scoped_refptr<TabLayer>>::value_type(id, layer));
  }

  DCHECK(layer);
  if (layer) {
    layer->InitStack(layer_title_cache_);

    layer->SetStackProperties(
        id, border_resource_id, x, y, width, alpha, border_alpha, border_scale,
        content_width, content_height, default_theme_color, inset_border,
        close_button_resource_id, close_button_on_right, pivot_x, pivot_y,
        rotation_x, rotation_y, close_alpha, close_btn_width,
        close_btn_asset_size, close_button_color, show_tab_title);
  }
}

void StackTabListSceneLayer::SetStackDependencies(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jobj,
    const base::android::JavaParamRef<jobject>& jlayer_title_cache) {
  if (!layer_title_cache_) {
    layer_title_cache_ = LayerTitleCache::FromJavaObject(jlayer_title_cache);
  }
}

static jlong JNI_StackTabListSceneLayer_Init(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj) {
  // This will automatically bind to the Java object and pass ownership there.
  StackTabListSceneLayer* scene_layer = new StackTabListSceneLayer(env, jobj);
  return reinterpret_cast<intptr_t>(scene_layer);
}

}  // namespace android
