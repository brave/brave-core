/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/pref_names.h"

#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/utils.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace local_ai::prefs {

namespace {

// Added 2026-11. Whether the user enabled the English model. Replaced by
// `kOnDeviceSpeechModel`.
constexpr char kObsoleteOnDeviceSpeechEnglishModelEnabled[] =
    "brave.local_ai.on_device_speech_model_enabled";

}  // namespace

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kBraveHistoryEmbeddingsEnabled, false);
  registry->RegisterBooleanPref(kAskEnableOnDeviceSpeechModel, true);
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kBraveLocalAIEnabled, true);
  registry->RegisterListPref(kHistoryEmbeddingsEnabledProfiles);
  // No model.
  registry->RegisterIntegerPref(kOnDeviceSpeechModel, 0);
}

void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
  // Added 2026-11.
  registry->RegisterBooleanPref(kObsoleteOnDeviceSpeechEnglishModelEnabled,
                                false);
}

void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // Added 2026-11. Only the English model had a pref that a user could
  // set. Nothing set the pref of the multilingual model.
  if (local_state->GetBoolean(kObsoleteOnDeviceSpeechEnglishModelEnabled)) {
    EnableOnDeviceSpeechModel(*local_state, OnDeviceSpeechModel::kEnglish);
  }
  local_state->ClearPref(kObsoleteOnDeviceSpeechEnglishModelEnabled);
}

}  // namespace local_ai::prefs
