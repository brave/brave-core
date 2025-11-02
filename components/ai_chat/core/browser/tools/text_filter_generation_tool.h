// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TEXT_FILTER_GENERATION_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TEXT_FILTER_GENERATION_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

namespace ai_chat {

class AssociatedContentManager;

// Tool for generating cosmetic filters (CSS selectors or scriptlets) to hide
// page elements. Uses DOM structure analysis to create filters based on user
// requests like "hide the cookie banner" or "block the sidebar".
//
// This tool analyzes the page's DOM structure (text-based) to identify
// elements and generate appropriate CSS selectors or JavaScript scriptlets.
class TextFilterGenerationTool : public Tool {
 public:
  explicit TextFilterGenerationTool(
      AssociatedContentManager* associated_content_manager);
  ~TextFilterGenerationTool() override;

  TextFilterGenerationTool(const TextFilterGenerationTool&) = delete;
  TextFilterGenerationTool& operator=(const TextFilterGenerationTool&) = delete;

  // Tool overrides
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::Value::Dict> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  bool IsSupportedByModel(const mojom::Model& model) const override;
  bool SupportsConversation(
      bool is_temporary,
      bool has_untrusted_content,
      mojom::ConversationCapability conversation_capability) const override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  raw_ptr<AssociatedContentManager> associated_content_manager_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TEXT_FILTER_GENERATION_TOOL_H_
