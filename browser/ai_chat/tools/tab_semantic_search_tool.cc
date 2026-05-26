// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_semantic_search_tool.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history_embeddings/content/history_embeddings_service.h"
#include "components/history_embeddings/core/history_embeddings_search.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr size_t kDefaultResultCount = 5;
constexpr size_t kMaxResultCount = 20;

// Cap on how many rows we ask HistoryEmbeddingsService::Search() to return so
// there is headroom for the open-tab URL filter to find matches without
// missing them due to a low Search() `count`.
constexpr size_t kSearchCount = 100;

using OpenTab = internal::SemanticSearchTabInfo;

// Same filter upstream TabSearch uses: regular browser windows belonging to
// `profile`. Inlined here so we don't pull in chromium_src/tab_search.
bool ShouldTrackBrowser(Profile* profile, BrowserWindowInterface* browser) {
  return browser->GetProfile() == profile &&
         browser->GetType() == BrowserWindowInterface::TYPE_NORMAL;
}

std::vector<OpenTab> SnapshotOpenTabs(Profile* profile) {
  std::vector<OpenTab> tabs;
  if (!profile) {
    return tabs;
  }
  GlobalBrowserCollection::GetInstance()->ForEach(
      [profile, &tabs](BrowserWindowInterface* browser_window) {
        if (!ShouldTrackBrowser(profile, browser_window)) {
          return true;
        }
        TabStripModel* tab_strip_model = browser_window->GetTabStripModel();
        if (!tab_strip_model) {
          return true;
        }
        std::vector<int> indices(tab_strip_model->count());
        std::iota(indices.begin(), indices.end(), 0);
        for (tabs::TabInterface* tab :
             tab_strip_model->GetTabsAtIndices(indices)) {
          if (!tab) {
            continue;
          }
          content::WebContents* contents = tab->GetContents();
          if (!contents) {
            continue;
          }
          GURL url = contents->GetLastCommittedURL();
          if (!url.SchemeIsHTTPOrHTTPS()) {
            continue;
          }
          OpenTab info;
          info.tab_id = tab->GetHandle().raw_value();
          info.title = base::UTF16ToUTF8(contents->GetTitle());
          info.url = std::move(url);
          info.url_id = 0;
          tabs.push_back(std::move(info));
        }
        return true;
      });
  return tabs;
}

std::string EmptyResultsJson() {
  base::DictValue root;
  root.Set("results", base::ListValue());
  std::string serialized;
  base::JSONWriter::Write(root, &serialized);
  return serialized;
}

void OnUrlIdsResolved(
    std::vector<OpenTab> tabs,
    size_t count,
    std::string query,
    history_embeddings::HistoryEmbeddingsService* embeddings_service,
    Tool::UseToolCallback callback,
    std::optional<std::vector<history::URLID>> url_ids) {
  if (!url_ids) {
    std::move(callback).Run(CreateContentBlocksForText(EmptyResultsJson()), {});
    return;
  }
  CHECK_EQ(tabs.size(), url_ids->size());
  std::vector<history::URLID> url_id_filter;
  url_id_filter.reserve(url_ids->size());
  for (size_t i = 0; i < url_ids->size(); ++i) {
    tabs[i].url_id = (*url_ids)[i];
    if ((*url_ids)[i] != 0) {
      url_id_filter.push_back((*url_ids)[i]);
    }
  }
  if (url_id_filter.empty()) {
    std::move(callback).Run(CreateContentBlocksForText(EmptyResultsJson()), {});
    return;
  }
  embeddings_service->Search(
      /*previous_search_result=*/nullptr, query,
      /*time_range_start=*/std::nullopt, kSearchCount,
      /*skip_answering=*/true, std::move(url_id_filter),
      base::BindRepeating(
          [](Tool::UseToolCallback& cb, const std::vector<OpenTab>& tabs,
             size_t count, history_embeddings::SearchResult result) {
            std::string serialized =
                internal::BuildSemanticSearchResultsJson(tabs, result, count);
            std::vector<mojom::ToolArtifactPtr> artifacts;
            std::string sources_json =
                internal::BuildSemanticSearchTabSourcesJson(tabs, result,
                                                            count);
            if (!sources_json.empty()) {
              auto artifact = mojom::ToolArtifact::New();
              artifact->type = mojom::kTabSourcesArtifactType;
              artifact->content_json = std::move(sources_json);
              artifacts.push_back(std::move(artifact));
            }
            std::move(cb).Run(CreateContentBlocksForText(serialized),
                              std::move(artifacts));
          },
          base::OwnedRef(std::move(callback)), std::move(tabs), count));
}

}  // namespace

namespace internal {

std::string BuildSemanticSearchResultsJson(
    const std::vector<SemanticSearchTabInfo>& tabs,
    const history_embeddings::SearchResult& result,
    size_t count) {
  // Map open tabs by `url_id` so we can attach `tab_id`/`title`/`url` to
  // ranked results. We use `url_id` (not GURL) because Search's
  // `row.row.url()` comes from history's canonical form, which may differ
  // from the open tab's URL (trailing slashes, fragments, etc.). The
  // URL-id filter already guarantees every row corresponds to an open tab,
  // so the lookup just needs a stable identifier.
  base::flat_map<history::URLID, const SemanticSearchTabInfo*> by_url_id;
  for (const auto& tab : tabs) {
    if (tab.url_id != 0) {
      by_url_id.emplace(tab.url_id, &tab);
    }
  }

  base::ListValue results_list;
  for (const auto& row : result.scored_url_rows) {
    if (results_list.size() >= count) {
      break;
    }
    auto* tab_pp = base::FindOrNull(by_url_id, row.scored_url.url_id);
    if (!tab_pp) {
      continue;
    }
    const SemanticSearchTabInfo* tab = *tab_pp;
    base::DictValue entry;
    entry.Set("tab_id", base::NumberToString(tab->tab_id));
    entry.Set("title", tab->title);
    entry.Set("url", tab->url.spec());
    results_list.Append(std::move(entry));
  }
  base::DictValue root;
  root.Set("results", std::move(results_list));
  std::string serialized;
  base::JSONWriter::Write(root, &serialized);
  return serialized;
}

std::string BuildSemanticSearchTabSourcesJson(
    const std::vector<SemanticSearchTabInfo>& tabs,
    const history_embeddings::SearchResult& result,
    size_t count) {
  base::flat_map<history::URLID, const SemanticSearchTabInfo*> by_url_id;
  for (const auto& tab : tabs) {
    if (tab.url_id != 0) {
      by_url_id.emplace(tab.url_id, &tab);
    }
  }

  base::ListValue sources;
  for (const auto& row : result.scored_url_rows) {
    if (sources.size() >= count) {
      break;
    }
    auto* tab_pp = base::FindOrNull(by_url_id, row.scored_url.url_id);
    if (!tab_pp) {
      continue;
    }
    const SemanticSearchTabInfo* tab = *tab_pp;
    base::DictValue entry;
    entry.Set("tab_id", tab->tab_id);
    entry.Set("title", tab->title);
    entry.Set("url", tab->url.spec());
    sources.Append(std::move(entry));
  }
  if (sources.empty()) {
    return std::string();
  }
  base::DictValue root;
  root.Set("sources", std::move(sources));
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
            "Natural-language query describing the tab to find by content.")},
       {"count",
        IntegerProperty("Maximum number of matching tabs to return (default "
                        "5, max 20).")}});
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
  return mojom::PermissionChallenge::New(
      std::nullopt,
      std::string(
          "Allow Leo to semantically search your currently-open tabs? The "
          "search query and the full content of your tabs stay on this "
          "device. Only matched tabs' titles and URLs are sent to Brave AI "
          "as tool output. Permission applies to this conversation."));
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
  size_t count = kDefaultResultCount;
  if (auto count_opt = input->FindInt("count")) {
    count = std::clamp<size_t>(static_cast<size_t>(*count_opt), 1u,
                               kMaxResultCount);
  }

  auto tabs = SnapshotOpenTabs(profile_);
  auto* embeddings_service =
      HistoryEmbeddingsServiceFactory::GetForProfile(profile_);
  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);

  if (tabs.empty() || !embeddings_service || !history_service) {
    std::move(callback).Run(CreateContentBlocksForText(EmptyResultsJson()), {});
    return;
  }

  std::vector<GURL> urls;
  urls.reserve(tabs.size());
  for (const auto& tab : tabs) {
    urls.push_back(tab.url);
  }
  history_service->QueryUrlIds(
      urls,
      base::BindOnce(&OnUrlIdsResolved, std::move(tabs), count, *query,
                     embeddings_service, std::move(callback)),
      &query_url_task_tracker_);
}

}  // namespace ai_chat
