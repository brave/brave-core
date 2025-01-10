// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SCREENSHOTS_STRATEGIES_SCREENSHOT_STRATEGY_H_
#define BRAVE_BROWSER_BRAVE_SCREENSHOTS_STRATEGIES_SCREENSHOT_STRATEGY_H_

#include "chrome/browser/image_editor/screenshot_flow.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/image/image.h"

namespace brave_screenshots {

class BraveScreenshotStrategy {
 public:
  virtual ~BraveScreenshotStrategy() = default;

  // Did the strategy clip/resize the screenshot?
  virtual bool DidClipScreenshot() const = 0;

  virtual void Capture(
      content::WebContents* web_contents,
      const image_editor::ScreenshotCaptureCallback callback) = 0;
};
}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_STRATEGIES_SCREENSHOT_STRATEGY_H_
