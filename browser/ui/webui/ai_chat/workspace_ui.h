// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_WORKSPACE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_WORKSPACE_UI_H_

#include <memory>

#include "content/public/browser/webui_config.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace ai_chat {

class WorkspaceUIConfig : public content::WebUIConfig {
 public:
  WorkspaceUIConfig();
  ~WorkspaceUIConfig() override;

  // content::WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

// Hidden, headless Untrusted WebUI that hosts the Leo "workspace" tools. The
// page receives a FileSystemDirectoryHandle (delivered by the browser via
// launchQueue) for a user-picked folder, implements the file tools in
// JavaScript against it, and registers them with Leo via WebMCP
// (navigator.modelContext). One instance is created per conversation and served
// at chrome-untrusted://workspace/<guid>; it runs with a locked-down CSP that
// only permits its own first-party bundle.
class WorkspaceUI : public ui::UntrustedWebUIController {
 public:
  explicit WorkspaceUI(content::WebUI* web_ui);
  ~WorkspaceUI() override;

  WorkspaceUI(const WorkspaceUI&) = delete;
  WorkspaceUI& operator=(const WorkspaceUI&) = delete;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_WORKSPACE_UI_H_
