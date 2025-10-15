// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_tools.h"

#include <string>
#include <string_view>

#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/browser/tools/todo_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

namespace {

// ai_chat component-level tools
class UserChoiceTool : public Tool {
 public:
  ~UserChoiceTool() override = default;

  std::string_view Name() const override { return mojom::kUserChoiceToolName; }
  std::string_view Description() const override {
    return "Presents a list of text choices to the user and returns the user's "
           "selection. The assistant will call this function only when it "
           "needs the user to make a choice between a list of a couple options "
           "in order to move forward with a task.";
  }

  std::optional<base::Value::Dict> InputProperties() const override {
    return CreateInputProperties(
        {{"choices",
          ArrayProperty(
              "A list of choices for the user to select from",
              StringProperty("Text of the choice which will be "
                             "displayed by the user for selection"))}});
  }

  std::optional<std::vector<std::string>> RequiredProperties() const override {
    return std::optional<std::vector<std::string>>({"choices"});
  }

  bool RequiresUserInteractionBeforeHandling() const override { return true; }
};

class AssistantDetailStorageTool : public Tool {
 public:
  // static name
  inline static const std::string_view kName =
      mojom::kAssistantDetailStorageToolName;

  ~AssistantDetailStorageTool() override = default;

  std::string_view Name() const override { return kName; }
  std::string_view Description() const override {
    return "This tool allows the assistant to preserve important information "
           "from large web content before it gets pushed out of "
           "context. The assistant should proactively use this tool "
           "before performing additional actions on the content which will "
           "force any content apart from the 2 most recent page content tool "
           "responses to be removed from the conversation. It should only be "
           "used if there's valuable information neccessary to complete the "
           "task or provide the information the user has requested. "
           "By storing key details, "
           "observations, or data points from page content, the assistant "
           "can reference this information later in the conversation even if "
           "the original web content is no longer in context. This is "
           "particularly important for multi-step tasks where earlier "
           "context contains critical information needed for later steps. "
           "Actions like scrolling, navigating, or clicking will result in an "
           "additional "
           "large web content result and anything before the latest 2 results "
           "being removed "
           "from "
           "context, so it's important to use this tool when any valuable "
           "information is gleamed from a web content output.";
  }

  std::optional<base::Value::Dict> InputProperties() const override {
    return CreateInputProperties(
        {{"information",
          StringProperty(
              "Useful information from an immediately-previous tool call")}});
  }

  bool RequiresUserInteractionBeforeHandling() const override { return false; }

  bool SupportsConversation(
      bool is_temporary,
      bool has_untrusted_content,
      mojom::ConversationCapability conversation_capability) const override {
    // This tool is only useful for multi-step agentic tasks especially when
    // other tools might have their output truncated from the context.
    return conversation_capability ==
           mojom::ConversationCapability::CONTENT_AGENT;
  }

  void UseTool(const std::string& input_json,
               Tool::UseToolCallback callback) override {
    std::move(callback).Run(CreateContentBlocksForText(
        "Look at the function input for the information the assistant needed "
        "to remember"));
  }
};

const std::vector<Tool*>& StaticTools() {
  static const base::NoDestructor<std::vector<Tool*>> kTools([] {
    std::vector<Tool*> tools;
    if (features::IsToolsEnabled()) {
      static base::NoDestructor<UserChoiceTool> user_choice_tool;
      tools.push_back(user_choice_tool.get());

      static base::NoDestructor<AssistantDetailStorageTool>
          assistant_detail_storage_tool;
      tools.push_back(assistant_detail_storage_tool.get());
    }
    return tools;
  }());

  return *kTools;
}

}  // namespace

ConversationToolProvider::ConversationToolProvider(
    base::WeakPtr<Tool> memory_storage_tool)
    : memory_storage_tool_(memory_storage_tool) {
  todo_tool_ = std::make_unique<TodoTool>();
}

ConversationToolProvider::~ConversationToolProvider() = default;

void ConversationToolProvider::OnNewGenerationLoop() {
  todo_tool_ = std::make_unique<TodoTool>();
}

std::vector<base::WeakPtr<Tool>> ConversationToolProvider::GetTools() {
  std::vector<base::WeakPtr<Tool>> tools;
  for (Tool* tool : StaticTools()) {
    tools.push_back(tool->GetWeakPtr());
  }

  tools.push_back(todo_tool_->GetWeakPtr());

  if (memory_storage_tool_) {
    tools.push_back(memory_storage_tool_);
  }

  return tools;
}

}  // namespace ai_chat
