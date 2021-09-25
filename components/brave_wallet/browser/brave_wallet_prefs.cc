/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"

#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/pref_service_syncable.h"

namespace {

base::Value GetDefaultUserAssets() {
  // Show ETH and BAT by default for mainnet.
  base::Value user_assets_pref(base::Value::Type::DICTIONARY);
  base::Value* user_assets_dict =
      user_assets_pref.SetKey("mainnet", base::Value(base::Value::Type::LIST));

  base::Value eth(base::Value::Type::DICTIONARY);
  eth.SetKey("contract_address", base::Value("eth"));
  eth.SetKey("name", base::Value("Ethereum"));
  eth.SetKey("symbol", base::Value("ETH"));
  eth.SetKey("is_erc20", base::Value(false));
  eth.SetKey("is_erc721", base::Value(false));
  eth.SetKey("decimals", base::Value(18));
  eth.SetKey("visible", base::Value(true));
  user_assets_dict->Append(std::move(eth));

  base::Value bat(base::Value::Type::DICTIONARY);
  bat.SetKey("contract_address",
             base::Value("0x0D8775F648430679A709E98d2b0Cb6250d2887EF"));
  bat.SetKey("name", base::Value("Basic Attention Token"));
  bat.SetKey("symbol", base::Value("BAT"));
  bat.SetKey("is_erc20", base::Value(true));
  bat.SetKey("is_erc721", base::Value(false));
  bat.SetKey("decimals", base::Value(18));
  bat.SetKey("visible", base::Value(true));
  user_assets_dict->Append(std::move(bat));

  return user_assets_pref;
}

}  // namespace

namespace brave_wallet {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(
      kBraveWalletWeb3Provider,
      static_cast<int>(brave_wallet::IsNativeWalletEnabled()
                           ? brave_wallet::mojom::DefaultWallet::BraveWallet
                           : brave_wallet::mojom::DefaultWallet::Ask));

  registry->RegisterBooleanPref(kShowWalletIconOnToolbar, true);

  registry->RegisterDictionaryPref(kBraveWalletTransactions);

  registry->RegisterTimePref(kBraveWalletLastUnlockTime, base::Time());
  registry->RegisterDictionaryPref(kBraveWalletKeyrings);
  registry->RegisterListPref(kBraveWalletCustomNetworks);
  registry->RegisterStringPref(kBraveWalletCurrentChainId,
                               brave_wallet::mojom::kMainnetChainId);
  registry->RegisterDictionaryPref(kBraveWalletUserAssets,
                                   GetDefaultUserAssets());
}

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

void ClearProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletCustomNetworks);
  prefs->ClearPref(kBraveWalletCurrentChainId);
  prefs->ClearPref(kBraveWalletTransactions);
  prefs->ClearPref(kBraveWalletUserAssets);
  prefs->ClearPref(kBraveWalletKeyrings);
}

}  // namespace brave_wallet
