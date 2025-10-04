// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_CODE_EXECUTION_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_CODE_EXECUTION_TOOL_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
class BrowserContext;
}  // namespace content

namespace ai_chat {

class ConsoleMessageObserver;

// Tool for executing JavaScript code and returning console.log output.
// This tool is provided by the browser and allows AI assistants to run
// JavaScript code in a sandboxed environment.
class CodeExecutionTool : public Tool {
 public:
  explicit CodeExecutionTool(content::BrowserContext* browser_context);
  ~CodeExecutionTool() override;

  CodeExecutionTool(const CodeExecutionTool&) = delete;
  CodeExecutionTool& operator=(const CodeExecutionTool&) = delete;

  // Tool overrides
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::Value::Dict> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  bool RequiresUserInteractionBeforeHandling() const override;
  bool SupportsConversation(bool is_temporary,
                            bool has_untrusted_content) const override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

  // Public accessor for lambda access
  content::WebContents* web_contents() const { return web_contents_.get(); }

 private:
  void OnJavaScriptExecutionComplete(UseToolCallback callback,
                                     base::Value result);

  raw_ptr<content::BrowserContext> browser_context_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<ConsoleMessageObserver> console_observer_;
  base::WeakPtrFactory<CodeExecutionTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_CODE_EXECUTION_TOOL_H_
