// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TEXT_FILTER_GENERATION_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TEXT_FILTER_GENERATION_TOOL_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

namespace ai_chat {

// Tool that generates custom cosmetic filters (CSS selectors or JavaScript
// scriptlets) to hide/remove page elements based on user descriptions.
//
// Example usage:
//   User: "Hide the cookie banner"
//   Tool input: {"target_description": "cookie banner"}
//   Tool output: GeneratedFilterEvent with filter code and metadata
class TextFilterGenerationTool : public Tool {
 public:
  explicit TextFilterGenerationTool(
      base::WeakPtr<AssociatedContentDelegate> content);
  ~TextFilterGenerationTool() override;

  TextFilterGenerationTool(const TextFilterGenerationTool&) = delete;
  TextFilterGenerationTool& operator=(const TextFilterGenerationTool&) = delete;

  // Tool implementation
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::Value::Dict> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  bool SupportsConversation(
      bool is_temporary,
      bool has_untrusted_content,
      mojom::ConversationCapability conversation_capability) const override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  void OnContentFetched(const std::string& input_json,
                        UseToolCallback callback,
                        PageContent content);

  base::WeakPtr<AssociatedContentDelegate> content_;
  base::WeakPtrFactory<TextFilterGenerationTool> weak_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TEXT_FILTER_GENERATION_TOOL_H_
