// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_FEATURE_H_
#define BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_FEATURE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/image_editor/screenshot_flow.h"

namespace content {
class DevToolsAgentHost;
class DevToolsAgentHostClient;
}  // namespace content

namespace brave_screenshots {

// This is a convenience method which enables us to defer attaching the tab
// helper until the first request for a screenshot has been received.
void TakeScreenshot(base::WeakPtr<content::WebContents>, int);

class BraveScreenshotsTabFeature : public image_editor::ScreenshotFlow {
 public:
  explicit BraveScreenshotsTabFeature(content::WebContents*);
  void Start();
  void StartFullscreenCapture();
  void StartScreenshotFullPageToClipboard();
  void OnCaptureComplete(const image_editor::ScreenshotCaptureResult&);

  // Delete the copy constructor and assignment operator
  BraveScreenshotsTabFeature(const BraveScreenshotsTabFeature&) = delete;
  BraveScreenshotsTabFeature& operator=(const BraveScreenshotsTabFeature&) =
      delete;
  ~BraveScreenshotsTabFeature() override;

 private:
  // DevToolsAgentHost/Client used to capture full page screenshots
  bool InitializeDevToolsAgentHost();
  void SendCaptureFullscreenCommand();

  scoped_refptr<content::DevToolsAgentHost> devtools_agent_host_ = nullptr;
  std::unique_ptr<content::DevToolsAgentHostClient>
      devtools_agent_host_client_ = nullptr;
  base::WeakPtr<BraveScreenshotsTabFeature> weak_this_ = nullptr;
  base::WeakPtrFactory<BraveScreenshotsTabFeature> weak_factory_{this};

};  // class BraveScreenshotsTabFeature

}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_FEATURE_H_
