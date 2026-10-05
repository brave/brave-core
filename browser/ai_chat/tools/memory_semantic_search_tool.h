// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_MEMORY_SEMANTIC_SEARCH_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_MEMORY_SEMANTIC_SEARCH_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

namespace ai_chat {

class AIChatEmbeddingsService;

namespace internal {

std::string BuildMemorySearchResultJson(
    const std::string& query,
    const std::vector<std::string>& memories);

}  // namespace internal

// Lets the assistant search the memories the user asked it to keep, by
// meaning. Memories already go with every request that this tool is offered
// for, so it asks for no permission.
class MemorySemanticSearchTool : public Tool {
 public:
  explicit MemorySemanticSearchTool(
      base::WeakPtr<AIChatEmbeddingsService> service);
  ~MemorySemanticSearchTool() override;

  MemorySemanticSearchTool(const MemorySemanticSearchTool&) = delete;
  MemorySemanticSearchTool& operator=(const MemorySemanticSearchTool&) = delete;

  // Tool:
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::DictValue> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  bool IsSupportedByModel(const mojom::Model& model,
                          const ConversationCapabilitySet&
                              conversation_capabilities) const override;
  bool SupportsConversation(bool is_temporary,
                            bool has_untrusted_content,
                            const ConversationCapabilitySet&
                                conversation_capabilities) const override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  void OnSearchResults(UseToolCallback callback,
                       const std::string& query,
                       std::vector<std::string> memories);

  const base::WeakPtr<AIChatEmbeddingsService> service_;

  base::WeakPtrFactory<MemorySemanticSearchTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_MEMORY_SEMANTIC_SEARCH_TOOL_H_
