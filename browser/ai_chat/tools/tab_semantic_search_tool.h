// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_SEMANTIC_SEARCH_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_SEMANTIC_SEARCH_TOOL_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "components/history/core/browser/history_types.h"
#include "url/gurl.h"

namespace history_embeddings {
struct SearchResult;
}

class Profile;

namespace ai_chat {

namespace internal {

// Minimal description of an open tab passed into the result-building helper
// below. Exposed for tests.
struct SemanticSearchTabInfo {
  int32_t tab_id = 0;
  std::string title;
  GURL url;
  history::URLID url_id = 0;
};

// Maps a `SearchResult` back to open-tab info by `url_id` and emits the
// tool's JSON response, capped at `count` items. Exposed (and not stuffed in
// an anonymous namespace inside the .cc) so unit tests can exercise the
// mapping without standing up a full HistoryEmbeddingsService.
std::string BuildSemanticSearchResultsJson(
    const std::vector<SemanticSearchTabInfo>& tabs,
    const history_embeddings::SearchResult& result,
    size_t count);

// Same mapping, but serialized as a `kTabSourcesArtifactType` artifact
// payload for the chat frontend to render as clickable cards. Returns the
// empty string when no results map to open tabs.
std::string BuildSemanticSearchTabSourcesJson(
    const std::vector<SemanticSearchTabInfo>& tabs,
    const history_embeddings::SearchResult& result,
    size_t count);

}  // namespace internal

// Leo tool that semantically searches the user's currently-open tabs. Calls
// `HistoryEmbeddingsService::Search()` and filters the ranked results to the
// set of currently-open HTTP(S) tab URLs. Fully on-device: the query and full
// page content stay local; only matched tabs' title/URL/best-snippet leave.
class TabSemanticSearchTool : public Tool {
 public:
  explicit TabSemanticSearchTool(Profile* profile);
  ~TabSemanticSearchTool() override;

  TabSemanticSearchTool(const TabSemanticSearchTool&) = delete;
  TabSemanticSearchTool& operator=(const TabSemanticSearchTool&) = delete;

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
  raw_ptr<Profile> profile_;
  bool user_has_granted_permission_ = false;

  // Tracks per-tab HistoryService::QueryURL calls issued by UseTool().
  // Destruction cancels any in-flight resolution so a stale invocation can't
  // fire its embeddings query after the tool is gone.
  base::CancelableTaskTracker query_url_task_tracker_;

  base::WeakPtrFactory<TabSemanticSearchTool> weak_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_SEMANTIC_SEARCH_TOOL_H_
