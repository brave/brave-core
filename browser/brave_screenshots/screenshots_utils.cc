// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_screenshots/screenshots_utils.h"

#include "brave/browser/ui/brave_browser_window.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

namespace brave_screenshots::utils {

using image_editor::ScreenshotCaptureResult;

void CopyImageToClipboard(const ScreenshotCaptureResult& result) {
  DVLOG(2) << __func__;
  if (result.image.IsEmpty()) {
    DVLOG(2) << "Image is empty";
    return;
  }

  DVLOG(2) << "Writing image to clipboard";
  // Copy the image to the user's clipboard
  ui::ScopedClipboardWriter(ui::ClipboardBuffer::kCopyPaste)
      .WriteImage(*result.image.ToSkBitmap());
}

void DisplayScreenshotBubble(const ScreenshotCaptureResult& result,
                             base::WeakPtr<content::WebContents> web_contents) {
  DVLOG(2) << __func__;
  if (!web_contents.get() || result.image.IsEmpty()) {
    DVLOG(2) << "Browser is null or image is empty";
    return;
  }

  DVLOG(2) << "Displaying screenshot bubble";
  // Leverage the screenshot bubble to show the user the screenshot
  BrowserWindow::FindBrowserWindowWithWebContents(web_contents.get())
      ->ShowScreenshotCapturedBubble(web_contents.get(), result.image);
}

}  // namespace brave_screenshots::utils
