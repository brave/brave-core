/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/exported/web_view_impl.h"

#include "third_party/blink/public/platform/web_runtime_features.h"

#define SetAccelerated2dCanvasEnabled(...)    \
  SetAccelerated2dCanvasEnabled(__VA_ARGS__); \
  RuntimeEnabledFeatures::SetWebShareEnabled(!prefs.disable_web_share);

#include "src/third_party/blink/renderer/core/exported/web_view_impl.cc"

#undef SetAccelerated2dCanvasEnabled
