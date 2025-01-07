// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SCREENSHOTS_DEVTOOLS_HELPER_H_
#define BRAVE_BROWSER_BRAVE_SCREENSHOTS_DEVTOOLS_HELPER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"

namespace brave_screenshots {

class DevToolsHelper {
 public:
  DevToolsHelper(base::WeakPtr<content::WebContents> web_contents,
                 image_editor::ScreenshotCaptureCallback callback);
  ~DevToolsHelper();

  bool Attach();
  void SendCaptureFullscreenCommand();

 private:
  class DevToolsClientImpl;
  scoped_refptr<content::DevToolsAgentHost> devtools_host_ = nullptr;
  std::unique_ptr<content::DevToolsAgentHostClient> devtools_client_ = nullptr;
};

}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_DEVTOOLS_HELPER_H_
