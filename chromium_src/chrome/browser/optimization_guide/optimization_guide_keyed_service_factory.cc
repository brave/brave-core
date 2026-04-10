/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Override IsOptimizationHintsEnabled to also return true when
// kHistoryEmbeddings is enabled, so GetForProfile returns a non-null
// service. This allows PageContentAnnotationsService to construct,
// enabling the upstream page content extraction pipeline. The service
// itself is safe to use because Brave blocks remote fetching via
// IsUserPermittedToFetchFromRemoteOptimizationGuide returning false.

#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/optimization_guide/core/optimization_guide_features.h"

namespace optimization_guide::features {

bool IsOptimizationHintsEnabled_Brave() {
  return IsOptimizationHintsEnabled() ||
         base::FeatureList::IsEnabled(history_embeddings::kHistoryEmbeddings);
}

}  // namespace optimization_guide::features

#define IsOptimizationHintsEnabled IsOptimizationHintsEnabled_Brave

#include <chrome/browser/optimization_guide/optimization_guide_keyed_service_factory.cc>

#undef IsOptimizationHintsEnabled
