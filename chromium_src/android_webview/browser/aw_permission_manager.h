/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_ANDROID_WEBVIEW_BROWSER_AW_PERMISSION_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_ANDROID_WEBVIEW_BROWSER_AW_PERMISSION_MANAGER_H_

#define SetOriginCanReadEnumerateDevicesAudioLabels              \
  SetOriginCanReadEnumerateDevicesAudioLabels_ChromiumImpl(      \
      const url::Origin& origin, bool audio);                    \
  void SetOriginCanReadEnumerateDevicesVideoLabels_ChromiumImpl( \
      const url::Origin& origin, bool video);                    \
  void SetOriginCanReadEnumerateDevicesAudioLabels

#include "src/android_webview/browser/aw_permission_manager.h"  // IWYU pragma: export

#undef SetOriginCanReadEnumerateDevicesAudioLabels

#endif  // BRAVE_CHROMIUM_SRC_ANDROID_WEBVIEW_BROWSER_AW_PERMISSION_MANAGER_H_
