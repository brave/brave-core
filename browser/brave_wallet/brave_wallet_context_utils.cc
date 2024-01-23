/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

namespace brave_wallet {

bool IsAllowedForContext(content::BrowserContext* context) {
  if (!context) {
    return false;
  }

  if (!IsAllowed(user_prefs::UserPrefs::Get(context))) {
    return false;
  }

  auto* profile = Profile::FromBrowserContext(context);
  if (!brave::IsRegularProfile(context)) {
    // If it's not a regular profile, then it's only allowed if it's a
    // non-Tor incognito profile AND the user has enabled brave wallet in
    // private tabs.
    if (context->IsTor()) {
      return false;
    }

    if (profile->IsIncognitoProfile()) {
      auto* prefs = user_prefs::UserPrefs::Get(context);
      return prefs->GetBoolean(kBraveWalletPrivateWindowsEnabled);
    }

    return false;
  }

  return true;
}

}  // namespace brave_wallet
