/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/page/chrome_client_impl.cc"

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/screen.h"
#include "ui/display/screen_info.h"
#include "ui/display/screen_infos.h"
#include "ui/gfx/geometry/rect.h"

namespace blink {

const display::ScreenInfos& ChromeClientImpl::BraveGetScreenInfos(
    LocalFrame& frame) const {
  display::ScreenInfo screen_info = GetScreenInfo(frame);
  LocalDOMWindow* dom_window = frame.DomWindow();
  if (!dom_window) {
    return GetScreenInfos(frame);
  }
  ExecutionContext* context = dom_window->GetExecutionContext();
  if (!brave::BlockScreenFingerprinting(context)) {
    return GetScreenInfos(frame);
  }
  gfx::Rect farbled_screen_rect(dom_window->screenX(), dom_window->screenY(),
                                dom_window->outerWidth(),
                                dom_window->outerHeight());
  screen_info.rect = farbled_screen_rect;
  screen_info.available_rect = farbled_screen_rect;
  screen_info.is_extended = false;
  screen_info.is_primary = false;
  screen_infos_ = display::ScreenInfos(screen_info);
  return screen_infos_;
}

}  // namespace blink
