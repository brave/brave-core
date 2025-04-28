// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_tools.h"

#include <string>
#include <string_view>
#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace ai_chat {

namespace {

// ai_chat component-level tools
class PageContentTool : public Tool {
  public:
   ~PageContentTool() override = default;

   std::string_view Name() const override { return mojom::kPageContentToolName; }
   std::string_view Description() const override {
     return "Fetches the text content of the active Tab in the user's current "
            "browser session that is open alongside this conversation. This "
            "web page may or may not be relevant to the user's question. The "
            "assistant will call this function when determining that the "
            "user's question could be related to the content they are looking "
            "at is not a standalone question.  The assistant should only "
            "query this when it is at last 80\% sure the user's query is "
            "related to the web page content.";
   }

   std::optional<std::string> InputProperties() const override {
     return R"({
         "type": "object",
         "properties": {
           "confidence_percent": {
             "type": "number",
             "description": "How confident the assistant is that it needs to content of the active web page to answer the user's query, where 100 is that the user's query is definitely related to the content and 0 is that it is definitely not related to the query."
           }
         }
       })";
   }

   bool IsContentAssociationRequired() const override { return true; }

   bool RequiresUserInteractionBeforeHandling() const override { return true; }
 };

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

   std::optional<std::string> InputProperties() const override {
     return R"({
         "type": "object",
         "properties": {
           "choices": {
             "type": "array",
             "description": "A list of choices for the user to select from",
             "items": {
               "type": "string"
             }
           }
         }
       })";
   }

   std::optional<std::vector<std::string>> RequiredProperties() const override {
     return std::optional<std::vector<std::string>>({"choices"});
   }

   bool IsContentAssociationRequired() const override { return false; }

   bool RequiresUserInteractionBeforeHandling() const override { return true; }
 };

 const std::vector<Tool*>& AllTools() {
  static const base::NoDestructor<std::vector<Tool*>> kTools([]{
    std::vector<Tool*> tools;
    if (features::IsAIChatToolsEnabled()) {
      static base::NoDestructor<UserChoiceTool> user_choice_tool;
      tools.push_back(user_choice_tool.get());

      if (base::FeatureList::IsEnabled(features::kSmartPageContent)) {
        static base::NoDestructor<PageContentTool> page_content_tool;
        tools.push_back(page_content_tool.get());
      }
    }
    return tools;
  }());

  return *kTools;
}

const std::vector<Tool*> GetToolsForConversation(bool has_associated_content,
                                                  const mojom::Model& model) {
  if (!features::IsAIChatToolsEnabled()) {
    return {};
  }
  // Filter AllTools based on arguments
  std::vector<Tool*> filtered_tools;
  for (const auto& tool : AllTools()) {
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

} // namespace




}  // namespace ai_chat