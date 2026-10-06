// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_ui_semantic_search.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/strings/string_split.h"
#include "brave/browser/ai_chat/ai_chat_embeddings_service_factory.h"
#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_service.h"
#include "components/history_embeddings/core/history_embeddings_features.h"

namespace ai_chat {

namespace {

// Counts words as brave://history does before it searches semantically.
bool HasEnoughWords(const std::string& query) {
  return base::SplitStringPiece(query, " ", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY)
             .size() >=
         static_cast<size_t>(history_embeddings::GetFeatureParameters()
                                 .search_query_minimum_word_count);
}

size_t GetResultCount() {
  return static_cast<size_t>(
      history_embeddings::GetFeatureParameters().search_result_item_count);
}

}  // namespace

void SearchConversationsForUI(content::BrowserContext* context,
                              const std::string& query,
                              UISearchConversationsCallback callback) {
  AIChatEmbeddingsService* service =
      AIChatEmbeddingsServiceFactory::GetForBrowserContext(context);
  if (!service) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  if (!HasEnoughWords(query)) {
    std::move(callback).Run(std::vector<ConversationSearchResult>());
    return;
  }
  service->SearchConversations(
      query, GetResultCount(), /*excluded_conversation_uuid=*/"",
      base::BindOnce(
          [](UISearchConversationsCallback callback,
             std::vector<ConversationSearchResult> results) {
            std::move(callback).Run(std::move(results));
          },
          std::move(callback)));
}

void SearchMemoriesForUI(content::BrowserContext* context,
                         const std::string& query,
                         UISearchMemoriesCallback callback) {
  AIChatEmbeddingsService* service =
      AIChatEmbeddingsServiceFactory::GetForBrowserContext(context);
  if (!service) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  if (!HasEnoughWords(query)) {
    std::move(callback).Run(std::vector<std::string>());
    return;
  }
  service->SearchMemories(query, GetResultCount(),
                          base::BindOnce(
                              [](UISearchMemoriesCallback callback,
                                 std::vector<std::string> memories) {
                                std::move(callback).Run(std::move(memories));
                              },
                              std::move(callback)));
}

}  // namespace ai_chat
