/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_content_settings_agent_impl_helper.h"

#define BRAVE_XR_SYSTEM_IS_SESSION_SUPPORTED        \
  if (!AllowFingerprinting(frame)) {         \
    query->Resolve(false, &exception_state); \
    return promise;                          \
  }

#include "../../../../../../../third_party/blink/renderer/modules/xr/xr_system.cc"

#undef BRAVE_XR_SYSTEM_IS_SESSION_SUPPORTED
