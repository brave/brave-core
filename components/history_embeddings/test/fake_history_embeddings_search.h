/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_TEST_FAKE_HISTORY_EMBEDDINGS_SEARCH_H_
#define BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_TEST_FAKE_HISTORY_EMBEDDINGS_SEARCH_H_

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "components/history/core/browser/history_types.h"
#include "components/history_embeddings/core/history_embeddings_search.h"

class GURL;

namespace ai_chat {

// Fake `HistoryEmbeddingsSearch` for tool tests. Records the args of the most
// recent `Search()` call and fires a caller-configured list of
// `ScoredUrlRow`s back asynchronously, so tests don't have to stand up a real
// `HistoryEmbeddingsService`.
class FakeHistoryEmbeddingsSearch
    : public history_embeddings::HistoryEmbeddingsSearch {
 public:
  FakeHistoryEmbeddingsSearch();
  ~FakeHistoryEmbeddingsSearch() override;

  FakeHistoryEmbeddingsSearch(const FakeHistoryEmbeddingsSearch&) = delete;
  FakeHistoryEmbeddingsSearch& operator=(const FakeHistoryEmbeddingsSearch&) =
      delete;

  // Builds a `ScoredUrlRow` suitable for `SetScoredRows()`. Convenience for
  // tests that need to populate the basic fields without touching the
  // embeddings details.
  static history_embeddings::ScoredUrlRow MakeRow(history::URLID url_id,
                                                  const GURL& url,
                                                  const std::u16string& title,
                                                  base::Time last_visit,
                                                  float score);

  // Sets the rows that subsequent `Search()` calls will return.
  void SetScoredRows(std::vector<history_embeddings::ScoredUrlRow> rows);

  // Captured args from the most recent `Search()` call.
  const std::string& last_query() const { return last_query_; }
  const std::optional<base::Time>& last_time_range_start() const {
    return last_time_range_start_;
  }
  size_t last_count() const { return last_count_; }
  bool last_skip_answering() const { return last_skip_answering_; }
  const std::vector<history::URLID>& last_url_id_filter() const {
    return last_url_id_filter_;
  }

  // `HistoryEmbeddingsSearch`:
  history_embeddings::SearchResult Search(
      history_embeddings::SearchResult* previous_search_result,
      std::string query,
      std::optional<base::Time> time_range_start,
      size_t count,
      bool skip_answering,
      std::vector<history::URLID> url_id_filter,
      history_embeddings::SearchResultCallback callback) override;

 private:
  std::vector<history_embeddings::ScoredUrlRow> scored_rows_;
  std::string last_query_;
  std::optional<base::Time> last_time_range_start_;
  size_t last_count_ = 0;
  bool last_skip_answering_ = false;
  std::vector<history::URLID> last_url_id_filter_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_TEST_FAKE_HISTORY_EMBEDDINGS_SEARCH_H_
