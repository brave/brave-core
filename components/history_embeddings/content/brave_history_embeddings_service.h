/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_BRAVE_HISTORY_EMBEDDINGS_SERVICE_H_
#define BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_BRAVE_HISTORY_EMBEDDINGS_SERVICE_H_

#include <vector>

#include "components/history_embeddings/content/history_embeddings_service.h"
#include "components/page_content_annotations/core/page_content_annotations_common.h"

namespace history_embeddings {

// Brave specialization that skips upstream's visibility filtering.
// Brave doesn't use PageContentAnnotationsService, so the visibility
// model is never available and annotation_results would always be empty,
// causing upstream to clear all search results.
//
// Uses the template pattern (see docs/gni_sources.md) so this target
// does not need a dep on //chrome/browser:browser — the template is
// instantiated at the factory site which already has that dep.
template <typename BaseClass>
class BraveHistoryEmbeddingsService : public BaseClass {
 public:
  using BaseClass::BaseClass;
  ~BraveHistoryEmbeddingsService() override = default;

 protected:
  // HistoryEmbeddingsService:
  void OnPassageVisibilityCalculated(
      SearchResultCallback callback,
      SearchResult result,
      std::vector<ScoredUrlRow> scored_url_rows,
      const std::vector<page_content_annotations::BatchAnnotationResult>&
          annotation_results) override {
    // Always synthesize passing scores (1.0) so upstream keeps all results
    // regardless of what annotation_results contains.
    std::vector<page_content_annotations::BatchAnnotationResult> passing;
    passing.reserve(scored_url_rows.size());
    for (const auto& url_row : scored_url_rows) {
      passing.push_back(
          page_content_annotations::BatchAnnotationResult::
              CreateContentVisibilityResult(url_row.GetBestPassage(), 1.0));
    }
    HistoryEmbeddingsService::OnPassageVisibilityCalculated(
        std::move(callback), std::move(result), std::move(scored_url_rows),
        passing);
  }
};

}  // namespace history_embeddings

#endif  // BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_BRAVE_HISTORY_EMBEDDINGS_SERVICE_H_
