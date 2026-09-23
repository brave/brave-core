// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_HISTORY_EMBEDDINGS_SERVICE_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_HISTORY_EMBEDDINGS_SERVICE_H_

#include <vector>

#include "chrome/browser/history_embeddings/chrome_history_embeddings_service.h"
#include "components/page_content_annotations/core/page_content_annotations_common.h"

namespace history_embeddings {

// Brave specialization that skips upstream's visibility filtering.
// See brave_history_embeddings_helpers.h for the shared
// SynthesizePassingVisibilityResults helper used by both this production
// subclass and the test subclass in the chromium_src unittest override.
class BraveHistoryEmbeddingsService : public ChromeHistoryEmbeddingsService {
 public:
  using ChromeHistoryEmbeddingsService::ChromeHistoryEmbeddingsService;
  ~BraveHistoryEmbeddingsService() override;

 protected:
  // HistoryEmbeddingsService (virtualized via chromium_src shim):
  void OnPassageVisibilityCalculated(
      SearchResultCallback callback,
      SearchResult result,
      std::vector<ScoredUrlRow> scored_url_rows,
      const std::vector<page_content_annotations::BatchAnnotationResult>&
          annotation_results) override;
};

}  // namespace history_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_HISTORY_EMBEDDINGS_SERVICE_H_
