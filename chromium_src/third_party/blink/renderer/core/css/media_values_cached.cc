/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/css/media_values_cached.h"

// Indicate that these values are accessed early (to be cached) so we don't
// want to trigger a fingerprint settings check.
#define CalculateDeviceWidth(...) \
  CalculateDeviceWidth(__VA_ARGS__, /* early = */ true)
#define CalculateDeviceHeight(...) \
  CalculateDeviceHeight(__VA_ARGS__, /* early = */ true)

#include "src/third_party/blink/renderer/core/css/media_values_cached.cc"

#undef CalculateDeviceWidth
#undef CalculateDeviceHeight
