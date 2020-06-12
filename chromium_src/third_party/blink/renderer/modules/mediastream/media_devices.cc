/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl_helper.h"

#define BRAVE_MEDIA_DEVICES_ENUMERATE_DEVICES          \
  if (!AllowFingerprinting(frame)) {                   \
    return ScriptPromise::CastUndefined(script_state); \
  }

#include "../../../../../../../third_party/blink/renderer/modules/mediastream/media_devices.cc"
#undef BRAVE_MEDIA_DEVICES_ENUMERATE_DEVICES
