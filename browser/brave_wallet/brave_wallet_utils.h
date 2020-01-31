/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_UTILS_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_UTILS_H_

class Profile;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave_wallet {

void MigrateBraveWalletPrefs(Profile* profile);
void RegisterBraveWalletProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry);

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_UTILS_H_
