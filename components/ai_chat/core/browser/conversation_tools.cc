// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_tools.h"

#include <string>
#include <string_view>

#include "base/feature_list.h"
#include "base/no_destructor.h"
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

  // The aim of this description is to avoid these scenarios:
  // 1. Over-use of pre-answer choices to chop up the answer to sections
  // (impossible for the user to choose another option later) User: "What are
  // some tips for coaching 10U soccer when the kids are up to 3 years apart and
  // of very varying skill?" Assistant: "Here are some tips"
  //    { user_choice_tool, { "choices": [ "Do this", "Do that", "Explore tip
  //    3"]} }
  //
  // 2. Under-use when post-answer suggestions would be useful
  // User: "Explore dinosaurs"
  // Assistant: "[good long detailed answer with no further sub-topic
  // suggestions]"
  //
  // 3. Suggesting the user thank the assistant
  // Assistant: "[detailed answer]"
  //    { user_choice_tool, { "choices": [ "Explore topic XYZ", "I have a good
  //    understanding now - thank you!"]} }
  //
  std::string_view Description() const override {
    return R"(The user_choice_tool should be used only in the following scenarios:
1. Preference clarification: Used when you need the user to choose a preference in order to proceed with a long task, such as a programming language or other *value* preference. But *only* when the user would clearly only want one of those choices and *NOT* if they would likely want to explore multiple of them, like sections of your answer. It allows you to present the user with a limited set of relevant choices before providing a solution tailored to their selection. Use when there are different branches you could take and the choice can be easily defined as simple multiple choices. Don't provide an "Other" style choice - if the user wants to make a different choice they will type it to you in a new message. For example:
"From the available times, which would you like me to book?" followed by { "choices": [ "1pm", "2:30pm", "3:45pm"]} or
"Which programming language would you like that example in? Choose one, or type a different answer." followed by { "choices": ["Javascript", "Typescript", "Python", "Java", "C++"]}. Ensure any response you make before the choices makes sense for the choices you are presenting.
2. Suggestions: The second user_choice_tool scenario is to offer a few (or one) really enticing and relevant *follow-up* questions or suggestions when your initial response is complete but there are obvious next steps you and the user could explore related to the topic. The suggestions provided should be concise, directly relevant, and showcase your broader knowledge on the topic. They should be phrased as if the user has submitted their choice to you because if the user does submit a choice, you will act as if they have asked it directly. Do not talk as the assistant for these choices, but use the user's voice. Utilize this scenario if there are clear related sub-topics you could suggest for the user to explore. They should be restricted to a sentence or two as each suggestion will be displayed in a button. They should be explicit follow-ups and there should not be an option to decline follow-up or thank the assistant for their answer. In the Suggestions scenario, don't provide an introduction to the suggestions, just send them at the end of your answer to the user's previous message. Again, they are for follow-ups when your response is complete and not an excuse to not provide a full answer. For example:
User: "Tell me about dinosaurs"; Assistant: "[detailed answer]"
{ user_choice_tool, { "choices": [ "Explore dinosaur topic XYZ", "Explore dinosaur topic ABC" ]} }
The key is to avoid overusing the tool. It should supplement your responses, not replace your ability to have a natural conversation flow. *Do not* use it to hide multiple sections of your answer, since the user can only choose one option. Use it for exploring different kinds of answers, do *not* use it for making your answer cover less ground. At the same time we want to keep the user engaged and learning so you can use it when you have clear next steps to offer the user, but only after you've completed your full response.)";
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

// Encourages the LLM to store important information before it gets removed
// from context. Offer in situations where large tool output is expected in
// conversations with multiple rounds. Adjust the Description and
// SupportsConversation to reflect any other use cases.
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
    if (base::FeatureList::IsEnabled(features::kAIChatUserChoiceTool)) {
      static base::NoDestructor<UserChoiceTool> user_choice_tool;
      tools.push_back(user_choice_tool.get());
    }

    static base::NoDestructor<AssistantDetailStorageTool>
        assistant_detail_storage_tool;
    tools.push_back(assistant_detail_storage_tool.get());

    return tools;
  }());

  return *kTools;
}

}  // namespace

ConversationToolProvider::ConversationToolProvider(
    base::WeakPtr<Tool> memory_storage_tool)
    : memory_storage_tool_(memory_storage_tool) {}

ConversationToolProvider::~ConversationToolProvider() = default;

std::vector<base::WeakPtr<Tool>> ConversationToolProvider::GetTools() {
  std::vector<base::WeakPtr<Tool>> tools;
  for (Tool* tool : StaticTools()) {
    tools.push_back(tool->GetWeakPtr());
  }

  if (memory_storage_tool_) {
    tools.push_back(memory_storage_tool_);
  }

  return tools;
}

}  // namespace ai_chat
