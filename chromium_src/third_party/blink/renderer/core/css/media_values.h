/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_VALUES_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_VALUES_H_

#define CalculateDeviceWidth(...)          \
  CalculateDeviceWidth(__VA_ARGS__, bool); \
  static int CalculateDeviceWidth_ChromiumImpl(__VA_ARGS__)

#define CalculateDeviceHeight(...)          \
  CalculateDeviceHeight(__VA_ARGS__, bool); \
  static int CalculateDeviceHeight_ChromiumImpl(__VA_ARGS__)

#include "src/third_party/blink/renderer/core/css/media_values.h"  // IWYU pragma: export

#undef CalculateDeviceWidth
#undef CalculateDeviceHeight

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_VALUES_H_
