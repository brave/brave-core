/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/render_widget_host_impl.h"

#define ForwardMouseEvent ForwardMouseEvent_ChromiumImpl
#define ForwardMouseEventWithLatencyInfo \
  ForwardMouseEventWithLatencyInfo_ChromiumImpl

#include <content/browser/renderer_host/render_widget_host_impl.cc>

#undef ForwardMouseEventWithLatencyInfo
#undef ForwardMouseEvent

namespace content {

void RenderWidgetHostImpl::ForwardMouseEvent(const WebMouseEvent& mouse_event) {
  // As this method have to call ForwardMouseEvent_ChromiumImpl(),
  // we need to have prehandle here.
  if (delegate_ && delegate_->PreHandleMouseEvent(mouse_event)) {
    return;
  }

  ForwardMouseEvent_ChromiumImpl(mouse_event);
}

void RenderWidgetHostImpl::ForwardMouseEventWithLatencyInfo(
    const WebMouseEvent& mouse_event,
    const ui::LatencyInfo& latency) {
  if (delegate_ && delegate_->PreHandleMouseEvent(mouse_event)) {
    return;
  }

  ForwardMouseEventWithLatencyInfo_ChromiumImpl(mouse_event, latency);
}

}  // namespace content
