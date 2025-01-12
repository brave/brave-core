// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_screenshots/strategies/selection_strategy.h"

#include <utility>

#include "base/logging.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "content/public/browser/web_contents.h"

namespace brave_screenshots {

SelectionStrategy::SelectionStrategy(content::WebContents* web_contents)
    : image_editor::ScreenshotFlow(web_contents) {
  DVLOG(2) << "SelectionStrategy created";
}

SelectionStrategy::~SelectionStrategy() {
  DVLOG(2) << "SelectionStrategy destroyed";
}

// Requests the user to select a region of the screen to capture
void SelectionStrategy::Capture(
    content::WebContents* web_contents,
    image_editor::ScreenshotCaptureCallback callback) {
  DVLOG(2) << "SelectionStrategy::Capture";
  image_editor::ScreenshotFlow::Start(std::move(callback));
}

bool SelectionStrategy::DidClipScreenshot() const {
  return false;
}

}  // namespace brave_screenshots
