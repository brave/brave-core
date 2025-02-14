// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_screenshots/strategies/viewport_strategy.h"

#include <utility>

#include "base/logging.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "content/public/browser/web_contents.h"

namespace brave_screenshots {

ViewportStrategy::ViewportStrategy(content::WebContents* web_contents)
    : image_editor::ScreenshotFlow(web_contents) {
  DVLOG(2) << "ViewportStrategy created";
}

ViewportStrategy::~ViewportStrategy() {
  DVLOG(2) << "ViewportStrategy destroyed";
}

// Captures the visible portion of the page (viewport)
void ViewportStrategy::Capture(
    content::WebContents* web_contents,
    image_editor::ScreenshotCaptureCallback callback) {
  DVLOG(2) << "ViewportStrategy::Capture";
  image_editor::ScreenshotFlow::StartFullscreenCapture(std::move(callback));
}

bool ViewportStrategy::DidClipScreenshot() const {
  return false;
}

}  // namespace brave_screenshots
