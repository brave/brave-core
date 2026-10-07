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
struct MemorySearchResult;

namespace internal {

// `memories` holds the memories that the user asked Leo to keep, and
// `learned_memories` the ones that Leo learned from past chats. The key of an
// empty list is left out.
std::string BuildMemorySearchResultJson(
    const std::string& query,
    const std::vector<std::string>& memories,
    const std::vector<std::string>& learned_memories = {});

}  // namespace internal

// Lets the assistant search by meaning the memories the user asked it to keep,
// and the ones it learned from past chats. The memories the user asked it to
// keep already go with every request that this tool is offered for, and the
// learned ones that fit a request go with it too, so it asks for no
// permission.
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
                       std::vector<MemorySearchResult> results);

  const base::WeakPtr<AIChatEmbeddingsService> service_;

  base::WeakPtrFactory<MemorySemanticSearchTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_MEMORY_SEMANTIC_SEARCH_TOOL_H_
