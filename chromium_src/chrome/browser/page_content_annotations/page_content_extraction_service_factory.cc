/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The feature check that the plaster puts in place of upstream's: it enables
// the PageContentExtractionService when history embeddings is active.

#include "brave/components/local_ai/buildflags/buildflags.h"
#include "components/page_content_annotations/core/page_content_annotations_features.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#endif

namespace page_content_annotations::features {

inline bool ShouldEnablePageContentAnnotations_Brave() {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  return base::FeatureList::IsEnabled(history_embeddings::kHistoryEmbeddings);
#else
  return false;
#endif
}

}  // namespace page_content_annotations::features

#include <chrome/browser/page_content_annotations/page_content_extraction_service_factory.cc>
