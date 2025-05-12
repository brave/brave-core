// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SCREENSHOTS_BRAVE_SCREENSHOTS_TAB_FEATURE_H_
#define BRAVE_BROWSER_BRAVE_SCREENSHOTS_BRAVE_SCREENSHOTS_TAB_FEATURE_H_

#include <memory>

#include "base/memory/weak_ptr.h"

namespace content {
class WebContents;
}  // namespace content

namespace image_editor {
struct ScreenshotCaptureResult;
}  // namespace image_editor

class Browser;

namespace brave_screenshots {

class BraveScreenshotStrategy;

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

 private:
  void OnCaptureComplete(const image_editor::ScreenshotCaptureResult& result);
  std::unique_ptr<BraveScreenshotStrategy> CreateStrategy(ScreenshotType type);
  base::WeakPtr<Browser> browser_ = nullptr;
  std::unique_ptr<BraveScreenshotStrategy> strategy_ = nullptr;
  base::WeakPtr<content::WebContents> web_contents_ = nullptr;
  base::WeakPtrFactory<BraveScreenshotsTabFeature> weak_factory_{this};

};  // class BraveScreenshotsTabFeature

}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_BRAVE_SCREENSHOTS_TAB_FEATURE_H_
