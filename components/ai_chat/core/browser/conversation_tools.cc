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

  bool IsContentAssociationRequired() const override { return false; }

  bool RequiresUserInteractionBeforeHandling() const override { return true; }
};

const std::vector<Tool*>& AllTools() {
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

const std::vector<Tool*> GetToolsForConversation(bool has_associated_content,
                                                 const mojom::Model& model) {
  if (!features::IsToolsEnabled()) {
    return {};
  }
  // Filter AllTools based on arguments
  std::vector<Tool*> filtered_tools;
  for (Tool* tool : AllTools()) {
    if (tool->IsContentAssociationRequired() && !has_associated_content) {
      continue;
    }
    if (!tool->IsSupportedByModel(model)) {
      continue;
    }
    filtered_tools.push_back(tool);
  }
  return filtered_tools;
}

}  // namespace ai_chat
