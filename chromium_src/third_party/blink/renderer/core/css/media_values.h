/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_VALUES_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_VALUES_H_

#define CalculateDeviceWidth                      \
  CalculateDeviceWidth_ChromiumImpl(LocalFrame*); \
  static int CalculateDeviceWidth

#define CalculateDeviceHeight                      \
  CalculateDeviceHeight_ChromiumImpl(LocalFrame*); \
  static int CalculateDeviceHeight

#include "src/third_party/blink/renderer/core/css/media_values.h"  // IWYU pragma: export

#undef CalculateDeviceWidth
#undef CalculateDeviceHeight

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_VALUES_H_
