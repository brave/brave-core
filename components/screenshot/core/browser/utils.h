// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SCREENSHOT_CORE_BROWSER_UTILS_H_
#define BRAVE_COMPONENTS_SCREENSHOT_CORE_BROWSER_UTILS_H_

#include "third_party/skia/include/core/SkBitmap.h"

namespace screenshot {

// Scales `bitmap` down so it fits within 1024x768 while preserving aspect
// ratio, so only one dimension reaches those bounds. Bitmaps already within
// them are returned unchanged, and the result is never upscaled. Safe to call
// on any platform, including iOS. Returns an empty bitmap if `bitmap` is not
// N32.
SkBitmap ScaleDownBitmap(const SkBitmap& bitmap);

}  // namespace screenshot

#endif  // BRAVE_COMPONENTS_SCREENSHOT_CORE_BROWSER_UTILS_H_
