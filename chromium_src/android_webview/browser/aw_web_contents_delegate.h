/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_ANDROID_WEBVIEW_BROWSER_AW_WEB_CONTENTS_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_ANDROID_WEBVIEW_BROWSER_AW_WEB_CONTENTS_DELEGATE_H_

#include "components/embedder_support/android/delegate/web_contents_delegate_android.h"

#define CheckMediaAccessPermission                                             \
  CheckMediaAccessPermission_ChromiumImpl(                                     \
      content::RenderFrameHost* render_frame_host,                             \
      const url::Origin& security_origin, blink::mojom::MediaStreamType type); \
  bool CheckMediaAccessPermission

#include "src/android_webview/browser/aw_web_contents_delegate.h"  // IWYU pragma: export

#undef CheckMediaAccessPermission

#endif  // BRAVE_CHROMIUM_SRC_ANDROID_WEBVIEW_BROWSER_AW_WEB_CONTENTS_DELEGATE_H_
