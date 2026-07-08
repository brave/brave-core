// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_semantic_search_tool.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/history_embeddings/open_tab_search.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/history_embeddings/content/history_embeddings_service.h"
#include "components/history_embeddings/core/history_embeddings_search.h"
#include "components/keyed_service/core/service_access_type.h"

namespace ai_chat {

namespace {

std::string EmptyResultsJson() {
  base::DictValue root;
  root.Set("results", base::ListValue());
  std::string serialized;
  base::JSONWriter::Write(root, &serialized);
  return serialized;
}

void OnRanked(Tool::UseToolCallback callback,
              std::vector<history_embeddings::OpenTabInfo> tabs) {
  if (tabs.empty()) {
    std::move(callback).Run(CreateContentBlocksForText(EmptyResultsJson()), {});
    return;
  }
  std::string serialized = internal::BuildSemanticSearchResultsJson(tabs);
  std::move(callback).Run(CreateContentBlocksForText(serialized), {});
}

}  // namespace

namespace internal {

std::string BuildSemanticSearchResultsJson(
    const std::vector<history_embeddings::OpenTabInfo>& tabs) {
  base::ListValue results_list;
  for (const auto& tab : tabs) {
    base::DictValue entry;
    entry.Set("tab_id", tab.tab_id);
    entry.Set("title", tab.title);
    entry.Set("url", tab.url.spec());
    results_list.Append(std::move(entry));
  }
  base::DictValue root;
  root.Set("results", std::move(results_list));
  std::string serialized;
  base::JSONWriter::Write(root, &serialized);
  return serialized;
}

}  // namespace internal

TabSemanticSearchTool::TabSemanticSearchTool(Profile* profile)
    : profile_(profile) {}

TabSemanticSearchTool::~TabSemanticSearchTool() = default;

std::string_view TabSemanticSearchTool::Name() const {
  return mojom::kSemanticTabSearchToolName;
}

std::string_view TabSemanticSearchTool::Description() const {
  return "Semantically search the user's currently-open browser tabs by page "
         "content. Use this when the user asks to find one of their open tabs "
         "by what it contains (e.g. \"the tab about react hooks\"), not just "
         "by title or URL. The query and full page content stay on device; "
         "only matched tabs' titles and URLs are returned. Provide a "
         "natural-language query.";
}

std::optional<base::DictValue> TabSemanticSearchTool::InputProperties() const {
  return CreateInputProperties(
      {{"query",
        StringProperty(
            "Natural-language query describing the tab to find by content.")}});
}

std::optional<std::vector<std::string>>
TabSemanticSearchTool::RequiredProperties() const {
  return std::vector<std::string>{"query"};
}

std::variant<bool, mojom::PermissionChallengePtr>
TabSemanticSearchTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  if (user_has_granted_permission_) {
    return false;
  }
  // The search runs on-device against the local embeddings index over the
  // user's open tabs; only matched tabs' title and URL are sent to the remote
  // LLM as this tool's output. User-facing wording lives in
  // `get_tool_permission_implications.tsx` so it goes through i18n; this
  // C++ side only needs to surface a non-null challenge.
  return mojom::PermissionChallenge::New(/*assessment=*/std::nullopt,
                                         /*plan=*/std::nullopt);
}

void TabSemanticSearchTool::UserPermissionGranted(
    const std::string& tool_use_id) {
  user_has_granted_permission_ = true;
}

void TabSemanticSearchTool::UseTool(const std::string& input_json,
                                    UseToolCallback callback) {
  if (!user_has_granted_permission_) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Permission to search open tabs has not been granted."),
        {});
    return;
  }
  auto input = base::JSONReader::ReadDict(input_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Failed to parse input JSON. Provide a 'query' field."),
        {});
    return;
  }
  const auto* query = input->FindString("query");
  if (!query || query->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Missing required 'query' field."), {});
    return;
  }

  auto* embeddings_search =
      embeddings_search_for_testing_
          ? embeddings_search_for_testing_.get()
          : HistoryEmbeddingsServiceFactory::GetForProfile(profile_);
  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);
  if (!embeddings_search || !history_service) {
    std::move(callback).Run(CreateContentBlocksForText(EmptyResultsJson()), {});
    return;
  }

  history_embeddings::SearchOpenTabsByContent(
      profile_, history_service, embeddings_search, *query,
      base::BindOnce(&OnRanked, std::move(callback)), &query_url_task_tracker_);
}

}  // namespace ai_chat
