/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/pref_names.h"

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace ai_chat::prefs {

namespace {

inline constexpr char kObseleteBraveChatAutoGenerateQuestions[] =
    "brave.ai_chat.auto_generate_questions";

}  // namespace

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kLastAcceptedDisclaimer, {});
  registry->RegisterBooleanPref(kBraveChatAutocompleteProviderEnabled, true);
  registry->RegisterBooleanPref(kUserDismissedPremiumPrompt, false);
  registry->RegisterStringPref(kDefaultModelKey, "chat-default");
}

void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kObseleteBraveChatAutoGenerateQuestions, false);
}

void MigrateProfilePrefs(PrefService* profile_prefs) {
  profile_prefs->ClearPref(kObseleteBraveChatAutoGenerateQuestions);
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  // Added 11/2023
  registry->RegisterDictionaryPref(kBraveChatPremiumCredentialCache);
}

}  // namespace ai_chat::prefs
