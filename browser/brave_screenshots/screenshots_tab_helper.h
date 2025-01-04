// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_HELPER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class DevToolsAgentHost;
class DevToolsAgentHostClient;
}  // namespace content

namespace brave_screenshots {

// This is a convenience method which enables us to defer attaching the tab
// helper until the first request for a screenshot has been received.
void TakeScreenshot(base::WeakPtr<content::WebContents>, int);

class BraveScreenshotsTabHelper
    : public image_editor::ScreenshotFlow,
      public content::WebContentsUserData<BraveScreenshotsTabHelper> {
 public:
  void Start();
  void StartFullscreenCapture();
  void StartScreenshotFullPageToClipboard();
  void OnCaptureComplete(const image_editor::ScreenshotCaptureResult&);

  // Delete the copy constructor and assignment operator
  BraveScreenshotsTabHelper(const BraveScreenshotsTabHelper&) = delete;
  BraveScreenshotsTabHelper& operator=(const BraveScreenshotsTabHelper&) =
      delete;
  ~BraveScreenshotsTabHelper() override;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

 private:
  friend class content::WebContentsUserData<BraveScreenshotsTabHelper>;
  explicit BraveScreenshotsTabHelper(content::WebContents*);

  // DevToolsAgentHost/Client used to capture full page screenshots
  bool InitializeDevToolsAgentHost();
  void SendCaptureFullscreenCommand();

  scoped_refptr<content::DevToolsAgentHost> devtools_agent_host_ = nullptr;
  std::unique_ptr<content::DevToolsAgentHostClient>
      devtools_agent_host_client_ = nullptr;
  base::WeakPtr<BraveScreenshotsTabHelper> weak_this_ = nullptr;
  base::WeakPtrFactory<BraveScreenshotsTabHelper> weak_factory_{this};

};  // class BraveScreenshotsTabHelper

}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_SCREENSHOTS_TAB_HELPER_H_
