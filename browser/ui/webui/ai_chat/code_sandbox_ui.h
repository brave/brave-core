// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_CODE_SANDBOX_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_CODE_SANDBOX_UI_H_

#include <memory>
#include <string>

#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/webui_config.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace ai_chat {

class CodeSandboxUIConfig : public content::WebUIConfig {
 public:
  CodeSandboxUIConfig();
  ~CodeSandboxUIConfig() override;

  // content::WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

// This Untrusted WebUI provides a sandboxed environment for executing
// JavaScript code. It runs with no WebUI bindings and strict content security
// policies.
class CodeSandboxUI : public ui::UntrustedWebUIController {
 public:
  explicit CodeSandboxUI(content::WebUI* web_ui);
  ~CodeSandboxUI() override;

  CodeSandboxUI(const CodeSandboxUI&) = delete;
  CodeSandboxUI& operator=(const CodeSandboxUI&) = delete;

 private:
  static bool ShouldHandleRequest(const std::string& path);
  static void HandleRequest(
      base::WeakPtr<content::BrowserContext> browser_context,
      const std::string& path,
      content::WebUIDataSource::GotDataCallback callback);
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_CODE_SANDBOX_UI_H_
