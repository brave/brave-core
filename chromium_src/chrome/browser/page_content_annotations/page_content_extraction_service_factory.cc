/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Override ShouldEnablePageContentAnnotations to check kHistoryEmbeddings
// instead of the upstream feature flags, enabling the upstream
// PageContentExtractionService when history embeddings is active.

#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/page_content_annotations/core/page_content_annotations_features.h"

namespace page_content_annotations::features {

inline bool ShouldEnablePageContentAnnotations_Brave() {
  return base::FeatureList::IsEnabled(history_embeddings::kHistoryEmbeddings);
}

}  // namespace page_content_annotations::features

#define ShouldEnablePageContentAnnotations \
  ShouldEnablePageContentAnnotations_Brave

#include <chrome/browser/page_content_annotations/page_content_extraction_service_factory.cc>

#undef ShouldEnablePageContentAnnotations
