// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/navigation_tool.h"

#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/page_transition_types.h"

namespace ai_chat {

NavigationTool::NavigationTool(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), web_contents_(web_contents) {}

NavigationTool::~NavigationTool() = default;

std::string_view NavigationTool::name() const {
  return "web_page_navigator";
}

std::string_view NavigationTool::description() const {
  return "Navigate the current browser Tab's URL to a new page. Use this "
         "function to completely change the url to another page or website. "
         "The assistant should always take a screenshot after navigating.";
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

void NavigationTool::UseTool(const std::string& input_json,
                             Tool::UseToolCallback callback) {
  // Fail any pending requests
  if (pending_callback_) {
    std::move(pending_callback_).Run(CreateContentBlocksForText("error"));
    pending_navigation_url_ = GURL();
    pending_callback_.Reset();
  }
  // Parse the input JSON
  auto input_value = base::JSONReader::ReadDict(input_json);

  if (!input_value) {
    LOG(ERROR) << "Failed to parse input JSON: " << input_json;
    std::move(callback).Run(
        CreateContentBlocksForText("Error - unable to parse input JSON"));
    return;
  }

  // Get the website URL
  auto* website_url = input_value->FindString("website_url");
  if (!website_url) {
    LOG(ERROR) << "Missing required property 'website_url' in " << input_json;
    std::move(callback).Run(CreateContentBlocksForText(
        "Error - missing required property 'website_url'"));
    return;
  }

  // Navigate the web contents to the new URL
  GURL url = GURL(*website_url);
  web_contents_->GetController().LoadURL(
      url, content::Referrer(), ui::PAGE_TRANSITION_FROM_API, std::string());
  // Wait for navigation and paint
  pending_callback_ = std::move(callback);
  pending_navigation_url_ = url;
  navigation_complete_ = false;
  visually_painted_ = false;
}

void NavigationTool::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  LOG(ERROR) << __func__;
  if (pending_callback_.is_null() || !navigation_handle->IsInMainFrame()) {
    return;
  }

  navigation_complete_ = true;
  if (navigation_handle->IsSameDocument()) {
    // Same‐document navigation doesn’t trigger DidFirstVisuallyNonEmptyPaint
    visually_painted_ = true;
  }
  MaybeFinish();
}

void NavigationTool::DidFirstVisuallyNonEmptyPaint() {
  LOG(ERROR) << __func__;
  if (pending_callback_.is_null()) {
    return;
  }
  visually_painted_ = true;
  MaybeFinish();
}

void NavigationTool::MaybeFinish() {
  if (navigation_complete_ && visually_painted_ &&
      !pending_callback_.is_null()) {
    // TODO: pass screenshot
    std::move(pending_callback_)
        .Run(CreateContentBlocksForText("navigation success"));
  }
}

}  // namespace ai_chat
