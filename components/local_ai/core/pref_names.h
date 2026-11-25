/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_PREF_NAMES_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_PREF_NAMES_H_

class PrefRegistrySimple;
class PrefService;

namespace local_ai::prefs {

// Profile-scoped: per-profile history embeddings on/off. Surfaced as the
// "Semantic history search" toggle in brave://settings/privacy.
inline constexpr char kBraveHistoryEmbeddingsEnabled[] =
    "brave.history_embeddings_enabled";

// Local-state-scoped: base names of the profiles whose
// `kBraveHistoryEmbeddingsEnabled` is on, kept so a profile that is not loaded
// still counts. The EmbeddingGemma model is shared by every profile, and is
// only registered for download and update while this is not empty.
inline constexpr char kHistoryEmbeddingsEnabledProfiles[] =
    "brave.local_ai.history_embeddings_enabled_profiles";

// Local-state-scoped: umbrella master switch for all local AI features.
// Surfaced as the Brave Origin Settings "Local AI" toggle. When false,
// every local AI surface (history embeddings UI, the brave-history-embeddings
// chrome://flags entry, the EmbeddingGemma model component install, ...) is
// dormant regardless of the per-profile toggle.
inline constexpr char kBraveLocalAIEnabled[] = "brave.local_ai_enabled";

// Profile-scoped: whether to ask this user before downloading the on-device
// speech recognition model. False once the user blocks the prompt with "Don't
// ask again". One person's choice, so it does not follow the model, which every
// profile shares.
inline constexpr char kAskEnableOnDeviceSpeechModel[] =
    "brave.local_ai.ask_enable_on_device_speech_model";

// Local-state-scoped: the on-device speech recognition model that the user
// enabled, as the value of an `OnDeviceSpeechModel`, or 0 when the user enabled
// none. Allowing a download or choosing a model in settings sets it. A user
// enables at most one model: the English model serves only English, and the
// multilingual model serves English and the other languages. Only the model
// that this names is registered for download and update. Cleared when
// on-device speech recognition is no longer allowed, or when
// `kBraveOnDeviceSpeechMultilingual` is disabled while it names the
// multilingual model. Thus turning them back on asks again rather than
// downloading.
inline constexpr char kOnDeviceSpeechModel[] =
    "brave.local_ai.on_device_speech_model";

void RegisterProfilePrefs(PrefRegistrySimple* registry);
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

// The local-state prefs that `MigrateObsoleteLocalStatePrefs` reads.
void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry);
void MigrateObsoleteLocalStatePrefs(PrefService* local_state);

}  // namespace local_ai::prefs

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_PREF_NAMES_H_
