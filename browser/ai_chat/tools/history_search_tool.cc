// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/history_search_tool.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"
#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/url_row.h"
#include "components/history_embeddings/content/history_embeddings_service.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

constexpr char kPropertyQuery[] = "query";
constexpr char kPropertyCount[] = "count";
constexpr char kPropertyTimeRangeStartDaysAgo[] = "time_range_start_days_ago";
constexpr char kPropertyIncludeAllPassages[] = "include_all_passages";

constexpr char kOutputKeyQuery[] = "query";
constexpr char kOutputKeyResults[] = "results";
constexpr char kOutputKeyTitle[] = "title";
constexpr char kOutputKeyUrl[] = "url";
constexpr char kOutputKeyLastVisitTime[] = "last_visit_time";
constexpr char kOutputKeyPassages[] = "passages";
constexpr char kOutputKeySnippet[] = "snippet";

constexpr size_t kDefaultResultCount = 5;
constexpr size_t kMaxResultCount = 10;

base::DictValue ScoredUrlRowToDict(const history_embeddings::ScoredUrlRow& row,
                                   bool include_all_passages) {
  base::DictValue entry;
  entry.Set(kOutputKeyTitle, base::UTF16ToUTF8(row.row.title()));
  entry.Set(kOutputKeyUrl, row.row.url().spec());
  // GetBestPassage() / GetBestScoreIndices() CHECK that there is at least
  // one passage; some rows (e.g. URL-only history entries with no embedded
  // passages) won't have any, so guard against it.
  const int passages_size = row.url_data.passages.passages_size();
  if (passages_size > 0) {
    if (include_all_passages) {
      base::ListValue passages;
      // Returns indices ordered by descending score.
      auto indices = row.GetBestScoreIndices(/*min_count=*/passages_size,
                                             /*min_word_count=*/0);
      for (size_t i : indices) {
        passages.Append(row.url_data.passages.passages(i));
      }
      entry.Set(kOutputKeyPassages, std::move(passages));
    } else {
      entry.Set(kOutputKeySnippet, row.GetBestPassage());
    }
  }
  if (!row.row.last_visit().is_null()) {
    entry.Set(kOutputKeyLastVisitTime,
              base::TimeFormatAsIso8601(row.row.last_visit()));
  }
  return entry;
}

}  // namespace

namespace internal {

std::string BuildHistorySearchResultJson(
    const std::string& query,
    const history_embeddings::SearchResult& result,
    bool include_all_passages) {
  base::ListValue results;
  for (const auto& row : result.scored_url_rows) {
    results.Append(ScoredUrlRowToDict(row, include_all_passages));
  }

  base::DictValue root;
  root.Set(kOutputKeyQuery, query);
  root.Set(kOutputKeyResults, std::move(results));

  std::string json;
  base::JSONWriter::Write(root, &json);
  return json;
}

}  // namespace internal

HistorySearchTool::HistorySearchTool(content::BrowserContext* browser_context)
    : profile_(Profile::FromBrowserContext(browser_context)) {}

HistorySearchTool::~HistorySearchTool() = default;

std::string_view HistorySearchTool::Name() const {
  return mojom::kSemanticHistorySearchToolName;
}

std::string_view HistorySearchTool::Description() const {
  return "Performs a semantic (meaning-based) search over the user's local "
         "browsing history. Use when the user asks about something they "
         "previously read but can't recall the URL or title, or when you "
         "need the page's text content to answer their question. Returns "
         "matching pages as JSON with title, URL, last visit time, and "
         "(by default) the indexed text passages -- the only way to "
         "access page content from history; you do not need to ask the "
         "user to fetch the page. Do not use for exact URL/title lookup "
         "or general web search. Runs entirely on-device.";
}

std::optional<base::DictValue> HistorySearchTool::InputProperties() const {
  return CreateInputProperties(
      {{kPropertyQuery,
        StringProperty("Natural language description of the page the user "
                       "is trying to find in their browsing history.")},
       {kPropertyCount,
        IntegerProperty(
            "Maximum number of results to return. Defaults to 5; capped "
            "at 10.")},
       {kPropertyTimeRangeStartDaysAgo,
        IntegerProperty(
            "If set, restrict the search to pages visited within the last "
            "N days. Omit to search the entire history.")},
       {kPropertyIncludeAllPassages,
        BooleanProperty(
            "Include all indexed passages per result, sorted by "
            "relevance. Defaults to true. Set false to return a single "
            "'snippet' (best-matching passage only) when no page "
            "content is needed.")}});
}

std::optional<std::vector<std::string>> HistorySearchTool::RequiredProperties()
    const {
  return std::vector<std::string>{kPropertyQuery};
}

std::variant<bool, mojom::PermissionChallengePtr>
HistorySearchTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  if (user_has_granted_permission_) {
    return false;
  }
  // The search itself runs entirely on-device against the local embeddings
  // index, so the query and the full history never leave the device. Only
  // the matching pages' titles, URLs, and indexed passages are sent to the
  // remote LLM as the tool's output. Ask the user to confirm before doing
  // that. The user-facing wording is in
  // `get_tool_permission_implications.tsx` so it goes through i18n; this
  // C++ side only needs to surface a non-null challenge.
  return mojom::PermissionChallenge::New(/*assessment=*/std::nullopt,
                                         /*plan=*/std::nullopt);
}

void HistorySearchTool::UserPermissionGranted(const std::string& tool_use_id) {
  user_has_granted_permission_ = true;
}

void HistorySearchTool::UseTool(const std::string& input_json,
                                UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: failed to parse input JSON"), {});
    return;
  }

  const std::string* query = input->FindString(kPropertyQuery);
  if (!query || query->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: missing or empty 'query' field"),
        {});
    return;
  }

  // Match the chrome://history and omnibox gating: reject queries with fewer
  // than `search_query_minimum_word_count` words (default 2). The service
  // API itself does not enforce this, but very short queries produce noisy
  // semantic results.
  const int min_words = history_embeddings::GetFeatureParameters()
                            .search_query_minimum_word_count;
  if (static_cast<int>(history_embeddings::CountWords(*query)) < min_words) {
    std::move(callback).Run(
        CreateContentBlocksForText(base::StrCat(
            {"Error: 'query' must contain at least ",
             base::NumberToString(min_words),
             " words. Provide a longer natural-language description."})),
        {});
    return;
  }

  size_t count = kDefaultResultCount;
  if (auto requested = input->FindInt(kPropertyCount)) {
    count = static_cast<size_t>(
        std::clamp(*requested, 1, static_cast<int>(kMaxResultCount)));
  }

  std::optional<base::Time> time_range_start;
  if (auto days = input->FindInt(kPropertyTimeRangeStartDaysAgo);
      days && *days > 0) {
    time_range_start = base::Time::Now() - base::Days(*days);
  }

  bool include_all_passages =
      input->FindBool(kPropertyIncludeAllPassages).value_or(true);

  // Defense in depth: the tool is only published when the flag is on, but
  // re-check here in case the flag flipped between advertisement and use.
  // Tests that inject a fake search interface bypass this gate.
  if (!search_for_testing_ &&
      !history_embeddings::IsHistoryEmbeddingsEnabledForProfile(profile_)) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Error: history embeddings is not enabled for this profile"),
        {});
    return;
  }

  history_embeddings::HistoryEmbeddingsSearch* service =
      search_for_testing_
          ? search_for_testing_.get()
          : HistoryEmbeddingsServiceFactory::GetForProfile(profile_);
  if (!service) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Error: history embeddings service is unavailable"),
        {});
    return;
  }

  // history_embeddings::SearchResultCallback is a RepeatingCallback. Adapt
  // the OnceCallback by heap-allocating it and letting base::Owned() tie its
  // lifetime to the bound closure; OnSearchResult guards against the
  // theoretical case of more than one fire.
  service->Search(
      /*previous_search_result=*/nullptr, *query, time_range_start, count,
      /*skip_answering=*/true,
      base::BindRepeating(
          &HistorySearchTool::OnSearchResult, weak_ptr_factory_.GetWeakPtr(),
          base::Owned(std::make_unique<UseToolCallback>(std::move(callback))),
          include_all_passages));
}

void HistorySearchTool::SetSearchForTesting(
    history_embeddings::HistoryEmbeddingsSearch* search) {
  search_for_testing_ = search;
}

void HistorySearchTool::OnSearchResult(
    UseToolCallback* callback,
    bool include_all_passages,
    history_embeddings::SearchResult result) {
  if (callback->is_null()) {
    return;
  }
  std::string json = internal::BuildHistorySearchResultJson(
      result.query, result, include_all_passages);

  // Surface the URLs in our results as a visited-links artifact so the
  // conversation UI permits anchors in the assistant's reply for them.
  // The artifact has no visual rendering -- it's a sidechannel trust-list.
  ToolArtifacts artifacts;
  if (!result.scored_url_rows.empty()) {
    base::ListValue links;
    for (const auto& row : result.scored_url_rows) {
      if (row.row.url().is_valid() &&
          row.row.url().SchemeIs(url::kHttpsScheme)) {
        links.Append(row.row.url().spec());
      }
    }
    if (!links.empty()) {
      std::string links_json;
      base::JSONWriter::Write(links, &links_json);
      artifacts.push_back(mojom::ToolArtifact::New(
          /*id=*/std::nullopt, mojom::kVisitedLinksArtifactType,
          std::move(links_json)));
    }
  }

  std::move(*callback).Run(CreateContentBlocksForText(json),
                           std::move(artifacts));
}

}  // namespace ai_chat
