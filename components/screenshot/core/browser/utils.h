// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SCREENSHOT_CORE_BROWSER_UTILS_H_
#define BRAVE_COMPONENTS_SCREENSHOT_CORE_BROWSER_UTILS_H_

#include "third_party/skia/include/core/SkBitmap.h"

namespace screenshot {

// Scales `bitmap` so it fits within 1024x768 while preserving aspect ratio,
// centering the result on a transparent canvas. Bitmaps already within those
// bounds are returned unchanged.
SkBitmap ScaleDownBitmap(const SkBitmap& bitmap);

}  // namespace screenshot

#endif  // BRAVE_COMPONENTS_SCREENSHOT_CORE_BROWSER_UTILS_H_
