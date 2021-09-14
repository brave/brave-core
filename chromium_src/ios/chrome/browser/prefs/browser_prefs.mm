/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "components/pref_registry/pref_registry_syncable.h"

void BraveRegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  brave_sync::Prefs::RegisterProfilePrefs(registry);
  brave_wallet::RegisterProfilePrefs(registry);
}

#define BRAVE_REGISTER_BROWSER_STATE_PREFS BraveRegisterBrowserStatePrefs(registry);
#include "../../../../../../ios/chrome/browser/prefs/browser_prefs.mm"
