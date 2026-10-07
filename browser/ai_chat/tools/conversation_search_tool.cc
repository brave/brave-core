// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/conversation_search_tool.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_service.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

namespace {

constexpr char kPropertyQuery[] = "query";
constexpr char kPropertyCount[] = "count";

constexpr char kOutputKeyQuery[] = "query";
constexpr char kOutputKeyResults[] = "results";
constexpr char kOutputKeyTitle[] = "title";
constexpr char kOutputKeyLastUpdated[] = "last_updated";
constexpr char kOutputKeyPassages[] = "passages";
constexpr char kOutputKeyText[] = "text";
constexpr char kOutputKeyTime[] = "time";
constexpr char kArtifactKeyConversationUuid[] = "conversation_uuid";
constexpr char kArtifactKeyEntryUuid[] = "entry_uuid";
constexpr char kArtifactKeySnippet[] = "snippet";

constexpr int kDefaultResultCount = 5;
constexpr int kMaxResultCount = 10;

base::DictValue ConversationSearchResultToDict(
    const ConversationSearchResult& result) {
  base::ListValue passages;
  for (const ConversationPassageMatch& passage : result.passages) {
    base::DictValue entry;
    entry.Set(kOutputKeyText, passage.text);
    if (!passage.created_time.is_null()) {
      entry.Set(kOutputKeyTime,
                base::TimeFormatAsIso8601(passage.created_time));
    }
    passages.Append(std::move(entry));
  }
  base::DictValue conversation;
  conversation.Set(kOutputKeyTitle, result.title);
  if (!result.updated_time.is_null()) {
    conversation.Set(kOutputKeyLastUpdated,
                     base::TimeFormatAsIso8601(result.updated_time));
  }
  conversation.Set(kOutputKeyPassages, std::move(passages));
  return conversation;
}

}  // namespace

namespace internal {

std::string BuildConversationSearchResultJson(
    const std::string& query,
    const std::vector<ConversationSearchResult>& results) {
  base::ListValue conversations;
  for (const ConversationSearchResult& result : results) {
    conversations.Append(ConversationSearchResultToDict(result));
  }
  base::DictValue root;
  root.Set(kOutputKeyQuery, query);
  root.Set(kOutputKeyResults, std::move(conversations));
  std::string json;
  base::JSONWriter::Write(root, &json);
  return json;
}

std::string BuildConversationSearchArtifactJson(
    const std::vector<ConversationSearchResult>& results) {
  base::ListValue cards;
  for (const ConversationSearchResult& result : results) {
    base::DictValue card;
    card.Set(kArtifactKeyConversationUuid, result.conversation_uuid);
    card.Set(kOutputKeyTitle, result.title);
    if (!result.passages.empty()) {
      const ConversationPassageMatch& best_passage = result.passages.front();
      card.Set(kArtifactKeyEntryUuid, best_passage.entry_uuid);
      card.Set(kArtifactKeySnippet, best_passage.text);
    }
    cards.Append(std::move(card));
  }
  std::string json;
  base::JSONWriter::Write(cards, &json);
  return json;
}

}  // namespace internal

ConversationSearchTool::ConversationSearchTool(
    base::WeakPtr<AIChatEmbeddingsService> service,
    std::string conversation_uuid)
    : service_(std::move(service)),
      conversation_uuid_(std::move(conversation_uuid)) {}

ConversationSearchTool::~ConversationSearchTool() = default;

std::string_view ConversationSearchTool::Name() const {
  return mojom::kConversationSearchToolName;
}

std::string_view ConversationSearchTool::Description() const {
  return "Performs a semantic (meaning-based) search over the user's past "
         "conversations with you, which are stored on their device. Use when "
         "the user refers to something discussed in an earlier conversation, "
         "such as what was decided or said before, or wants to pick up a "
         "past topic. Returns the matching conversations as JSON, each with "
         "its title, when it was last updated, and the passages of its "
         "messages that match. The current conversation is not searched. "
         "Runs entirely on-device.";
}

std::optional<base::DictValue> ConversationSearchTool::InputProperties() const {
  return CreateInputProperties(
      {{kPropertyQuery,
        StringProperty("Natural language description of what to find in "
                       "the user's past conversations.")},
       {kPropertyCount,
        IntegerProperty("Maximum number of conversations to return. "
                        "Defaults to 5; capped at 10.")}});
}

std::optional<std::vector<std::string>>
ConversationSearchTool::RequiredProperties() const {
  return std::vector<std::string>{kPropertyQuery};
}

std::variant<bool, mojom::PermissionChallengePtr>
ConversationSearchTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  if (user_has_granted_permission_) {
    return false;
  }
  // The search runs on device, but what it finds is sent to the model. The
  // user-facing wording is in `get_tool_permission_implications.tsx`.
  return mojom::PermissionChallenge::New(
      /*assessment=*/std::nullopt, /*plan=*/std::nullopt,
      /*description=*/std::nullopt, /*supports_allow_session=*/false);
}

void ConversationSearchTool::UserPermissionGranted(
    const std::string& tool_use_id) {
  user_has_granted_permission_ = true;
}

void ConversationSearchTool::UseTool(const std::string& input_json,
                                     UseToolCallback callback) {
  std::optional<base::DictValue> input = base::JSONReader::ReadDict(
      input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input) {
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
  if (!service_) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Error: searching past conversations is unavailable"),
        {});
    return;
  }
  const int count =
      std::clamp(input->FindInt(kPropertyCount).value_or(kDefaultResultCount),
                 1, kMaxResultCount);
  service_->SearchConversations(
      *query, static_cast<size_t>(count), conversation_uuid_,
      base::BindOnce(&ConversationSearchTool::OnSearchResults,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     *query));
}

void ConversationSearchTool::OnSearchResults(
    UseToolCallback callback,
    const std::string& query,
    std::vector<ConversationSearchResult> results) {
  // The user is shown what the model is sent as cards.
  ToolArtifacts artifacts;
  if (!results.empty()) {
    artifacts.push_back(mojom::ToolArtifact::New(
        /*id=*/std::nullopt, mojom::kConversationSearchResultsArtifactType,
        internal::BuildConversationSearchArtifactJson(results)));
  }
  std::move(callback).Run(
      CreateContentBlocksForText(
          internal::BuildConversationSearchResultJson(query, results)),
      std::move(artifacts));
}

}  // namespace ai_chat
