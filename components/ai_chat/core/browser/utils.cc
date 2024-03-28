/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/utils.h"

#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"

namespace ai_chat {

namespace {

bool IsDisabledByPolicy(PrefService* prefs) {
  DCHECK(prefs);
  return prefs->IsManagedPreference(prefs::kEnabledByPolicy) &&
         !prefs->GetBoolean(prefs::kEnabledByPolicy);
}

}  // namespace

bool IsAIChatEnabled(PrefService* prefs) {
  DCHECK(prefs);
  return features::IsAIChatEnabled() && !IsDisabledByPolicy(prefs);
}

bool HasUserOptedIn(PrefService* prefs) {
  if (!prefs) {
    return false;
  }

  base::Time last_accepted_disclaimer =
      prefs->GetTime(prefs::kLastAcceptedDisclaimer);
  return !last_accepted_disclaimer.is_null();
}

void SetUserOptedIn(PrefService* prefs, bool opted_in) {
  if (!prefs) {
    return;
  }

  if (opted_in) {
    prefs->SetTime(prefs::kLastAcceptedDisclaimer, base::Time::Now());
  } else {
    prefs->ClearPref(prefs::kLastAcceptedDisclaimer);
  }
}

}  // namespace ai_chat
