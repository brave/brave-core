/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_IMPL_H_

#include "content/public/browser/render_widget_host.h"

#define ForwardMouseEvent                                                  \
  ForwardMouseEvent_ChromiumImpl(const blink::WebMouseEvent& mouse_event); \
  void ForwardMouseEvent

#define ForwardMouseEventWithLatencyInfo         \
  ForwardMouseEventWithLatencyInfo_ChromiumImpl( \
      const blink::WebMouseEvent& mouse_event,   \
      const ui::LatencyInfo& latency);           \
  void ForwardMouseEventWithLatencyInfo

#include <content/browser/renderer_host/render_widget_host_impl.h>  // IWYU pragma: export

#undef ForwardMouseEventWithLatencyInfo
#undef ForwardMouseEvent

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_IMPL_H_
