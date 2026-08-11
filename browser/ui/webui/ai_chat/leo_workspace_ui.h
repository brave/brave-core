// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_UI_H_

#include <memory>

#include "content/public/browser/webui_config.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace ai_chat {

class LeoWorkspaceUIConfig : public content::WebUIConfig {
 public:
  LeoWorkspaceUIConfig();
  ~LeoWorkspaceUIConfig() override;

  // content::WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

// Hidden, headless Untrusted WebUI that will host the Leo "workspace" tools.
// One instance is created per conversation and served at
// chrome-untrusted://leo-workspace/<guid>, with a locked-down CSP that only
// permits its own first-party bundle. This page has no visible UI.
//
// At this stage the page only serves a placeholder document. The file tools
// bundle and the FileSystemDirectoryHandle plumbing are added separately.
class LeoWorkspaceUI : public ui::UntrustedWebUIController {
 public:
  explicit LeoWorkspaceUI(content::WebUI* web_ui);
  ~LeoWorkspaceUI() override;

  LeoWorkspaceUI(const LeoWorkspaceUI&) = delete;
  LeoWorkspaceUI& operator=(const LeoWorkspaceUI&) = delete;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_UI_H_
