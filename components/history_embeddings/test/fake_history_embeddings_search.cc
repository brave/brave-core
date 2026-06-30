/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/history_embeddings/test/fake_history_embeddings_search.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "components/history_embeddings/core/vector_database.h"
#include "url/gurl.h"

namespace ai_chat {

FakeHistoryEmbeddingsSearch::FakeHistoryEmbeddingsSearch() = default;
FakeHistoryEmbeddingsSearch::~FakeHistoryEmbeddingsSearch() = default;

// static
history_embeddings::ScoredUrlRow FakeHistoryEmbeddingsSearch::MakeRow(
    history::URLID url_id,
    const GURL& url,
    const std::u16string& title,
    base::Time last_visit,
    float score) {
  history_embeddings::ScoredUrl scored_url(url_id, /*visit_id=*/1,
                                           base::Time::Now(), score,
                                           /*word_match_score=*/0.0f);
  history_embeddings::ScoredUrlRow row(std::move(scored_url));
  row.row.set_url(url);
  row.row.set_title(title);
  row.row.set_last_visit(last_visit);
  return row;
}

void FakeHistoryEmbeddingsSearch::SetScoredRows(
    std::vector<history_embeddings::ScoredUrlRow> rows) {
  scored_rows_ = std::move(rows);
}

history_embeddings::SearchResult FakeHistoryEmbeddingsSearch::Search(
    history_embeddings::SearchResult* previous_search_result,
    std::string query,
    std::optional<base::Time> time_range_start,
    size_t count,
    bool skip_answering,
    std::vector<history::URLID> url_id_filter,
    history_embeddings::SearchResultCallback callback) {
  last_query_ = query;
  last_time_range_start_ = time_range_start;
  last_count_ = count;
  last_skip_answering_ = skip_answering;
  last_url_id_filter_ = url_id_filter;

  history_embeddings::SearchResult result;
  result.query = std::move(query);
  result.scored_url_rows = scored_rows_;

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(callback, std::move(result)));
  return history_embeddings::SearchResult();
}

}  // namespace ai_chat
