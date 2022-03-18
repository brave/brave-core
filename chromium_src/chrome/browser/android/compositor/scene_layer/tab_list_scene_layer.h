/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ANDROID_COMPOSITOR_SCENE_LAYER_TAB_LIST_SCENE_LAYER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ANDROID_COMPOSITOR_SCENE_LAYER_TAB_LIST_SCENE_LAYER_H_

#define tab_map_               \
  *Dummy() { return nullptr; } \
                               \
 protected:                    \
  TabMap tab_map_

#include "src/chrome/browser/android/compositor/scene_layer/tab_list_scene_layer.h"

#undef tab_map_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ANDROID_COMPOSITOR_SCENE_LAYER_TAB_LIST_SCENE_LAYER_H_
