// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_CODE_EXECUTION_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_CODE_EXECUTION_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

namespace ai_chat {

class CodeSandbox;

// Tool for executing JavaScript code and returning console.log output.
// This tool is provided by the browser and allows AI assistants to run
// JavaScript code in a sandboxed environment.
class CodeExecutionTool : public Tool {
 public:
  explicit CodeExecutionTool(CodeSandbox* code_sandbox);
  ~CodeExecutionTool() override;

  CodeExecutionTool(const CodeExecutionTool&) = delete;
  CodeExecutionTool& operator=(const CodeExecutionTool&) = delete;

  // Tool overrides
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::Value::Dict> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  std::variant<bool, mojom::PermissionChallengePtr>
  RequiresUserInteractionBeforeHandling(
      const mojom::ToolUseEvent& tool_use) const override;
  bool SupportsConversation(
      bool is_temporary,
      bool has_untrusted_content,
      mojom::ConversationCapability conversation_capability) const override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  void OnCodeExecuted(UseToolCallback callback, std::string output);

  raw_ptr<CodeSandbox> code_sandbox_;
  base::WeakPtrFactory<CodeExecutionTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_CODE_EXECUTION_TOOL_H_
