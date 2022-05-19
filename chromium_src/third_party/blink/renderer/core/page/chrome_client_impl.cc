/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/page/chrome_client_impl.cc"

#include "third_party/blink/public/common/features.h"

namespace blink {

const display::ScreenInfos& ChromeClientImpl::BraveGetScreenInfos(
    LocalFrame& frame) const {
  display::ScreenInfo screenInfo = GetScreenInfo(frame);
  LocalDOMWindow* dom_window = frame.DomWindow();
  if (!dom_window) {
    return GetScreenInfos(frame);
  }
  ExecutionContext* context = dom_window->GetExecutionContext();
  if (brave::AllowScreenFingerprinting(context)) {
    return GetScreenInfos(frame);
  }
  gfx::Rect farbledScreenRect(dom_window->screenX(), dom_window->screenY(),
                              dom_window->outerWidth(),
                              dom_window->outerHeight());
  screenInfo.rect = farbledScreenRect;
  screenInfo.available_rect = farbledScreenRect;
  screenInfo.is_extended = false;
  screenInfo.is_primary = false;
  screen_infos_ = display::ScreenInfos(screenInfo);
  return screen_infos_;
}

}  // namespace blink
