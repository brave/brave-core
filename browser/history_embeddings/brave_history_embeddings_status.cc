/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/history_embeddings/brave_history_embeddings_status.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

namespace history_embeddings {

namespace {

constexpr char kBraveHistoryEmbeddingsStatusKey[] =
    "brave_history_embeddings_status";

}  // namespace

BraveHistoryEmbeddingsStatus::BraveHistoryEmbeddingsStatus(Profile* profile,
                                                           bool enabled)
    : profile_(profile), enabled_(enabled) {
  pref_change_registrar_.Init(profile_->GetPrefs());
  pref_change_registrar_.Add(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled,
      base::BindRepeating(&BraveHistoryEmbeddingsStatus::OnEnabledPrefChanged,
                          base::Unretained(this)));
}

void BraveHistoryEmbeddingsStatus::OnEnabledPrefChanged() {
#if BUILDFLAG(ENABLE_AI_CHAT)
  // The pref is only registered when the AI Chat feature is on at runtime,
  // and clearing an unregistered pref is fatal.
  if (!ai_chat::features::IsAIChatEnabled()) {
    return;
  }
  if (profile_->GetPrefs()->GetBoolean(
          local_ai::prefs::kBraveHistoryEmbeddingsEnabled)) {
    return;
  }
  profile_->GetPrefs()->ClearPref(
      ai_chat::prefs::kBraveAIChatTabOrganizationSendPageContent);
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
}

// static
void BraveHistoryEmbeddingsStatus::CreateForProfile(Profile* profile) {
  if (profile->GetUserData(kBraveHistoryEmbeddingsStatusKey)) {
    return;
  }
  // Object cleanup is handled by SupportsUserData
  profile->SetUserData(
      kBraveHistoryEmbeddingsStatusKey,
      std::make_unique<BraveHistoryEmbeddingsStatus>(
          profile, IsHistoryEmbeddingsEnabledForProfile(profile)));
}

// static
BraveHistoryEmbeddingsStatus* BraveHistoryEmbeddingsStatus::GetForProfile(
    Profile* profile) {
  CreateForProfile(profile);
  return static_cast<BraveHistoryEmbeddingsStatus*>(
      profile->GetUserData(kBraveHistoryEmbeddingsStatusKey));
}

bool BraveHistoryEmbeddingsStatus::IsEnabled() const {
  return enabled_;
}

bool BraveHistoryEmbeddingsStatus::NeedsRestart() const {
  return IsHistoryEmbeddingsEnabledForProfile(profile_) != enabled_;
}

}  // namespace history_embeddings
