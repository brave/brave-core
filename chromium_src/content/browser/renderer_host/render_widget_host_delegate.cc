/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <content/browser/renderer_host/render_widget_host_delegate.cc>

namespace content {

bool RenderWidgetHostDelegate::PreHandleMouseEvent(
    const blink::WebMouseEvent& event) {
  return false;
}

}  // namespace content
