/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Override IsHistoryEmbeddingsEnabledForProfile() to bypass upstream's
// OptimizationGuide pref checks (we don't use OptimizationGuide for this) and
// instead gate on a Brave-owned boolean pref toggled from the Leo settings
// page. IsHistoryEmbeddingsSettingVisible() is forced to false so the
// chrome://settings/ai History Search entry (and, transitively, the AI
// settings page) is not surfaced; the toggle lives on the Leo page. The
// disclaimer IDS swap for this file is handled in
// brave/rewrite/chrome/browser/history_embeddings/history_embeddings_utils.cc.toml;
// brave_generated_resources.h below supplies the IDS_BRAVE_* definitions
// referenced after the swap.
#include "base/feature_list.h"
#include "base/feature_override.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/local_ai/core/pref_names.h"
#endif

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
#if BUILDFLAG(ENABLE_LOCAL_AI)
  if (!IsHistoryEmbeddingsFeatureEnabled()) {
    return false;
  }
  return profile->GetPrefs()->GetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled);
#else
  return false;
#endif
}

bool IsHistoryEmbeddingsSettingVisible(Profile* profile) {
  return false;
}

}  // namespace history_embeddings
