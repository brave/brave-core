// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_HISTORY_EMBEDDINGS_SEARCH_H_
#define BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_HISTORY_EMBEDDINGS_SEARCH_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "brave/components/history_embeddings/core/vector_database.h"
#include "components/history/core/browser/url_row.h"

namespace brave_history_embeddings {

// Counts the # of ' ' vanilla-space characters in `s`.
size_t CountWords(const std::string& s);

// A single item that forms part of a search result; combines metadata found in
// the history embeddings database with additional info from history database.
struct ScoredUrlRow {
  explicit ScoredUrlRow(ScoredUrl scored_url);
  ScoredUrlRow(const ScoredUrlRow&);
  ScoredUrlRow(ScoredUrlRow&&);
  ~ScoredUrlRow();
  ScoredUrlRow& operator=(const ScoredUrlRow&);
  ScoredUrlRow& operator=(ScoredUrlRow&&);

  // Returns the highest scored passage in `url_data`.
  std::string GetBestPassage() const;

  // Finds the indices of the top scores, ordered descending by score.
  // The size of the returned vector will be at least
  // `min_count` provided there is sufficient data available. The
  // `min_word_count` parameter will also be used to ensure the
  // passages for returned indices have word counts adding up to at
  // least this minimum.
  std::vector<size_t> GetBestScoreIndices(size_t min_count,
                                          size_t min_word_count) const;

  // Basic scoring and history data for this URL.
  ScoredUrl scored_url;
  history::URLRow row;
  bool is_url_known_to_sync = false;

  // All passages and embeddings for this URL (i.e. not a partial set).
  UrlData url_data;

  // All scores against the query for `url_data`.
  std::vector<float> scores;
};

struct SearchResult {
  SearchResult();
  SearchResult(SearchResult&&);
  ~SearchResult();
  SearchResult& operator=(SearchResult&&);

  // Explicit copy only.
  SearchResult Clone();

  // Returns true if this search result is related to the given `other`
  // result returned by HistoryEmbeddingsService::Search (same session/query).
  bool IsContinuationOf(const SearchResult& other);

  // Session ID to associate continuing queries.
  std::string session_id;

  // Keep context for search parameters requested, to make logging easier.
  std::string query;
  std::optional<base::Time> time_range_start;
  size_t count = 0;
  SearchParams search_params;

  // The actual search result data. Note that the size of this vector will
  // not necessarily match the above requested `count`.
  std::vector<ScoredUrlRow> scored_url_rows;
};

using SearchResultCallback = base::RepeatingCallback<void(SearchResult)>;

class HistoryEmbeddingsSearch {
 public:
  virtual ~HistoryEmbeddingsSearch() = default;

  // Finds the top `count` URL visit info entries nearest to `query`. Passes the
  // results to `callback` when search completes, whether successfully or not.
  // Search will be narrowed to a time range if `time_range_start` is provided.
  // In that case, the start of the time range is inclusive and the end is
  // unbounded. Practically, this can be thought of as [start, now) but now
  // isn't fixed.
  // The `previous_search_result` may be nullptr to signal the beginning of a
  // completely new search session; if it is non-null and the session_id is set,
  // the new session_id is set based on the previous to indicate a continuing
  // search session.
  // When `url_id_filter` is non-empty, the search is restricted to UrlData
  // rows whose `url_id` is in the set; storage backends use it to short-circuit
  // a full-corpus scan when the caller already knows the candidate URLIDs.
  // Returns a stub result that can be used to detect if a later published
  // SearchResult instance is related to this search.
  virtual SearchResult Search(SearchResult* previous_search_result,
                              std::string query,
                              std::optional<base::Time> time_range_start,
                              size_t count,
                              std::vector<history::URLID> url_id_filter,
                              SearchResultCallback callback) = 0;
};

}  // namespace brave_history_embeddings

#endif  // BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_HISTORY_EMBEDDINGS_SEARCH_H_
