/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/pref_service_syncable.h"

namespace brave_wallet {

// static
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(
      kBraveWalletWeb3Provider,
      static_cast<int>(brave_wallet::IsNativeWalletEnabled()
                           ? brave_wallet::Web3ProviderTypes::BRAVE_WALLET
                           : brave_wallet::Web3ProviderTypes::ASK));

  registry->RegisterBooleanPref(kShowWalletIconOnToolbar, true);

  registry->RegisterDictionaryPref(kBraveWalletTransactions);

  registry->RegisterTimePref(kBraveWalletLastUnlockTime, base::Time());
  registry->RegisterDictionaryPref(kBraveWalletKeyrings);
  registry->RegisterListPref(kBraveWalletCustomNetworks);
  registry->RegisterStringPref(kBraveWalletCurrentChainId,
                               brave_wallet::mojom::kMainnetChainId);
}

// static
void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  // Added 08/2021
  registry->RegisterStringPref(kBraveWalletPasswordEncryptorSalt, "");
  registry->RegisterStringPref(kBraveWalletPasswordEncryptorNonce, "");
  registry->RegisterStringPref(kBraveWalletEncryptedMnemonic, "");
  registry->RegisterIntegerPref(kBraveWalletDefaultKeyringAccountNum, 0);
  registry->RegisterBooleanPref(kBraveWalletBackupComplete, false);
  registry->RegisterListPref(kBraveWalletAccountNames);
}

}  // namespace brave_wallet
