// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/navigation_tool.h"
#include "base/json/json_reader.h"
#include "ui/base/page_transition_types.h"

namespace ai_chat {

NavigationTool::NavigationTool(content::WebContents* web_contents)
    : web_contents_(web_contents) {}

NavigationTool::~NavigationTool() = default;

std::string_view NavigationTool::name() const {
  return "web_page_navigator";
}

std::string_view NavigationTool::description() const {
  return "Navigate the current browser Tab's URL to a new page. Use this "
         "function to completely change the url to another page or website.";
}

std::optional<std::string> NavigationTool::GetInputSchemaJson() const {
  return R"({
        "type": "object",
        "properties": {
          "website_url": {
            "type": "string",
            "description": "The full website URL to navigate to, starting with https://"
          }
        }
      })";
}

std::optional<std::vector<std::string>> NavigationTool::required_properties()
    const {
  return std::optional<std::vector<std::string>>{{"website_url"}};
}

bool NavigationTool::IsContentAssociationRequired() const {
  return true;
}

bool NavigationTool::RequiresUserInteractionBeforeHandling() const {
  return false;
}

void NavigationTool::UseTool(
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
  auto* website_url = input_value->FindString("website_url");
  if (!website_url) {
    LOG(ERROR) << "Missing required property 'website_url' in " << input_json;
    std::move(callback).Run(std::nullopt, 0);
    return;
  }

  // Navigate the web contents to the new URL
  web_contents_->GetController().LoadURL(GURL(*website_url),
                                         content::Referrer(),
                                         ui::PAGE_TRANSITION_FROM_API,
                                         std::string());
  std::move(callback).Run(R"({
    "status": "navigated"
  })", 4000); // Allow time for the navigation to at least partially complete
}

}  // namespace ai_chat
