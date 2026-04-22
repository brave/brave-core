/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Override IsHistoryEmbeddingsEnabledForProfile() to bypass upstream's
// OptimizationGuide pref checks that we don't use, and force
// IsHistoryEmbeddingsSettingVisible() to false so the chrome://settings/ai
// History Search entry (and, transitively, the AI settings page) is not
// surfaced. The disclaimer IDS swap for this file is handled in
// brave/rewrite/chrome/browser/history_embeddings/history_embeddings_utils.cc.toml;
// brave_generated_resources.h below supplies the IDS_BRAVE_* definitions
// referenced after the swap.
#include "base/feature_list.h"
#include "base/feature_override.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/history_embeddings/core/history_embeddings_features.h"

#define IsHistoryEmbeddingsEnabledForProfile \
  IsHistoryEmbeddingsEnabledForProfile_ChromiumImpl
#define IsHistoryEmbeddingsSettingVisible \
  IsHistoryEmbeddingsSettingVisible_ChromiumImpl

#include <chrome/browser/history_embeddings/history_embeddings_utils.cc>

#undef IsHistoryEmbeddingsEnabledForProfile
#undef IsHistoryEmbeddingsSettingVisible

namespace history_embeddings {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kLaunchedHistoryEmbeddings, base::FEATURE_DISABLED_BY_DEFAULT},
}});

bool IsHistoryEmbeddingsEnabledForProfile(Profile* profile) {
  return IsHistoryEmbeddingsFeatureEnabled();
}

bool IsHistoryEmbeddingsSettingVisible(Profile* profile) {
  return false;
}

}  // namespace history_embeddings
