/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Gate IsHistoryEmbeddingsFeatureEnabled() on the local-state
// `kBraveLocalAIEnabled` master switch (Brave Origin Settings "Local AI"
// toggle) so every upstream consumer that reaches the feature through this
// chokepoint — model component installer, side bar toggle visibility,
// history embeddings service factory, etc. — folds off when the master
// switch is off. IsHistoryEmbeddingsEnabledForProfile() is additionally
// gated on the per-profile `kBraveHistoryEmbeddingsEnabled` pref backing
// the chrome://history side bar toggle. IsHistoryEmbeddingsSettingVisible()
// is forced to false so the chrome://settings/ai History Search entry is
// not surfaced; the per-profile toggle lives on chrome://history.
#include "base/feature_list.h"
#include "base/feature_override.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/local_ai/core/pref_names.h"
#endif

#define IsHistoryEmbeddingsFeatureEnabled \
  IsHistoryEmbeddingsFeatureEnabled_ChromiumImpl
#define IsHistoryEmbeddingsEnabledForProfile \
  IsHistoryEmbeddingsEnabledForProfile_ChromiumImpl
#define IsHistoryEmbeddingsSettingVisible \
  IsHistoryEmbeddingsSettingVisible_ChromiumImpl

#include <chrome/browser/history_embeddings/history_embeddings_utils.cc>

#undef IsHistoryEmbeddingsFeatureEnabled
#undef IsHistoryEmbeddingsEnabledForProfile
#undef IsHistoryEmbeddingsSettingVisible

namespace history_embeddings {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kLaunchedHistoryEmbeddings, base::FEATURE_DISABLED_BY_DEFAULT},
}});

bool IsHistoryEmbeddingsFeatureEnabled() {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  PrefService* local_state =
      g_browser_process ? g_browser_process->local_state() : nullptr;
  if (local_state &&
      !local_state->GetBoolean(local_ai::prefs::kBraveLocalAIEnabled)) {
    return false;
  }
  return IsHistoryEmbeddingsFeatureEnabled_ChromiumImpl();
#else
  return false;
#endif
}

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
