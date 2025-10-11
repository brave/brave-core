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
    return "Presents a list of text choices to the user and returns the "
           "user's "
           "selection. The assistant will call this function only when it "
           "needs "
           "the user to make a choice between a list of a couple options in "
           "order to "
           "move forward with a task.";
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

const std::vector<Tool*>& StaticTools() {
  static const base::NoDestructor<std::vector<Tool*>> kTools([] {
    std::vector<Tool*> tools;
    if (features::IsToolsEnabled()) {
      static base::NoDestructor<UserChoiceTool> user_choice_tool;
      tools.push_back(user_choice_tool.get());
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
