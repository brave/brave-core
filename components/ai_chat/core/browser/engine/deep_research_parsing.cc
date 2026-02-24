// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/deep_research_parsing.h"

#include <utility>

#include "base/logging.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

using GenerationResultData = EngineConsumer::GenerationResultData;

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

std::optional<GenerationResultData> ParseQueriesEvent(
    const base::DictValue& params,
    std::optional<std::string> model_key) {
  const base::ListValue* queries = params.FindList(kKeyQueries);
  if (!queries) {
    return std::nullopt;
  }

  auto event_data = mojom::DeepResearchQueriesEvent::New();
  for (const auto& query : *queries) {
    if (query.is_string()) {
      event_data->queries.push_back(query.GetString());
    }
  }

  auto dr_event =
      mojom::DeepResearchEvent::NewQueriesEvent(std::move(event_data));
  auto event = mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
  return GenerationResultData(std::move(event), std::move(model_key));
}

std::optional<GenerationResultData> ParseAnalyzingEvent(
    const base::DictValue& params,
    std::optional<std::string> model_key) {
  auto event_data = mojom::DeepResearchAnalyzingEvent::New();
  if (const std::string* query = params.FindString(kKeyQuery)) {
    event_data->query = *query;
  }
  event_data->url_count =
      static_cast<uint32_t>(params.FindInt(kKeyUrls).value_or(0));
  event_data->new_url_count =
      static_cast<uint32_t>(params.FindInt(kKeyNewUrls).value_or(0));

  auto dr_event =
      mojom::DeepResearchEvent::NewAnalyzingEvent(std::move(event_data));
  auto event = mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
  return GenerationResultData(std::move(event), std::move(model_key));
}

std::optional<GenerationResultData> ParseThinkingEvent(
    const base::DictValue& params,
    std::optional<std::string> model_key) {
  auto event_data = mojom::DeepResearchThinkingEvent::New();
  if (const std::string* query = params.FindString(kKeyQuery)) {
    event_data->query = *query;
  }
  event_data->chunks_analyzed =
      static_cast<uint32_t>(params.FindInt(kKeyChunksAnalyzed).value_or(0));
  event_data->chunks_selected =
      static_cast<uint32_t>(params.FindInt(kKeyChunksSelected).value_or(0));
  event_data->urls_analyzed =
      static_cast<uint32_t>(params.FindInt(kKeyUrlsAnalyzed).value_or(0));

  if (const base::ListValue* urls_selected =
          params.FindList(kKeyUrlsSelected)) {
    for (const auto& url : *urls_selected) {
      if (url.is_string()) {
        event_data->urls_selected.push_back(GURL(url.GetString()));
      }
    }
  }

  if (const base::ListValue* urls_info = params.FindList(kKeyUrlsInfo)) {
    for (const auto& info : *urls_info) {
      if (info.is_dict()) {
        const base::DictValue& info_dict = info.GetDict();
        auto url_info = mojom::DeepResearchUrlInfo::New();
        if (const std::string* url = info_dict.FindString(kKeyUrl)) {
          url_info->url = GURL(*url);
        }
        if (const std::string* favicon = info_dict.FindString(kKeyFavicon)) {
          url_info->favicon = GURL(*favicon);
        }
        event_data->urls_info.push_back(std::move(url_info));
      }
    }
  }

  auto dr_event =
      mojom::DeepResearchEvent::NewThinkingEvent(std::move(event_data));
  auto event = mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
  return GenerationResultData(std::move(event), std::move(model_key));
}

std::optional<GenerationResultData> ParseProgressEvent(
    const base::DictValue& params,
    std::optional<std::string> model_key) {
  auto event_data = mojom::DeepResearchProgressEvent::New();
  event_data->elapsed_seconds =
      params.FindDouble(kKeyElapsedSeconds).value_or(0.0);
  event_data->iteration_count =
      static_cast<uint32_t>(params.FindInt(kKeyIterations).value_or(0));
  event_data->queries_count =
      static_cast<uint32_t>(params.FindInt(kKeyQueriesCount).value_or(0));
  event_data->urls_analyzed =
      static_cast<uint32_t>(params.FindInt(kKeyUrlsAnalyzed).value_or(0));
  event_data->snippets_analyzed =
      static_cast<uint32_t>(params.FindInt(kKeySnippetsAnalyzed).value_or(0));

  auto dr_event =
      mojom::DeepResearchEvent::NewProgressEvent(std::move(event_data));
  auto event = mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
  return GenerationResultData(std::move(event), std::move(model_key));
}

std::optional<GenerationResultData> ParseBlindspotsEvent(
    const base::DictValue& params,
    std::optional<std::string> model_key) {
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
  auto event = mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
  return GenerationResultData(std::move(event), std::move(model_key));
}

std::optional<GenerationResultData> ParseCompleteEvent(
    const base::DictValue& params,
    std::optional<std::string> model_key) {
  auto event_data = mojom::DeepResearchCompleteEvent::New();
  if (const std::string* reason = params.FindString(kKeyReason)) {
    event_data->reason = *reason;
  }

  auto dr_event =
      mojom::DeepResearchEvent::NewCompleteEvent(std::move(event_data));
  auto event = mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
  return GenerationResultData(std::move(event), std::move(model_key));
}

std::optional<GenerationResultData> ParseErrorEvent(
    const base::DictValue& params,
    std::optional<std::string> model_key) {
  auto event_data = mojom::DeepResearchErrorEvent::New();
  if (const std::string* error = params.FindString(kKeyError)) {
    event_data->error = *error;
  }

  auto dr_event =
      mojom::DeepResearchEvent::NewErrorEvent(std::move(event_data));
  auto event = mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
  return GenerationResultData(std::move(event), std::move(model_key));
}

std::optional<GenerationResultData> ParseSearchStatusEvent(
    const base::DictValue& params,
    std::optional<std::string> model_key) {
  auto event_data = mojom::DeepResearchSearchStatusEvent::New();
  if (const std::string* status = params.FindString(kKeyStatus)) {
    if (*status == kStatusCompleted) {
      event_data->status = mojom::DeepResearchSearchStatus::kCompleted;
    } else {
      event_data->status = mojom::DeepResearchSearchStatus::kStarted;
    }
  }
  if (const std::string* query = params.FindString(kKeyQuery)) {
    event_data->query = *query;
  }
  event_data->query_index =
      static_cast<uint32_t>(params.FindInt(kKeyQueryIndex).value_or(0));
  event_data->total_queries =
      static_cast<uint32_t>(params.FindInt(kKeyTotalQueries).value_or(0));
  event_data->urls_found =
      static_cast<uint32_t>(params.FindInt(kKeyUrlsFound).value_or(0));
  event_data->elapsed_ms =
      static_cast<uint32_t>(params.FindInt(kKeyElapsedMs).value_or(0));

  auto dr_event =
      mojom::DeepResearchEvent::NewSearchStatusEvent(std::move(event_data));
  auto event = mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
  return GenerationResultData(std::move(event), std::move(model_key));
}

std::optional<GenerationResultData> ParseFetchStatusEvent(
    const base::DictValue& params,
    std::optional<std::string> model_key) {
  auto event_data = mojom::DeepResearchFetchStatusEvent::New();
  if (const std::string* query = params.FindString(kKeyQuery)) {
    event_data->query = *query;
  }
  event_data->urls_total =
      static_cast<uint32_t>(params.FindInt(kKeyUrlsTotal).value_or(0));
  event_data->urls_fetched =
      static_cast<uint32_t>(params.FindInt(kKeyUrlsFetched).value_or(0));

  auto dr_event =
      mojom::DeepResearchEvent::NewFetchStatusEvent(std::move(event_data));
  auto event = mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
  return GenerationResultData(std::move(event), std::move(model_key));
}

std::optional<GenerationResultData> ParseAnalysisStatusEvent(
    const base::DictValue& params,
    std::optional<std::string> model_key) {
  auto event_data = mojom::DeepResearchAnalysisStatusEvent::New();
  if (const std::string* status = params.FindString(kKeyStatus)) {
    if (*status == kStatusProgress) {
      event_data->status = mojom::DeepResearchAnalysisStatus::kProgress;
    } else {
      event_data->status = mojom::DeepResearchAnalysisStatus::kStarted;
    }
  }
  if (const std::string* query = params.FindString(kKeyQuery)) {
    event_data->query = *query;
  }
  event_data->chunks_analyzed =
      static_cast<uint32_t>(
          params.FindInt(kKeyChunksAnalyzedCamel).value_or(0));
  event_data->chunks_total =
      static_cast<uint32_t>(
          params.FindInt(kKeyChunksTotalCamel).value_or(0));

  auto dr_event =
      mojom::DeepResearchEvent::NewAnalysisStatusEvent(std::move(event_data));
  auto event = mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
  return GenerationResultData(std::move(event), std::move(model_key));
}

std::optional<GenerationResultData> ParseIterationCompleteEvent(
    const base::DictValue& params,
    std::optional<std::string> model_key) {
  auto event_data = mojom::DeepResearchIterationCompleteEvent::New();
  event_data->iteration =
      static_cast<uint32_t>(params.FindInt(kKeyIteration).value_or(0));
  event_data->total_iterations =
      static_cast<uint32_t>(params.FindInt(kKeyTotalIterations).value_or(0));
  event_data->queries_this_iteration =
      static_cast<uint32_t>(
          params.FindInt(kKeyQueriesThisIteration).value_or(0));
  event_data->urls_analyzed =
      static_cast<uint32_t>(params.FindInt(kKeyUrlsAnalyzedCamel).value_or(0));
  event_data->blindspots_identified =
      static_cast<uint32_t>(
          params.FindInt(kKeyBlindspotsIdentified).value_or(0));

  auto dr_event = mojom::DeepResearchEvent::NewIterationCompleteEvent(
      std::move(event_data));
  auto event = mojom::ConversationEntryEvent::NewDeepResearchEvent(
      std::move(dr_event));
  return GenerationResultData(std::move(event), std::move(model_key));
}

}  // namespace

std::optional<GenerationResultData> ParseDeepResearchEvent(
    const std::string& object_type,
    const base::DictValue& params,
    std::optional<std::string> model_key) {
  if (object_type == kTypeQueries) {
    return ParseQueriesEvent(params, std::move(model_key));
  }
  if (object_type == kTypeAnalyzing) {
    return ParseAnalyzingEvent(params, std::move(model_key));
  }
  if (object_type == kTypeThinking) {
    return ParseThinkingEvent(params, std::move(model_key));
  }
  if (object_type == kTypeProgress) {
    return ParseProgressEvent(params, std::move(model_key));
  }
  if (object_type == kTypeBlindspots) {
    return ParseBlindspotsEvent(params, std::move(model_key));
  }
  if (object_type == kTypeComplete) {
    return ParseCompleteEvent(params, std::move(model_key));
  }
  if (object_type == kTypeError) {
    return ParseErrorEvent(params, std::move(model_key));
  }
  if (object_type == kTypeSearchStatus) {
    return ParseSearchStatusEvent(params, std::move(model_key));
  }
  if (object_type == kTypeFetchStatus) {
    return ParseFetchStatusEvent(params, std::move(model_key));
  }
  if (object_type == kTypeAnalysisStatus) {
    return ParseAnalysisStatusEvent(params, std::move(model_key));
  }
  if (object_type == kTypeIterationComplete) {
    return ParseIterationCompleteEvent(params, std::move(model_key));
  }

  VLOG(1) << "Unknown deep research event type: " << object_type;
  return std::nullopt;
}

}  // namespace ai_chat
