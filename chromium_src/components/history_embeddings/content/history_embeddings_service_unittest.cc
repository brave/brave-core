/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/history_embeddings/content/brave_history_embeddings_service.h"

// Swap the base class so that HistoryEmbeddingsServicePublic (and all
// upstream tests) use BraveHistoryEmbeddingsService's override of
// OnPassageVisibilityCalculated. The blue-paint rule prevents recursive
// expansion: HistoryEmbeddingsService inside the template parameter
// stays as-is.
#define HistoryEmbeddingsService \
  BraveHistoryEmbeddingsService<HistoryEmbeddingsService>

#include <components/history_embeddings/content/history_embeddings_service_unittest.cc>

#undef HistoryEmbeddingsService

namespace history_embeddings {

// Brave-specific: verify that search results are preserved even when the
// visibility model is unavailable. Upstream would clear all results
// because DeterminePassageVisibility passes empty annotation_results to
// OnPassageVisibilityCalculated, which calls scored_url_rows.clear().
// Our override synthesizes passing scores (1.0) so results survive.
TEST_F(HistoryEmbeddingsServiceTest,
       BraveSearchPreservesResultsWithoutVisibilityModel) {
  // Deliberately do NOT call OverrideVisibilityScoresForTesting —
  // the visibility model is unavailable.
  AddTestHistoryPage("http://test1.com");
  OnPassagesEmbeddingsComputed(
      UrlData(1, 1, base::Time::Now()),
      {"test passage with five words", "another test passage here now"},
      {Embedding(std::vector<float>(768, 1.0f)),
       Embedding(std::vector<float>(768, 1.0f))},
      ComputeEmbeddingsStatus::kSuccess);

  // Lower the score threshold so our test embeddings pass.
  SetMetadataScoreThreshold(0.01);

  base::test::TestFuture<SearchResult> future;
  service_->Search(nullptr, "test passage", {}, 3,
                   /*skip_answering=*/true, future.GetRepeatingCallback());
  SearchResult result = future.Take();

  // Brave: results must be preserved despite no visibility model.
  ASSERT_EQ(result.scored_url_rows.size(), 1u);
  EXPECT_EQ(result.scored_url_rows[0].scored_url.url_id, 1);
}

}  // namespace history_embeddings
