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
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

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
1. Preference clarification: Used when you need the user to choose a preference in order to proceed with your answer or task, such as a programming language or other *value* preference. But *only* when the user would clearly only want one of those choices and *NOT* if they would likely want to explore multiple of them, like sections of your answer. It allows you to present the user with a limited set of relevant choices before providing a solution tailored to their selection. Use when there are different branches you could take and the choice can be easily defined as simple multiple choices. Don't provide an "Other" style choice - if the user wants to make a different choice they will type it to you in a new message. For example:
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

  bool IsContentAssociationRequired() const override { return false; }

  bool RequiresUserInteractionBeforeHandling() const override { return true; }
};

const std::vector<Tool*>& AllTools() {
  static const base::NoDestructor<std::vector<Tool*>> kTools([] {
    std::vector<Tool*> tools;
    if (base::FeatureList::IsEnabled(features::kAIChatUserChoiceTool)) {
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
    : memory_storage_tool_(memory_storage_tool) {}

ConversationToolProvider::~ConversationToolProvider() = default;

std::vector<base::WeakPtr<Tool>> ConversationToolProvider::GetTools() {
  std::vector<base::WeakPtr<Tool>> tools;
  for (Tool* tool : AllTools()) {
    tools.push_back(tool->GetWeakPtr());
  }

  if (memory_storage_tool_) {
    tools.push_back(memory_storage_tool_);
  }

  return tools;
}

}  // namespace ai_chat
