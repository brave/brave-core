/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/history_embeddings/history_embeddings_utils.h"

#include "brave/components/local_ai/buildflags/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/local_ai/core/pref_names.h"
#endif

namespace history_embeddings {

namespace {

// Whether IsHistoryEmbeddingsFeatureEnabled() should short-circuit to false
// ahead of upstream's own check, per the Brave Origin Settings "Local AI"
// master switch.
bool ShouldForceHistoryEmbeddingsFeatureDisabled() {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  PrefService* local_state =
      g_browser_process ? g_browser_process->local_state() : nullptr;
  return local_state &&
         !local_state->GetBoolean(local_ai::prefs::kBraveLocalAIEnabled);
#else
  return true;
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)
}

// Replaces IsHistoryEmbeddingsEnabledForProfile()'s upstream body: gates it on
// the per-profile kBraveHistoryEmbeddingsEnabled pref backing the
// chrome://history side bar toggle.
bool ComputeHistoryEmbeddingsEnabledForProfile(Profile* profile) {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  if (!IsHistoryEmbeddingsFeatureEnabled()) {
    return false;
  }
  return profile->GetPrefs()->GetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled);
#else
  return false;
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)
}

}  // namespace

}  // namespace history_embeddings

#include <chrome/browser/history_embeddings/history_embeddings_utils.cc>

namespace history_embeddings {

namespace {

// kEnabledByDefaultForDesktopOnly is left unused because we disable
// `kLaunchedHistoryEmbeddings` with a plaster, so we suppress the unused
// variable warning with this.
[[maybe_unused]] constexpr auto& kUnusedEnabledByDefaultForDesktopOnly =
    kEnabledByDefaultForDesktopOnly;

}  // namespace

}  // namespace history_embeddings
