// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/code_execution_tool.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/browser/code_sandbox.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/grit/brave_components_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace ai_chat {

namespace {

constexpr char kScriptProperty[] = "script";

}  // namespace

CodeExecutionTool::CodeExecutionTool(CodeSandbox* code_sandbox)
    : code_sandbox_(code_sandbox) {
  CHECK(code_sandbox_);
}

CodeExecutionTool::~CodeExecutionTool() = default;

std::string_view CodeExecutionTool::Name() const {
  return mojom::kCodeExecutionToolName;
}

std::string_view CodeExecutionTool::Description() const {
  return "Execute JavaScript code and capture console output. "
         "Use only when the task requires code execution for providing an "
         "accurate answer. "
         "Do not use this if you are able to answer without executing code. "
         "Do not use this for content generation. "
         "Do not use this for fetching information from the internet. "
         "Use console.log() to output results. "
         "The code will be executed in a sandboxed environment. "
         "Network requests are not allowed. "
         "bignumber.js is available in the global scope. Use it for any "
         "decimal math (i.e. financial calculations). "
         "Do not use require to import bignumber.js, as it is not needed.\n"
         "Example tasks that require code execution:\n"
         " - Financial calculations (e.g. compound interest)\n"
         " - Analyzing data or web content\n"
         "Example tasks that do not require code execution:\n"
         " - Very simple calculations (e.g. 2 + 2)\n"
         " - Finding the 4th prime number\n"
         " - Retrieving weather information for a location";
}

std::optional<base::Value::Dict> CodeExecutionTool::InputProperties() const {
  return CreateInputProperties(
      {{kScriptProperty, StringProperty("The JavaScript code to execute")}});
}

std::optional<std::vector<std::string>> CodeExecutionTool::RequiredProperties()
    const {
  return std::vector<std::string>{kScriptProperty};
}

std::variant<bool, mojom::PermissionChallengePtr>
CodeExecutionTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  return false;
}

bool CodeExecutionTool::SupportsConversation(
    bool is_temporary,
    bool has_untrusted_content,
    mojom::ConversationCapability conversation_capability) const {
  return true;
}

void CodeExecutionTool::UseTool(const std::string& input_json,
                                UseToolCallback callback) {
  auto input_dict = base::JSONReader::ReadDict(
      input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input_dict.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: Invalid JSON input, input must be a JSON object"));
    return;
  }

  const std::string* script = input_dict->FindString(kScriptProperty);

  if (!script || script->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Missing or empty 'script' field"));
    return;
  }

  // Prepend bignumber.js before the user's script
  auto bignumber_js =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_AI_CHAT_BIGNUMBER_JS);
  std::string prepared_script = base::StrCat({bignumber_js, " ", *script});

  code_sandbox_->ExecuteCode(
      prepared_script,
      base::BindOnce(&CodeExecutionTool::OnCodeExecuted,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CodeExecutionTool::OnCodeExecuted(UseToolCallback callback,
                                       std::string output) {
  std::move(callback).Run(CreateContentBlocksForText(std::move(output)));
}

}  // namespace ai_chat
