// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SCREENSHOTS_STRATEGIES_FULLPAGE_STRATEGY_H_
#define BRAVE_BROWSER_BRAVE_SCREENSHOTS_STRATEGIES_FULLPAGE_STRATEGY_H_

#include <string>

#include "brave/browser/brave_screenshots/strategies/screenshot_strategy.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/web_contents.h"

namespace brave_screenshots {

class FullPageStrategy : public BraveScreenshotStrategy,
                         public content::DevToolsAgentHostClient {
 public:
  FullPageStrategy();
  FullPageStrategy(const FullPageStrategy&) = delete;
  FullPageStrategy& operator=(const FullPageStrategy&) = delete;
  ~FullPageStrategy() override;

  // BraveScreenshotStrategy implementation
  void Capture(content::WebContents* web_contents,
               image_editor::ScreenshotCaptureCallback callback) override;

  bool DidClipScreenshot() const override;

 private:
  // DevToolsAgentHostClient overrides
  void DispatchProtocolMessage(content::DevToolsAgentHost* host,
                               base::span<const uint8_t> message) override;
  void AgentHostClosed(content::DevToolsAgentHost* host) override;

  // Steps:
  void RequestPageLayoutMetrics();
  void OnLayoutMetricsReceived(int width, int height);
  void RequestFullPageScreenshot(int width, int height);

  // Utility functions
  void SendDevToolsCommand(const std::string& command,
                           base::Value::Dict params,
                           int command_id);
  void RunCallback(const image_editor::ScreenshotCaptureResult& result);

  image_editor::ScreenshotCaptureCallback callback_;
  base::WeakPtr<content::WebContents> web_contents_;
  scoped_refptr<content::DevToolsAgentHost> devtools_host_;
  std::optional<bool> screenshot_was_clipped_;
  int next_id_ = 1;
};

}  // namespace brave_screenshots

#endif  // BRAVE_BROWSER_BRAVE_SCREENSHOTS_STRATEGIES_FULLPAGE_STRATEGY_H_
