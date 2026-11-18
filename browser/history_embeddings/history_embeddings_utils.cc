// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/history_embeddings_utils.h"

#include "base/feature_list.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace brave_history_embeddings {

bool IsHistoryEmbeddingsFeatureEnabled() {
  // The Local AI master switch lives in local state. Unit tests may run
  // without one, in which case only the feature flag applies.
  PrefService* local_state =
      g_browser_process ? g_browser_process->local_state() : nullptr;
  if (local_state &&
      !local_state->GetBoolean(local_ai::prefs::kBraveLocalAIEnabled)) {
    return false;
  }
  return base::FeatureList::IsEnabled(kHistoryEmbeddings);
}

bool IsHistoryEmbeddingsEnabledForProfile(Profile* profile) {
  return IsHistoryEmbeddingsFeatureEnabled() &&
         profile->GetPrefs()->GetBoolean(
             local_ai::prefs::kBraveHistoryEmbeddingsEnabled);
}

}  // namespace brave_history_embeddings
