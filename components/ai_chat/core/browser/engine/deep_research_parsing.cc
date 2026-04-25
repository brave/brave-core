// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/deep_research_parsing.h"

#include <utility>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

// JSON field name constants
constexpr char kKeyQueries[] = "queries";
constexpr char kKeyQuery[] = "query";
constexpr char kKeyUrls[] = "urls";
constexpr char kKeyNewUrls[] = "new_urls";
constexpr char kKeyUrl[] = "url";
constexpr char kKeyFavicon[] = "favicon";
constexpr char kKeyChunksAnalyzed[] = "chunks_analyzed";
constexpr char kKeyChunksSelected[] = "chunks_selected";
constexpr char kKeyUrlsAnalyzed[] = "urls_analyzed";
constexpr char kKeyUrlsSelected[] = "urls_selected";
constexpr char kKeyUrlsInfo[] = "urls_info";
constexpr char kKeyElapsedSeconds[] = "elapsed_seconds";
constexpr char kKeyIterations[] = "iterations";
constexpr char kKeyQueriesCount[] = "queries_count";
constexpr char kKeySnippetsAnalyzed[] = "snippets_analyzed";
constexpr char kKeyBlindspots[] = "blindspots";
constexpr char kKeyReason[] = "reason";
constexpr char kKeyError[] = "error";
constexpr char kKeyStatus[] = "status";
constexpr char kKeyQueryIndex[] = "queryIndex";
constexpr char kKeyTotalQueries[] = "totalQueries";
constexpr char kKeyUrlsFound[] = "urlsFound";
constexpr char kKeyElapsedMs[] = "elapsedMs";
constexpr char kKeyUrlsTotal[] = "urlsTotal";
constexpr char kKeyUrlsFetched[] = "urlsFetched";
constexpr char kKeyChunksAnalyzedCamel[] = "chunksAnalyzed";
constexpr char kKeyChunksTotalCamel[] = "chunksTotal";
constexpr char kKeyIteration[] = "iteration";
constexpr char kKeyTotalIterations[] = "totalIterations";
constexpr char kKeyQueriesThisIteration[] = "queriesThisIteration";
constexpr char kKeyUrlsAnalyzedCamel[] = "urlsAnalyzed";
constexpr char kKeyBlindspotsIdentified[] = "blindspotsIdentified";

// Status string values
constexpr char kStatusStarted[] = "started";
constexpr char kStatusCompleted[] = "completed";
constexpr char kStatusProgress[] = "progress";

// Object type constants for the dispatcher
constexpr char kTypeQueries[] = "brave-chat.deepResearch.queries";
constexpr char kTypeAnalyzing[] = "brave-chat.deepResearch.analyzing";
constexpr char kTypeThinking[] = "brave-chat.deepResearch.thinking";
constexpr char kTypeProgress[] = "brave-chat.deepResearch.progress";
constexpr char kTypeBlindspots[] = "brave-chat.deepResearch.blindspots";
constexpr char kTypeComplete[] = "brave-chat.deepResearch.complete";
constexpr char kTypeError[] = "brave-chat.deepResearch.error";
constexpr char kTypeSearchStatus[] = "brave-chat.deepResearch.searchStatus";
constexpr char kTypeFetchStatus[] = "brave-chat.deepResearch.fetchStatus";
constexpr char kTypeAnalysisStatus[] = "brave-chat.deepResearch.analysisStatus";
constexpr char kTypeIterationComplete[] =
    "brave-chat.deepResearch.iterationComplete";

mojom::ConversationEntryEventPtr ParseQueriesEvent(
    const base::DictValue& params) {
  const base::ListValue* queries = params.FindList(kKeyQueries);
  if (!queries) {
    return nullptr;
  }

  auto event_data = mojom::DeepResearchQueriesEvent::New();
  for (const auto& query : *queries) {
    if (query.is_string()) {
      event_data->queries.push_back(query.GetString());
    }
  }

  auto dr_event =
      mojom::DeepResearchEvent::NewQueriesEvent(std::move(event_data));
  return mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
}

mojom::ConversationEntryEventPtr ParseAnalyzingEvent(
    const base::DictValue& params) {
  auto event_data = mojom::DeepResearchAnalyzingEvent::New();
  if (const std::string* query = params.FindString(kKeyQuery)) {
    event_data->query = *query;
  }
  event_data->url_count =
      base::saturated_cast<uint32_t>(params.FindInt(kKeyUrls).value_or(0));
  event_data->new_url_count =
      base::saturated_cast<uint32_t>(params.FindInt(kKeyNewUrls).value_or(0));

  auto dr_event =
      mojom::DeepResearchEvent::NewAnalyzingEvent(std::move(event_data));
  return mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
}

mojom::ConversationEntryEventPtr ParseThinkingEvent(
    const base::DictValue& params) {
  auto event_data = mojom::DeepResearchThinkingEvent::New();
  if (const std::string* query = params.FindString(kKeyQuery)) {
    event_data->query = *query;
  }
  event_data->chunks_analyzed = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyChunksAnalyzed).value_or(0));
  event_data->chunks_selected = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyChunksSelected).value_or(0));
  event_data->urls_analyzed = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyUrlsAnalyzed).value_or(0));

  if (const base::ListValue* urls_selected =
          params.FindList(kKeyUrlsSelected)) {
    for (const auto& url : *urls_selected) {
      if (url.is_string()) {
        GURL parsed_url(url.GetString());
        if (parsed_url.is_valid()) {
          event_data->urls_selected.push_back(std::move(parsed_url));
        }
      }
    }
  }

  if (const base::ListValue* urls_info = params.FindList(kKeyUrlsInfo)) {
    for (const auto& info : *urls_info) {
      if (info.is_dict()) {
        const base::DictValue& info_dict = info.GetDict();
        auto url_info = mojom::DeepResearchUrlInfo::New();
        if (const std::string* url = info_dict.FindString(kKeyUrl)) {
          GURL parsed_url(*url);
          if (parsed_url.is_valid()) {
            url_info->url = std::move(parsed_url);
          }
        }
        if (const std::string* favicon = info_dict.FindString(kKeyFavicon)) {
          GURL parsed_favicon(*favicon);
          if (parsed_favicon.is_valid()) {
            url_info->favicon = std::move(parsed_favicon);
          }
        }
        event_data->urls_info.push_back(std::move(url_info));
      }
    }
  }

  auto dr_event =
      mojom::DeepResearchEvent::NewThinkingEvent(std::move(event_data));
  return mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
}

mojom::ConversationEntryEventPtr ParseProgressEvent(
    const base::DictValue& params) {
  auto event_data = mojom::DeepResearchProgressEvent::New();
  event_data->elapsed_seconds =
      params.FindDouble(kKeyElapsedSeconds).value_or(0.0);
  event_data->iteration_count = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyIterations).value_or(0));
  event_data->queries_count = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyQueriesCount).value_or(0));
  event_data->urls_analyzed = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyUrlsAnalyzed).value_or(0));
  event_data->snippets_analyzed = base::saturated_cast<uint32_t>(
      params.FindInt(kKeySnippetsAnalyzed).value_or(0));

  auto dr_event =
      mojom::DeepResearchEvent::NewProgressEvent(std::move(event_data));
  return mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
}

mojom::ConversationEntryEventPtr ParseBlindspotsEvent(
    const base::DictValue& params) {
  auto event_data = mojom::DeepResearchBlindspotsEvent::New();
  if (const base::ListValue* blindspots = params.FindList(kKeyBlindspots)) {
    for (const auto& bs : *blindspots) {
      if (bs.is_string()) {
        event_data->blindspots.push_back(bs.GetString());
      }
    }
  }

  auto dr_event =
      mojom::DeepResearchEvent::NewBlindspotsEvent(std::move(event_data));
  return mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
}

mojom::ConversationEntryEventPtr ParseCompleteEvent(
    const base::DictValue& params) {
  auto event_data = mojom::DeepResearchCompleteEvent::New();
  if (const std::string* reason = params.FindString(kKeyReason)) {
    event_data->reason = *reason;
  }

  auto dr_event =
      mojom::DeepResearchEvent::NewCompleteEvent(std::move(event_data));
  return mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
}

mojom::ConversationEntryEventPtr ParseErrorEvent(
    const base::DictValue& params) {
  auto event_data = mojom::DeepResearchErrorEvent::New();
  if (const std::string* error = params.FindString(kKeyError)) {
    event_data->error = *error;
  }

  auto dr_event =
      mojom::DeepResearchEvent::NewErrorEvent(std::move(event_data));
  return mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
}

mojom::ConversationEntryEventPtr ParseSearchStatusEvent(
    const base::DictValue& params) {
  auto event_data = mojom::DeepResearchSearchStatusEvent::New();
  if (const std::string* status = params.FindString(kKeyStatus)) {
    if (*status == kStatusCompleted) {
      event_data->status = mojom::DeepResearchSearchStatus::kCompleted;
    } else if (*status == kStatusStarted) {
      event_data->status = mojom::DeepResearchSearchStatus::kStarted;
    } else {
      VLOG(1) << "Unknown search status: " << *status;
      return nullptr;
    }
  }
  if (const std::string* query = params.FindString(kKeyQuery)) {
    event_data->query = *query;
  }
  event_data->query_index = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyQueryIndex).value_or(0));
  event_data->total_queries = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyTotalQueries).value_or(0));
  event_data->urls_found =
      base::saturated_cast<uint32_t>(params.FindInt(kKeyUrlsFound).value_or(0));
  event_data->elapsed_ms =
      base::saturated_cast<uint32_t>(params.FindInt(kKeyElapsedMs).value_or(0));

  auto dr_event =
      mojom::DeepResearchEvent::NewSearchStatusEvent(std::move(event_data));
  return mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
}

mojom::ConversationEntryEventPtr ParseFetchStatusEvent(
    const base::DictValue& params) {
  auto event_data = mojom::DeepResearchFetchStatusEvent::New();
  if (const std::string* query = params.FindString(kKeyQuery)) {
    event_data->query = *query;
  }
  event_data->urls_total =
      base::saturated_cast<uint32_t>(params.FindInt(kKeyUrlsTotal).value_or(0));
  event_data->urls_fetched = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyUrlsFetched).value_or(0));

  auto dr_event =
      mojom::DeepResearchEvent::NewFetchStatusEvent(std::move(event_data));
  return mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
}

mojom::ConversationEntryEventPtr ParseAnalysisStatusEvent(
    const base::DictValue& params) {
  auto event_data = mojom::DeepResearchAnalysisStatusEvent::New();
  if (const std::string* status = params.FindString(kKeyStatus)) {
    if (*status == kStatusProgress) {
      event_data->status = mojom::DeepResearchAnalysisStatus::kProgress;
    } else if (*status == kStatusStarted) {
      event_data->status = mojom::DeepResearchAnalysisStatus::kStarted;
    } else {
      VLOG(1) << "Unknown analysis status: " << *status;
      return nullptr;
    }
  }
  if (const std::string* query = params.FindString(kKeyQuery)) {
    event_data->query = *query;
  }
  event_data->chunks_analyzed = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyChunksAnalyzedCamel).value_or(0));
  event_data->chunks_total = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyChunksTotalCamel).value_or(0));

  auto dr_event =
      mojom::DeepResearchEvent::NewAnalysisStatusEvent(std::move(event_data));
  return mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
}

mojom::ConversationEntryEventPtr ParseIterationCompleteEvent(
    const base::DictValue& params) {
  auto event_data = mojom::DeepResearchIterationCompleteEvent::New();
  event_data->iteration =
      base::saturated_cast<uint32_t>(params.FindInt(kKeyIteration).value_or(0));
  event_data->total_iterations = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyTotalIterations).value_or(0));
  event_data->queries_this_iteration = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyQueriesThisIteration).value_or(0));
  event_data->urls_analyzed = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyUrlsAnalyzedCamel).value_or(0));
  event_data->blindspots_identified = base::saturated_cast<uint32_t>(
      params.FindInt(kKeyBlindspotsIdentified).value_or(0));

  auto dr_event = mojom::DeepResearchEvent::NewIterationCompleteEvent(
      std::move(event_data));
  return mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
}

}  // namespace

mojom::ConversationEntryEventPtr ParseDeepResearchEvent(
    const std::string& object_type,
    const base::DictValue& params) {
  if (object_type == kTypeQueries) {
    return ParseQueriesEvent(params);
  }
  if (object_type == kTypeAnalyzing) {
    return ParseAnalyzingEvent(params);
  }
  if (object_type == kTypeThinking) {
    return ParseThinkingEvent(params);
  }
  if (object_type == kTypeProgress) {
    return ParseProgressEvent(params);
  }
  if (object_type == kTypeBlindspots) {
    return ParseBlindspotsEvent(params);
  }
  if (object_type == kTypeComplete) {
    return ParseCompleteEvent(params);
  }
  if (object_type == kTypeError) {
    return ParseErrorEvent(params);
  }
  if (object_type == kTypeSearchStatus) {
    return ParseSearchStatusEvent(params);
  }
  if (object_type == kTypeFetchStatus) {
    return ParseFetchStatusEvent(params);
  }
  if (object_type == kTypeAnalysisStatus) {
    return ParseAnalysisStatusEvent(params);
  }
  if (object_type == kTypeIterationComplete) {
    return ParseIterationCompleteEvent(params);
  }

  VLOG(1) << "Unknown deep research event type: " << object_type;
  return nullptr;
}

}  // namespace ai_chat
