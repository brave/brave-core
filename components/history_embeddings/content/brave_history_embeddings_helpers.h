// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_BRAVE_HISTORY_EMBEDDINGS_HELPERS_H_
#define BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_BRAVE_HISTORY_EMBEDDINGS_HELPERS_H_

#include <vector>

#include "components/history_embeddings/core/history_embeddings_search.h"
#include "components/page_content_annotations/core/page_content_annotations_common.h"

namespace history_embeddings {

// Returns a visibility annotation for every input row with a passing
// score (1.0). Brave doesn't run PageContentAnnotationsService, so
// upstream's HistoryEmbeddingsService::OnPassageVisibilityCalculated
// receives empty annotations and would clear scored_url_rows. Brave
// subclasses substitute synthesized passing annotations so the base
// takes its "filter by score" branch with every row above the
// threshold. Shared between the production subclass in
// brave/browser/history_embeddings/ and the test subclass in
// brave/chromium_src/components/history_embeddings/content/.
inline std::vector<page_content_annotations::BatchAnnotationResult>
SynthesizePassingVisibilityResults(
    const std::vector<ScoredUrlRow>& scored_url_rows) {
  std::vector<page_content_annotations::BatchAnnotationResult> passing;
  passing.reserve(scored_url_rows.size());
  for (const auto& url_row : scored_url_rows) {
    passing.push_back(
        page_content_annotations::BatchAnnotationResult::
            CreateContentVisibilityResult(url_row.GetBestPassage(), 1.0));
  }
  return passing;
}

}  // namespace history_embeddings

#endif  // BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_BRAVE_HISTORY_EMBEDDINGS_HELPERS_H_
