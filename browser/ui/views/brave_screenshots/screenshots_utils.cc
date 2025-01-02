// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_screenshots/screenshots_utils.h"

#include "chrome/browser/image_editor/screenshot_flow.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

namespace brave_screenshots::utils {

void NotifyUserOfScreenshot(const image_editor::ScreenshotCaptureResult& result,
                            base::WeakPtr<content::WebContents> web_contents) {
  if (!web_contents || result.image.IsEmpty()) {
    return;
  }

  // Copy the image to the user's clipboard
  ui::ScopedClipboardWriter(ui::ClipboardBuffer::kCopyPaste)
      .WriteImage(*result.image.ToSkBitmap());

  // Notify the user that the screenshot has been taken
  auto* browser = chrome::FindBrowserWithTab(web_contents.get());

  if (!browser) {
    VLOG(1) << "Failed to find browser for web contents";
    return;
  }

  auto* window = browser->window();

  if (!window) {
    VLOG(1) << "Failed to find window for browser";
    return;
  }

  // Leverage the screenshot bubble to show the user the screenshot
  window->ShowScreenshotCapturedBubble(web_contents.get(), result.image);
}

}  // namespace brave_screenshots::utils
