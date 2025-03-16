// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/navigate_history_tool.h"

#include "base/json/json_reader.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"

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
  // Fail any pending requests
  if (pending_callback_) {
    std::move(pending_callback_).Run(CreateContentBlocksForText("error"));
    pending_callback_ = base::NullCallback();
  }
  // Parse the input JSON
  auto input_value = base::JSONReader::ReadDict(input_json);

  if (!input_value) {
    DVLOG(0) << "Failed to parse input JSON: " << input_json;
    std::move(callback).Run(
        CreateContentBlocksForText("Error - failed to parse input JSON"));
    return;
  }

  // Get the website URL
  auto is_back = input_value->FindBool("back");
  if (!is_back.has_value()) {
    DVLOG(0) << "Missing required property 'back' in " << input_json;
    std::move(callback).Run(
        CreateContentBlocksForText("Error - 'back' property missing"));
    return;
  }

  // Navigate the web contents to the new URL
  if (is_back.value()) {
    if (!web_contents_->GetController().CanGoBack()) {
      std::move(callback).Run(
          CreateContentBlocksForText("Error - cannot navigate back"));
      return;
    }
    web_contents_->GetController().GoBack();
  } else {
    if (!web_contents_->GetController().CanGoForward()) {
      std::move(callback).Run(
          CreateContentBlocksForText("Error - cannot navigate forward"));
      return;
    }
    web_contents_->GetController().GoForward();
  }
  // Wait for navigation and paint
  pending_callback_ = std::move(callback);
  navigation_complete_ = false;
  visually_painted_ = false;
}

void NavigateHistoryTool::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
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

void NavigateHistoryTool::DidFirstVisuallyNonEmptyPaint() {
  if (pending_callback_.is_null()) {
    return;
  }
  visually_painted_ = true;
  MaybeFinish();
}

void NavigateHistoryTool::MaybeFinish() {
  if (navigation_complete_ && visually_painted_ &&
      !pending_callback_.is_null()) {
    // TODO: pass screenshot
    std::move(pending_callback_)
        .Run(CreateContentBlocksForText("navigation success"));
  }
}

}  // namespace ai_chat
