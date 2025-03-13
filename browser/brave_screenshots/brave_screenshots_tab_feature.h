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

namespace brave_screenshots {

class BraveScreenshotStrategy;

enum ScreenshotType {
  kSelection,
  kViewport,
  kFullPage,
};

class BraveScreenshotsTabFeature {
 public:
  explicit BraveScreenshotsTabFeature(content::WebContents* web_contents);
  BraveScreenshotsTabFeature(const BraveScreenshotsTabFeature&) = delete;
  BraveScreenshotsTabFeature& operator=(const BraveScreenshotsTabFeature&) =
      delete;
  ~BraveScreenshotsTabFeature();

  void StartScreenshot(ScreenshotType type);
  bool IsScreenshotInProgress() const;

 private:
  void OnCaptureComplete(const image_editor::ScreenshotCaptureResult& result);
  std::unique_ptr<BraveScreenshotStrategy> CreateStrategy(ScreenshotType type);
  std::unique_ptr<BraveScreenshotStrategy> strategy_ = nullptr;
  raw_ptr<content::WebContents> web_contents_ = nullptr;
  base::WeakPtrFactory<BraveScreenshotsTabFeature> weak_factory_{this};

};  // class BraveScreenshotsTabFeature

}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_BRAVE_SCREENSHOTS_TAB_FEATURE_H_
