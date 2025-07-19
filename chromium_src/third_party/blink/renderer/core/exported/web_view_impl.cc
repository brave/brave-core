/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/exported/web_view_impl.h"

#include "third_party/blink/public/platform/web_runtime_features.h"

#if BUILDFLAG(IS_MAC)
#define SetAccelerated2dCanvasEnabled(...)             \
  SetAccelerated2dCanvasEnabled(__VA_ARGS__);          \
  if (prefs.disable_web_share) {                       \
    RuntimeEnabledFeatures::SetWebShareEnabled(false); \
  }
#endif

#include <third_party/blink/renderer/core/exported/web_view_impl.cc>

#if BUILDFLAG(IS_MAC)
#undef SetAccelerated2dCanvasEnabled
#endif
