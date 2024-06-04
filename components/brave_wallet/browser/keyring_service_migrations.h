/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_MIGRATIONS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_MIGRATIONS_H_

#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/pref_service.h"

namespace brave_wallet {

void MigrateDerivedAccountIndex(PrefService* profile_prefs);
void MaybeMigrateSelectedAccountPrefs(
    PrefService* profile_prefs,
    const std::vector<mojom::AccountInfoPtr>& all_accounts);
void MaybeRunPasswordMigrations(PrefService* profile_prefs,
                                const std::string& password);
void MaybeMigratePBKDF2Iterations(PrefService* profile_prefs,
                                  const std::string& password);
void MaybeMigrateToWalletMnemonic(PrefService* profile_prefs,
                                  const std::string& password);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_MIGRATIONS_H_
