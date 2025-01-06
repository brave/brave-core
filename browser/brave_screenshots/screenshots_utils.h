// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_UTILS_H_
#define BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_UTILS_H_

#include "base/memory/weak_ptr.h"

class Browser;

namespace image_editor {
struct ScreenshotCaptureResult;
}  // namespace image_editor

namespace brave_screenshots {
namespace utils {
void CopyImageToClipboard(const image_editor::ScreenshotCaptureResult& result);
void DisplayScreenshotBubble(
    const image_editor::ScreenshotCaptureResult& result,
    base::WeakPtr<Browser> browser);
}  // namespace utils
}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_UTILS_H_
