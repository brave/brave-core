// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_HISTORY_SEARCH_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_HISTORY_SEARCH_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "components/history_embeddings/core/history_embeddings_search.h"

class Profile;

namespace content {
class BrowserContext;
}  // namespace content

namespace ai_chat {

namespace internal {

// Serializes a history embeddings SearchResult into the JSON payload
// returned by HistorySearchTool. Exposed for unit tests.
std::string BuildHistorySearchResultJson(
    const std::string& query,
    const history_embeddings::SearchResult& result,
    bool include_all_passages);

}  // namespace internal

// Exposes Brave's local history semantic search (history embeddings) as a
// tool callable by the AI Chat assistant. The tool runs the user's natural
// language query against the on-device embeddings index and returns the
// matching pages as a JSON-encoded TextContentBlock.
class HistorySearchTool : public Tool {
 public:
  explicit HistorySearchTool(content::BrowserContext* browser_context);
  ~HistorySearchTool() override;

  HistorySearchTool(const HistorySearchTool&) = delete;
  HistorySearchTool& operator=(const HistorySearchTool&) = delete;

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
  // SearchResultCallback is a RepeatingCallback but with `skip_answering=true`
  // it's expected to fire once. `callback` is heap-allocated and owned by the
  // bound closure; the first invocation moves it out and runs the
  // UseToolCallback, any further invocations are no-ops.
  void OnSearchResult(UseToolCallback* callback,
                      bool include_all_passages,
                      history_embeddings::SearchResult result);

  raw_ptr<Profile> profile_;
  bool user_has_granted_permission_ = false;

  base::WeakPtrFactory<HistorySearchTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_HISTORY_SEARCH_TOOL_H_
