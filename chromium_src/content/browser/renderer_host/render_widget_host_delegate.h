/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_DELEGATE_H_

// Callback to give the browser a chance to handle the specified mouse
// event before sending it to the renderer. Returns true if the event was
// handled, false otherwise. A true value means no more processing should
// happen on the event. The default return value is false.
#define PreHandleKeyboardEvent(...)    \
  PreHandleKeyboardEvent(__VA_ARGS__); \
  virtual bool PreHandleMouseEvent(const blink::WebMouseEvent& event)

#include <content/browser/renderer_host/render_widget_host_delegate.h>  // IWYU pragma: export

#undef PreHandleKeyboardEvent

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_DELEGATE_H_
