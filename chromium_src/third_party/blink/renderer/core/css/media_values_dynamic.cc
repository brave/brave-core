/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/css/media_values_dynamic.h"

// These values are computed just-in-time (not early) so it's fine to
// trigger a fingerprint settings check.
#define CalculateDeviceWidth(...) \
  CalculateDeviceWidth(__VA_ARGS__, /* early = */ false)
#define CalculateDeviceHeight(...) \
  CalculateDeviceHeight(__VA_ARGS__, /* early = */ false)

#include "src/third_party/blink/renderer/core/css/media_values_dynamic.cc"

#undef CalculateDeviceWidth
#undef CalculateDeviceHeight
