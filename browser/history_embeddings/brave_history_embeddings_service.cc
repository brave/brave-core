// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_history_embeddings_service.h"

#include <utility>

#include "brave/components/history_embeddings/content/brave_history_embeddings_helpers.h"

namespace history_embeddings {

BraveHistoryEmbeddingsService::~BraveHistoryEmbeddingsService() = default;

void BraveHistoryEmbeddingsService::OnPassageVisibilityCalculated(
    SearchResultCallback callback,
    SearchResult result,
    std::vector<ScoredUrlRow> scored_url_rows,
    const std::vector<page_content_annotations::BatchAnnotationResult>&
        annotation_results) {
  auto passing = SynthesizePassingVisibilityResults(scored_url_rows);
  HistoryEmbeddingsService::OnPassageVisibilityCalculated(
      std::move(callback), std::move(result), std::move(scored_url_rows),
      passing);
}

}  // namespace history_embeddings
