/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_PREF_NAMES_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_PREF_NAMES_H_

class PrefRegistrySimple;

namespace local_ai::prefs {

inline constexpr char kBraveHistoryEmbeddingsEnabled[] =
    "brave.history_embeddings_enabled";

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace local_ai::prefs

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_PREF_NAMES_H_
