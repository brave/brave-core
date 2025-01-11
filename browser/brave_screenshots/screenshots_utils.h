// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_UTILS_H_
#define BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_UTILS_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "chrome/browser/ui/browser.h"

namespace brave_screenshots {
namespace utils {

// While the image will be written to the clipboard, depending on its size it
// may not be displayed within Windows' clipboard history (Win+V). The limit is
// (reportedly) 4MB. Larger screenshots will be written to the clipboard, but
// will not be displayed in the clipboard history.
void CopyImageToClipboard(const image_editor::ScreenshotCaptureResult& result);

void DisplayScreenshotBubble(
    const image_editor::ScreenshotCaptureResult& result,
    base::WeakPtr<Browser> browser);

}  // namespace utils
}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_UTILS_H_
