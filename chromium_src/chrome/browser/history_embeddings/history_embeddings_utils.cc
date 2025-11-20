/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"
#include "base/logging.h"

// Override functions to enable history embeddings worldwide
#define IsHistoryEmbeddingsFeatureEnabled \
  IsHistoryEmbeddingsFeatureEnabled_ChromiumImpl
#define IsHistoryEmbeddingsEnabledForProfile \
  IsHistoryEmbeddingsEnabledForProfile_ChromiumImpl

#include <chrome/browser/history_embeddings/history_embeddings_utils.cc>

#undef IsHistoryEmbeddingsEnabledForProfile
#undef IsHistoryEmbeddingsFeatureEnabled

namespace history_embeddings {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kLaunchedHistoryEmbeddings, base::FEATURE_ENABLED_BY_DEFAULT},
}});

// Brave: Enable worldwide, always on
bool IsHistoryEmbeddingsFeatureEnabled() {
  return true;
}

// Brave: Enable for all profiles, bypassing OptimizationGuide checks
bool IsHistoryEmbeddingsEnabledForProfile(Profile* profile) {
  return true;
}

}  // namespace history_embeddings
