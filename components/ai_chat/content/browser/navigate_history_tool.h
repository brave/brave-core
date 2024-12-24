// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_NAVIGATE_HISTORY_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_NAVIGATE_HISTORY_TOOL_H_

#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

class NavigateHistoryTool : public Tool {
 public:
  explicit NavigateHistoryTool(content::WebContents* web_contents);
  ~NavigateHistoryTool() override;

  NavigateHistoryTool(const NavigateHistoryTool&) = delete;
  NavigateHistoryTool& operator=(const NavigateHistoryTool&) = delete;

  // Tool:
  std::string_view name() const override;
  std::string_view description() const override;
  std::optional<std::string> GetInputSchemaJson() const override;
  std::optional<std::vector<std::string>> required_properties() const override;
  bool IsContentAssociationRequired() const override;
  bool RequiresUserInteractionBeforeHandling() const override;

  void UseTool(const std::string& input_json,
               Tool::UseToolCallback callback) override;

 private:
  raw_ptr<content::WebContents> web_contents_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_NAVIGATE_HISTORY_TOOL_H_
