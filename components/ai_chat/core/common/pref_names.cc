/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/pref_names.h"

#include <string_view>

#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "components/prefs/pref_registry_simple.h"

namespace ai_chat::prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  if (ai_chat::features::IsAIChatEnabled()) {
    registry->RegisterTimePref(kLastAcceptedDisclaimer, {});
    registry->RegisterBooleanPref(kBraveChatAutocompleteProviderEnabled, true);
    registry->RegisterBooleanPref(kUserDismissedPremiumPrompt, false);
#if BUILDFLAG(IS_ANDROID)
    registry->RegisterBooleanPref(kBraveChatSubscriptionActiveAndroid, false);
    registry->RegisterStringPref(kBraveChatPurchaseTokenAndroid, "");
    registry->RegisterStringPref(kBraveChatPackageNameAndroid, "");
    registry->RegisterStringPref(kBraveChatProductIdAndroid, "");
    registry->RegisterStringPref(kBraveChatOrderIdAndroid, "");
#endif
    registry->RegisterBooleanPref(kBraveAIChatContextMenuEnabled, true);
    registry->RegisterBooleanPref(kBraveAIChatShowToolbarButton, true);
  }
  registry->RegisterBooleanPref(kEnabledByPolicy, true);
}

void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry) {
  if (ai_chat::features::IsAIChatEnabled()) {
    registry->RegisterBooleanPref(kObseleteBraveChatAutoGenerateQuestions,
                                  false);
  }
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  // Added 11/2023
  registry->RegisterDictionaryPref(kBraveChatPremiumCredentialCache);
}

}  // namespace ai_chat::prefs
