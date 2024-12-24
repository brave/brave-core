// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/navigate_history_tool.h"

#include "base/json/json_reader.h"
#include "ui/base/page_transition_types.h"

namespace ai_chat {

NavigateHistoryTool::NavigateHistoryTool(content::WebContents* web_contents)
    : web_contents_(web_contents) {}

NavigateHistoryTool::~NavigateHistoryTool() = default;

std::string_view NavigateHistoryTool::name() const {
  return "web_page_history_navigator";
}

std::string_view NavigateHistoryTool::description() const {
  return "Go back or forward a single entry in the current browser Tab's "
         "history. This is preferred over using a keyboard shortcut for the "
         "action as there are platform differences with keyboard shortcuts. "
         "It's important to take a screenshot after navigating to verify that "
         "previous clicks didn't create an unknown amount of history entries.";
}

std::optional<std::string> NavigateHistoryTool::GetInputSchemaJson() const {
  return R"({
        "type": "object",
        "properties": {
          "back": {
            "type": "boolean",
            "description": "True to navigate back one entry in the history, false to navigate forward one entry"
          }
        }
      })";
}

std::optional<std::vector<std::string>> NavigateHistoryTool::required_properties()
    const {
  return std::optional<std::vector<std::string>>{{"back"}};
}

bool NavigateHistoryTool::IsContentAssociationRequired() const {
  return true;
}

bool NavigateHistoryTool::RequiresUserInteractionBeforeHandling() const {
  return false;
}

void NavigateHistoryTool::UseTool(
    const std::string& input_json,
    Tool::UseToolCallback callback) {
  // Parse the input JSON
  auto input_value = base::JSONReader::ReadDict(input_json);

  if (!input_value) {
    LOG(ERROR) << "Failed to parse input JSON: " << input_json;
    std::move(callback).Run(std::nullopt, 0);
    return;
  }

  // Get the website URL
  auto is_back = input_value->FindBool("back");
  if (!is_back.has_value()) {
    LOG(ERROR) << "Missing required property 'back' in " << input_json;
    std::move(callback).Run(R"({ "status": "'back' property missing" })", 0);
    return;
  }

  // Navigate the web contents to the new URL
  web_contents_->GetController().GoToOffset(is_back.value() ? -1 : 1);
  std::move(callback).Run(R"({
    "status": "navigated"
  })", 2000); // Allow time for the navigation to at least partially complete
}

}  // namespace ai_chat
