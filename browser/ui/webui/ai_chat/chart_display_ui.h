// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_CHART_DISPLAY_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_CHART_DISPLAY_UI_H_

#include <memory>

#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/webui_config.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace ai_chat {

class ChartDisplayUIConfig : public content::WebUIConfig {
 public:
  ChartDisplayUIConfig();
  ~ChartDisplayUIConfig() override;

  // content::WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

// This Untrusted WebUI provides a sandboxed environment for displaying
// charts. It runs with restricted content security policies.
class ChartDisplayUI : public ui::UntrustedWebUIController {
 public:
  explicit ChartDisplayUI(content::WebUI* web_ui);
  ~ChartDisplayUI() override;

  ChartDisplayUI(const ChartDisplayUI&) = delete;
  ChartDisplayUI& operator=(const ChartDisplayUI&) = delete;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_CHART_DISPLAY_UI_H_
