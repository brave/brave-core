/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/common/pref_names.h"

#include "components/prefs/pref_registry_simple.h"

namespace ai_chat::prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kLastAcceptedDisclaimer, {});
  registry->RegisterBooleanPref(kBraveChatAutoGenerateQuestions, false);
  registry->RegisterBooleanPref(kBraveChatAutocompleteProviderEnabled, true);
  registry->RegisterBooleanPref(kUserDismissedPremiumPrompt, false);
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kBraveChatPremiumCredentialCache);
}

}  // namespace ai_chat::prefs
