/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Override IsHistoryEmbeddingsEnabledForProfile() to bypass upstream's
// OptimizationGuide pref checks that we don't use.
#include "base/feature_list.h"
#include "base/feature_override.h"
#include "components/history_embeddings/history_embeddings_features.h"

#define IsHistoryEmbeddingsFeatureEnabled \
  IsHistoryEmbeddingsFeatureEnabled_ChromiumImpl
#define IsHistoryEmbeddingsEnabledForProfile \
  IsHistoryEmbeddingsEnabledForProfile_ChromiumImpl

#include <chrome/browser/history_embeddings/history_embeddings_utils.cc>

#undef IsHistoryEmbeddingsEnabledForProfile
#undef IsHistoryEmbeddingsFeatureEnabled

namespace history_embeddings {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kLaunchedHistoryEmbeddings, base::FEATURE_DISABLED_BY_DEFAULT},
}});

bool IsHistoryEmbeddingsFeatureEnabled() {
  return base::FeatureList::IsEnabled(kHistoryEmbeddings);
}

bool IsHistoryEmbeddingsEnabledForProfile(Profile* profile) {
  return IsHistoryEmbeddingsFeatureEnabled();
}

}  // namespace history_embeddings
