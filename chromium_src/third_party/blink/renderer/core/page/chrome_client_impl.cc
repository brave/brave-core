/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <array>

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/screen.h"
#include "ui/display/screen_info.h"
#include "ui/display/screen_infos.h"
#include "ui/gfx/geometry/rect.h"

#include <third_party/blink/renderer/core/page/chrome_client_impl.cc>

namespace blink {

namespace {

constexpr auto allowed_desktop_screen_sizes = std::to_array<const gfx::Size>({
    gfx::Size(1280, 800),
    gfx::Size(1366, 768),
    gfx::Size(1440, 900),
    gfx::Size(1680, 1050),
    gfx::Size(1920, 1080),
    gfx::Size(2560, 1440),
    gfx::Size(3840, 2160),
});

}  // namespace

const display::ScreenInfos& ChromeClientImpl::BraveGetScreenInfos(
    LocalFrame& frame) const {
  LocalDOMWindow* dom_window = frame.DomWindow();
  if (!dom_window) {
    return GetScreenInfos(frame);
  }
  ExecutionContext* context = dom_window->GetExecutionContext();
  if (!brave::BlockScreenFingerprinting(context)) {
    return GetScreenInfos(frame);
  }
  display::ScreenInfo screen_info = GetScreenInfo(frame);
  screen_info.rect = gfx::Rect(allowed_desktop_screen_sizes.back().width(),
                               allowed_desktop_screen_sizes.back().height());
  const int outerWidth = dom_window->outerWidth();
  const int outerHeight = dom_window->outerHeight();
  for (const auto& size : allowed_desktop_screen_sizes) {
    if (size.width() >= outerWidth && size.height() >= outerHeight) {
      screen_info.rect = gfx::Rect(size.width(), size.height());
      break;
    }
  }
  screen_info.available_rect = screen_info.rect;
  screen_info.is_extended = false;
  screen_info.is_primary = false;
  screen_infos_ = display::ScreenInfos(screen_info);
  return screen_infos_;
}

}  // namespace blink
