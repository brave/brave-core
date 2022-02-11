/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PREFS_H_

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave_wallet {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry);
void ClearJsonRpcServiceProfilePrefs(PrefService* prefs);
void ClearKeyringServiceProfilePrefs(PrefService* prefs);
void ClearTxServiceProfilePrefs(PrefService* prefs);
void ClearBraveWalletServicePrefs(PrefService* prefs);
void MigrateObsoleteProfilePrefs(PrefService* prefs);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PREFS_H_
