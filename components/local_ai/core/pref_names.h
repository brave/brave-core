/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_PREF_NAMES_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_PREF_NAMES_H_

class PrefRegistrySimple;

namespace local_ai::prefs {

// Profile-scoped: per-profile history embeddings on/off. Surfaced as the
// chrome://history side bar toggle.
inline constexpr char kBraveHistoryEmbeddingsEnabled[] =
    "brave.history_embeddings_enabled";

// Local-state-scoped: umbrella master switch for all local AI features.
// Surfaced as the Brave Origin Settings "Local AI" toggle. When false,
// every local AI surface (history embeddings UI, the brave-history-embeddings
// chrome://flags entry, the EmbeddingGemma model component install, ...) is
// dormant regardless of the per-profile toggle.
inline constexpr char kBraveLocalAIEnabled[] = "brave.local_ai_enabled";

void RegisterProfilePrefs(PrefRegistrySimple* registry);
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

}  // namespace local_ai::prefs

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_PREF_NAMES_H_
