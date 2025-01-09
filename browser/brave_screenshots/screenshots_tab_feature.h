// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_FEATURE_H_
#define BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_FEATURE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_screenshots/strategies/screenshot_strategy.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/browser/web_contents.h"

namespace brave_screenshots {

enum ScreenshotType {
  kSelection,
  kViewport,
  kFullPage,
};

class BraveScreenshotsTabFeature {
 public:
  BraveScreenshotsTabFeature();
  BraveScreenshotsTabFeature(const BraveScreenshotsTabFeature&) = delete;
  BraveScreenshotsTabFeature& operator=(const BraveScreenshotsTabFeature&) =
      delete;
  ~BraveScreenshotsTabFeature();

  void StartScreenshot(Browser* browser, ScreenshotType type);
  void OnCaptureComplete(const image_editor::ScreenshotCaptureResult& result);

 private:
  std::unique_ptr<BraveScreenshotStrategy> CreateStrategy(ScreenshotType type);
  base::WeakPtr<Browser> browser_ = nullptr;
  std::unique_ptr<BraveScreenshotStrategy> strategy_ = nullptr;
  base::WeakPtr<content::WebContents> web_contents_ = nullptr;
  base::WeakPtrFactory<BraveScreenshotsTabFeature> weak_factory_{this};

};  // class BraveScreenshotsTabFeature

}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_FEATURE_H_
