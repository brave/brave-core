// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_NAVIGATION_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_NAVIGATION_TOOL_H_

#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"

namespace ai_chat {

class NavigationTool : public Tool, public content::WebContentsObserver {
 public:
  explicit NavigationTool(content::WebContents* web_contents);
  ~NavigationTool() override;

  NavigationTool(const NavigationTool&) = delete;
  NavigationTool& operator=(const NavigationTool&) = delete;

  // Tool:
  std::string_view name() const override;
  std::string_view description() const override;
  std::optional<std::string> GetInputSchemaJson() const override;
  std::optional<std::vector<std::string>> required_properties() const override;
  bool IsContentAssociationRequired() const override;
  bool RequiresUserInteractionBeforeHandling() const override;
  // Should only process one action at a time. Will cause any Tool use request
  // still in progress (waiting for navigation to complete) to be called
  // with an error message.
  void UseTool(const std::string& input_json,
               Tool::UseToolCallback callback) override;

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFirstVisuallyNonEmptyPaint() override;

 private:
  void MaybeFinish();

  raw_ptr<content::WebContents> web_contents_;

  // Store the pending UseTool request whilst waiting for navigation
  // to complete
  GURL pending_navigation_url_;
  Tool::UseToolCallback pending_callback_;
  // Track if the navigation has completed
  bool navigation_complete_ = false;
  bool visually_painted_ = false;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_NAVIGATION_TOOL_H_
