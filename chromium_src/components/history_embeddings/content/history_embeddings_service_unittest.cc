/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/history_embeddings/content/history_embeddings_service.h"

#include <utility>
#include <vector>

#include "base/test/test_future.h"
#include "brave/components/history_embeddings/content/brave_history_embeddings_helpers.h"
#include "components/page_content_annotations/core/page_content_annotations_common.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace history_embeddings {

// Test-only subclass that inherits from the base HistoryEmbeddingsService
// (so upstream's lightweight test fixture — which passes os_crypt_async*
// instead of Profile* — can construct it) and reuses the production
// BraveHistoryEmbeddingsService's SynthesizePassingVisibilityResults
// helper so the override behavior under test matches production.
class BraveTestHistoryEmbeddingsService : public HistoryEmbeddingsService {
 public:
  using HistoryEmbeddingsService::HistoryEmbeddingsService;

 protected:
  void OnPassageVisibilityCalculated(
      SearchResultCallback callback,
      SearchResult result,
      std::vector<ScoredUrlRow> scored_url_rows,
      const std::vector<page_content_annotations::BatchAnnotationResult>&
          annotation_results) override {
    auto passing = SynthesizePassingVisibilityResults(scored_url_rows);
    HistoryEmbeddingsService::OnPassageVisibilityCalculated(
        std::move(callback), std::move(result), std::move(scored_url_rows),
        passing);
  }
};

}  // namespace history_embeddings

// Swap the instantiated class so HistoryEmbeddingsServicePublic (and
// every upstream test that derives from it) exercises Brave's override.
#define HistoryEmbeddingsService BraveTestHistoryEmbeddingsService

#include <components/history_embeddings/content/history_embeddings_service_unittest.cc>

#undef HistoryEmbeddingsService

namespace history_embeddings {

// Brave-specific: verify that search results are preserved even when
// the visibility model is unavailable. Upstream would clear all
// results because DeterminePassageVisibility passes empty
// annotation_results to OnPassageVisibilityCalculated, which calls
// scored_url_rows.clear(). Our override synthesizes passing scores
// (1.0) so results survive.
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
