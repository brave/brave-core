// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_CONVERSATION_SEARCH_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_CONVERSATION_SEARCH_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

namespace ai_chat {

class AIChatEmbeddingsService;
struct ConversationSearchResult;

namespace internal {

std::string BuildConversationSearchResultJson(
    const std::string& query,
    const std::vector<ConversationSearchResult>& results);

// The JSON of the conversations the user is shown as cards, one for each
// result.
std::string BuildConversationSearchArtifactJson(
    const std::vector<ConversationSearchResult>& results);

}  // namespace internal

// Lets the assistant search the user's past conversations semantically. The
// search runs on device, but the matching passages are sent to the model as
// the tool's output, so the user is asked for permission first. The
// conversation the tool is used in is left out of its results.
class ConversationSearchTool : public Tool {
 public:
  ConversationSearchTool(base::WeakPtr<AIChatEmbeddingsService> service,
                         std::string conversation_uuid);
  ~ConversationSearchTool() override;

  ConversationSearchTool(const ConversationSearchTool&) = delete;
  ConversationSearchTool& operator=(const ConversationSearchTool&) = delete;

  // Tool:
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::DictValue> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  std::variant<bool, mojom::PermissionChallengePtr>
  RequiresUserInteractionBeforeHandling(
      const mojom::ToolUseEvent& tool_use) const override;
  void UserPermissionGranted(const std::string& tool_use_id) override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  void OnSearchResults(UseToolCallback callback,
                       const std::string& query,
                       std::vector<ConversationSearchResult> results);

  const base::WeakPtr<AIChatEmbeddingsService> service_;
  const std::string conversation_uuid_;
  bool user_has_granted_permission_ = false;

  base::WeakPtrFactory<ConversationSearchTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_CONVERSATION_SEARCH_TOOL_H_
