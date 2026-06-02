/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/hid/hid.h"

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"

namespace blink {
namespace {

bool RequireTransientActivationForHidRequestDevice(LocalFrame* frame) {
  if (frame && frame->GetContentSettingsClient()) {
    return frame->GetContentSettingsClient()
        ->RequireTransientActivationForHidRequestDevice();
  }

  return true;
}

}  // namespace
}  // namespace blink

#define BRAVE_REQUIRE_TRANSIENT_ACTIVATION_FOR_HID_REQUEST_DEVICE(window) \
  RequireTransientActivationForHidRequestDevice(window)

#include <third_party/blink/renderer/modules/hid/hid.cc>

#undef BRAVE_REQUIRE_TRANSIENT_ACTIVATION_FOR_HID_REQUEST_DEVICE
