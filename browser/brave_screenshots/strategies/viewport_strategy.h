// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SCREENSHOTS_STRATEGIES_VIEWPORT_STRATEGY_H_
#define BRAVE_BROWSER_BRAVE_SCREENSHOTS_STRATEGIES_VIEWPORT_STRATEGY_H_

#include "brave/browser/brave_screenshots/strategies/screenshot_strategy.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "content/public/browser/web_contents.h"

namespace brave_screenshots {
class ViewportStrategy : public BraveScreenshotStrategy,
                         public image_editor::ScreenshotFlow {
 public:
  explicit ViewportStrategy(content::WebContents* web_contents);
  // Disable copy and assign
  ViewportStrategy(const ViewportStrategy&) = delete;
  ViewportStrategy& operator=(const ViewportStrategy&) = delete;
  ~ViewportStrategy() override;
  // Captures the visible portion of the page (viewport)
  void Capture(content::WebContents* web_contents,
               image_editor::ScreenshotCaptureCallback callback) override;

  bool DidClipScreenshot() const override;
};
}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_STRATEGIES_VIEWPORT_STRATEGY_H_
