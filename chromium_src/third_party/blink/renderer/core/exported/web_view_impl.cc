/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/exported/web_view_impl.h"

#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

// not a funtions because Set* methods are protected and WebView is a friend of
// RuntimeEnabledFeatures.
#define BRAVE_APPLY_WEB_PREFERENCES(prefs)                       \
  {                                                              \
    RuntimeEnabledFeatures::SetBraveIsInTorContextEnabled(       \
        prefs.is_tor_window);                                    \
    if (prefs.is_tor_window) {                                   \
      RuntimeEnabledFeatures::SetWebShareEnabled(false);         \
    }                                                            \
    RuntimeEnabledFeatures::SetBraveGlobalPrivacyControlEnabled( \
        prefs.global_privacy_control_enabled);                   \
  }

#define SetAccelerated2dCanvasEnabled(...)    \
  SetAccelerated2dCanvasEnabled(__VA_ARGS__); \
  BRAVE_APPLY_WEB_PREFERENCES(prefs)

#include <third_party/blink/renderer/core/exported/web_view_impl.cc>

#undef SetAccelerated2dCanvasEnabled
#undef BRAVE_APPLY_WEB_PREFERENCES
