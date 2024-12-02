/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "android_webview/browser/aw_web_contents_delegate.h"

namespace content {
class RenderFrameHost;
}

namespace url {
class Origin;
}

namespace android_webview {

bool AwWebContentsDelegate::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const url::Origin& security_origin,
    blink::mojom::MediaStreamType type) {
  return false;
}

}  // namespace android_webview

#define CheckMediaAccessPermission CheckMediaAccessPermission_ChromiumImpl

#include "src/android_webview/browser/aw_web_contents_delegate.cc"

#undef CheckMediaAccessPermission
