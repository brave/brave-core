// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_FEATURE_H_
#define BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_FEATURE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/image_editor/screenshot_flow.h"

class Browser;

namespace content {
class DevToolsAgentHost;
class DevToolsAgentHostClient;
}  // namespace content

namespace brave_screenshots {

enum ScreenshotType {
  kSelection,
  kViewport,
  kFullPage,
};

class BraveScreenshotsTabFeature : public image_editor::ScreenshotFlow {
 public:
  explicit BraveScreenshotsTabFeature(content::WebContents* web_contents);
  void StartScreenshot(Browser* browser, ScreenshotType type);
  void OnCaptureComplete(const image_editor::ScreenshotCaptureResult& result);

  // Delete the copy constructor and assignment operator
  BraveScreenshotsTabFeature(const BraveScreenshotsTabFeature&) = delete;
  BraveScreenshotsTabFeature& operator=(const BraveScreenshotsTabFeature&) =
      delete;
  ~BraveScreenshotsTabFeature() override;

 private:
  // DevToolsAgentHost/Client used to capture full page screenshots
  bool InitDevToolsHelper(image_editor::ScreenshotCaptureCallback callback);
  void SendCaptureFullscreenCommand();

  scoped_refptr<content::DevToolsAgentHost> devtools_host_ = nullptr;
  std::unique_ptr<content::DevToolsAgentHostClient> devtools_client_ = nullptr;
  base::WeakPtr<Browser> browser_ = nullptr;
  base::WeakPtr<BraveScreenshotsTabFeature> weak_this_ = nullptr;
  base::WeakPtrFactory<BraveScreenshotsTabFeature> weak_factory_{this};

};  // class BraveScreenshotsTabFeature

}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_FEATURE_H_
