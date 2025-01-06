// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_screenshots/screenshots_utils.h"

#include "chrome/browser/image_editor/screenshot_flow.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/public/tab_interface.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

namespace brave_screenshots {
namespace utils {
void CopyImageToClipboard(const image_editor::ScreenshotCaptureResult& result) {
  if (result.image.IsEmpty()) {
    return;
  }

  // Copy the image to the user's clipboard
  ui::ScopedClipboardWriter(ui::ClipboardBuffer::kCopyPaste)
      .WriteImage(*result.image.ToSkBitmap());
}

void DisplayScreenshotBubble(
    const image_editor::ScreenshotCaptureResult& result,
    base::WeakPtr<content::WebContents> web_contents) {
  if (!web_contents || result.image.IsEmpty()) {
    return;
  }

  // Notify the user that the screenshot has been taken
  tabs::TabInterface* tab =
      tabs::TabInterface::GetFromContents(web_contents.get());

  // Leverage the screenshot bubble to show the user the screenshot
  tab->GetBrowserWindowInterface()
      ->GetBrowserForMigrationOnly()
      ->window()
      ->ShowScreenshotCapturedBubble(web_contents.get(), result.image);
}

}  // namespace utils
}  // namespace brave_screenshots
