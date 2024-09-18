/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/keyboard/navigator_keyboard.h"

#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"

#define keyboard keyboard_ChromiumImpl
#include "src/third_party/blink/renderer/modules/keyboard/navigator_keyboard.cc"
#undef keyboard

namespace blink {

// static
Keyboard* NavigatorKeyboard::keyboard(Navigator& navigator) {
  if (ExecutionContext* context = navigator.GetExecutionContext()) {
    if (brave::GetBraveFarblingLevelFor(
            context, ContentSettingsType::BRAVE_WEBCOMPAT_KEYBOARD,
            BraveFarblingLevel::OFF) != BraveFarblingLevel::OFF) {
      return nullptr;
    }
  }
  return keyboard_ChromiumImpl(navigator);
}

}  // namespace blink
