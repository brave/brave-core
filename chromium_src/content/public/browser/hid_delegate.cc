/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <content/public/browser/hid_delegate.cc>

namespace content {

bool HidDelegate::AllowRequestDeviceWithoutTransientActivation(
    RenderFrameHost* render_frame_host) {
  return false;
}

}  // namespace content
