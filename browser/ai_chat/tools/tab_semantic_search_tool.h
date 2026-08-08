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
#include "base/task/cancelable_task_tracker.h"
#include "brave/browser/history_embeddings/open_tab_search.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

class Profile;

namespace ai_chat {

namespace internal {

// Serializes the ranked `tabs` returned by
// `history_embeddings::SearchOpenTabsByContent` as the tool's JSON response.
// Exposed for unit testing.
std::string BuildSemanticSearchResultsJson(
    const std::vector<history_embeddings::OpenTabInfo>& tabs);

// Same ranked `tabs` serialized as a `kTabSourcesArtifactType` artifact
// payload for the chat frontend to render as clickable cards. Returns empty
// when there are no ranked tabs.
std::string BuildSemanticSearchTabSourcesJson(
    const std::vector<history_embeddings::OpenTabInfo>& tabs);

}  // namespace internal

// Leo tool that semantically searches the user's currently-open tabs. Calls
// `history_embeddings::SearchOpenTabsByContent()` and returns matched tabs'
// metadata to the model. Fully on-device: the query and full page content
// stay local; only matched tabs' titles and URLs are returned.
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

  void SetEmbeddingsSearchForTesting(
      history_embeddings::HistoryEmbeddingsSearch* embeddings_search) {
    embeddings_search_for_testing_ = embeddings_search;
  }

 private:
  raw_ptr<Profile> profile_;
  bool user_has_granted_permission_ = false;

  // When set, used instead of the profile's HistoryEmbeddingsService so tests
  // can supply canned scored rows without a real embeddings model.
  raw_ptr<history_embeddings::HistoryEmbeddingsSearch>
      embeddings_search_for_testing_ = nullptr;

  // Tracks the HistoryService::QueryUrlIds call issued via
  // `SearchOpenTabsByContent`. Destruction cancels any in-flight resolution so
  // a stale invocation can't fire its embeddings query after the tool is
  // gone.
  base::CancelableTaskTracker query_url_task_tracker_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_TAB_SEMANTIC_SEARCH_TOOL_H_
