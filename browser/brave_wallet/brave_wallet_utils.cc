/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_utils.h"

#include "brave/common/brave_wallet_constants.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/common/constants.h"

namespace {

void MigrateBraveWalletPrefs_V0_V1(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  bool wallet_was_enabled = true;  // true was the default
  if (prefs->HasPrefPath(kBraveWalletEnabledDeprecated)) {
    wallet_was_enabled = prefs->GetBoolean(kBraveWalletEnabledDeprecated);
  }

  bool has_crypto_wallets = extensions::ExtensionPrefs::Get(profile)->
      HasPrefForExtension(ethereum_remote_client_extension_id);
  bool has_metamask = extensions::ExtensionPrefs::Get(profile)->
      HasPrefForExtension(metamask_extension_id);

  BraveWalletWeb3ProviderTypes provider =
      BraveWalletWeb3ProviderTypes::ASK;
  if (!wallet_was_enabled&& has_metamask) {
    // If Crypto Wallets was disabled and MetaMask is installed, set to MetaMask
    provider = BraveWalletWeb3ProviderTypes::METAMASK;
  } else if (!wallet_was_enabled && !has_metamask) {
    // If Crypto Wallets is diabled, and MetaMask not installed, set None
    provider = BraveWalletWeb3ProviderTypes::NONE;
  } else if (wallet_was_enabled && has_metamask) {
    // If Crypto Wallets is enabled, and MetaMask is installed, set
    // to Crypto Wallets
    provider = BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS;
  } else if (has_crypto_wallets && wallet_was_enabled) {
    // If CryptoWallets is enabled and installed, but MetaMask is not
    // installed, set Crypto Wallets.
    provider = BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS;
  } else if (!has_crypto_wallets && wallet_was_enabled) {
    // If CryptoWallets is enabled and not installed yet, and MetaMask is not
    // installed, set Ask
    provider = BraveWalletWeb3ProviderTypes::ASK;
  }
  prefs->SetInteger(kBraveWalletWeb3Provider, static_cast<int>(provider));
  prefs->ClearPref(kBraveWalletEnabledDeprecated);
  prefs->SetInteger(kBraveWalletPrefVersion, 1);
}

}  // namespace

namespace brave_wallet {

void RegisterBraveWalletProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kBraveWalletEnabledDeprecated, true);
}

void MigrateBraveWalletPrefs(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  if (prefs->GetInteger(kBraveWalletPrefVersion) == 0) {
    MigrateBraveWalletPrefs_V0_V1(profile);
  }
}

}  // namespace brave_wallet
