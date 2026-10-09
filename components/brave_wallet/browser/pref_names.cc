/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/pref_names.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "components/content_settings/core/common/content_settings.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_migrations.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

namespace {

constexpr int kDefaultWalletAutoLockMinutes = 10;

// Added 05/2026
inline constexpr char kBraveWalletEip1559ForCustomNetworksMigrated[] =
    "brave.wallet.eip1559_chains_migrated";
// Added 05/2026
inline constexpr char kBraveWalletIsCompressedNftMigrated[] =
    "brave.wallet.is_compressed_nft_migrated";
// Added 05/2026
inline constexpr char kBraveWalletAuroraMainnetMigrated[] =
    "brave.wallet.aurora_mainnet_migrated";
// Added 05/2026
inline constexpr char kBraveWalletIsSPLTokenProgramMigrated[] =
    "brave.wallet.is_spl_token_program_migrated";
// Added 05/2026
inline constexpr char kBraveWalletGoerliNetworkMigrated[] =
    "brave.wallet.custom_networks.goerli_migrated";

// Deprecated 05/2026
inline constexpr char kBraveWalletP3ANewUserBalanceReportedDeprecated[] =
    "brave.wallet.p3a_new_user_balance_reported";
// Deprecated 05/2026
inline constexpr char kBraveWalletP3AActiveWalletDictDeprecated[] =
    "brave.wallet.wallet_p3a_active_wallets";
// Deprecated 05/2026
inline constexpr char kBraveWalletLastTransactionSentTimeDictDeprecated[] =
    "brave.wallet.last_transaction_sent_time_dict";
// Deprecated 05/2026
inline constexpr char kBraveWalletP3ANFTGalleryUsedDeprecated[] =
    "brave.wallet.wallet_p3a_nft_gallery_used";
// Deprecated 05/2026
inline constexpr char kBraveWalletP3AOnboardingLastStepDeprecated[] =
    "brave.wallet.p3a_last_onboarding_step";
// Deprecated 05/2026
inline constexpr char kBraveWalletP3AFirstUnlockTimeDeprecated[] =
    "brave.wallet.p3a_first_unlock_time";
// Deprecated 05/2026
inline constexpr char kBraveWalletP3ALastUnlockTimeDeprecated[] =
    "brave.wallet.p3a_last_unlock_time";
// Deprecated 05/2026
inline constexpr char kBraveWalletP3AUsedSecondDayDeprecated[] =
    "brave.wallet.p3a_used_second_day";

// Site permissions used to be stored as plain ALLOW content settings against
// synthetic "<origin><account>" origins of these types. Accounts now live in
// the matching BRAVE_*_CHOOSER_DATA website settings and these types are
// registered as ask/block guards only, so a stored ALLOW is rejected by
// ContentSettingsPref::IsValueAllowedForType and would DCHECK as it is loaded.
inline constexpr auto kWalletPermissionSettingNames =
    std::to_array<std::string_view>(
        {"brave_ethereum", "brave_solana", "brave_cardano"});

// Matches ContentSettingsPref's stored schema:
//   { "<primary>,<secondary>": { "setting": <int>, ... }, ... }
inline constexpr char kContentSettingKey[] = "setting";

void RemoveAllowEntries(PrefService* prefs, const std::string& pref_path) {
  // Only ALLOW is obsolete. Block entries are the guard's remaining state and
  // belong to the user, so they stay.
  std::vector<std::string> obsolete_patterns;
  for (const auto entry : prefs->GetDict(pref_path)) {
    const auto* settings = entry.second.GetIfDict();
    if (settings && settings->FindInt(kContentSettingKey) ==
                        static_cast<int>(CONTENT_SETTING_ALLOW)) {
      obsolete_patterns.emplace_back(entry.first);
    }
  }

  if (obsolete_patterns.empty()) {
    return;
  }

  ScopedDictPrefUpdate update(prefs, pref_path);
  for (const auto& pattern : obsolete_patterns) {
    update->Remove(pattern);
  }
}

// Must run before HostContentSettingsMap exists, since its pref provider reads
// and validates these during construction. MigrateObsoleteProfilePrefs runs
// from ProfileImpl::OnLocaleReady, ahead of any keyed service, so a migration
// here is early enough where a keyed-service one would not be.
//
// Deliberately not gated on a one-shot flag: dropping exactly the ALLOW
// entries is idempotent and self-healing, so a downgrade that rewrites them
// cannot leave a profile that crashes on the next upgrade.
void ResetObsoleteWalletPermissions(PrefService* prefs) {
  for (const auto name : kWalletPermissionSettingNames) {
    RemoveAllowEntries(
        prefs, base::StrCat({"profile.content_settings.exceptions.", name}));
    RemoveAllowEntries(prefs,
                       base::StrCat({"profile.content_settings."
                                     "partitioned_exceptions.",
                                     name}));

    // The default is a bare int rather than a dict of exceptions.
    const auto default_path =
        base::StrCat({"profile.default_content_setting_values.", name});
    if (prefs->GetInteger(default_path) ==
        static_cast<int>(CONTENT_SETTING_ALLOW)) {
      prefs->ClearPref(default_path);
    }
  }
}

base::DictValue GetDefaultSelectedNetworks() {
  base::DictValue selected_networks;
  selected_networks.Set(kEthereumPrefKey, mojom::kMainnetChainId);
  selected_networks.Set(kSolanaPrefKey, mojom::kSolanaMainnet);
  selected_networks.Set(kFilecoinPrefKey, mojom::kFilecoinMainnet);
  selected_networks.Set(kBitcoinPrefKey, mojom::kBitcoinMainnet);
  selected_networks.Set(kZCashPrefKey, mojom::kZCashMainnet);

  return selected_networks;
}

base::DictValue GetDefaultSelectedNetworksPerOrigin() {
  base::DictValue selected_networks;
  selected_networks.Set(kEthereumPrefKey, base::DictValue());
  selected_networks.Set(kSolanaPrefKey, base::DictValue());
  selected_networks.Set(kFilecoinPrefKey, base::DictValue());
  selected_networks.Set(kBitcoinPrefKey, base::DictValue());
  selected_networks.Set(kZCashPrefKey, base::DictValue());

  return selected_networks;
}

base::DictValue GetDefaultHiddenNetworks() {
  base::DictValue hidden_networks;

  base::ListValue eth_hidden;
  eth_hidden.Append(mojom::kSepoliaChainId);
  eth_hidden.Append(mojom::kFilecoinEthereumTestnetChainId);
  hidden_networks.Set(kEthereumPrefKey, std::move(eth_hidden));

  base::ListValue fil_hidden;
  fil_hidden.Append(mojom::kFilecoinTestnet);
  hidden_networks.Set(kFilecoinPrefKey, std::move(fil_hidden));

  base::ListValue sol_hidden;
  sol_hidden.Append(mojom::kSolanaDevnet);
  sol_hidden.Append(mojom::kSolanaTestnet);
  hidden_networks.Set(kSolanaPrefKey, std::move(sol_hidden));

  base::ListValue btc_hidden;
  btc_hidden.Append(mojom::kBitcoinTestnet);
  hidden_networks.Set(kBitcoinPrefKey, std::move(btc_hidden));

  base::ListValue zec_hidden;
  zec_hidden.Append(mojom::kZCashTestnet);
  hidden_networks.Set(kZCashPrefKey, std::move(zec_hidden));

  base::ListValue cardano_hidden;
  cardano_hidden.Append(mojom::kCardanoTestnet);
  hidden_networks.Set(kCardanoPrefKey, std::move(cardano_hidden));

  base::ListValue polkadot_hidden;
  polkadot_hidden.Append(mojom::kPolkadotTestnet);
  polkadot_hidden.Append(mojom::kPolkadotTestnetAssetHub);
  polkadot_hidden.Append(mojom::kPolkadotPaseoAssetHub);
  hidden_networks.Set(kPolkadotPrefKey, std::move(polkadot_hidden));

  return hidden_networks;
}

}  // namespace

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kBraveWalletLastUnlockTime, base::Time());
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kBraveWalletDisabledByPolicy, false);
  registry->RegisterIntegerPref(
      kDefaultEthereumWallet,
      static_cast<int>(
          brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension));
  registry->RegisterIntegerPref(
      kDefaultSolanaWallet,
      static_cast<int>(
          brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension));
  registry->RegisterIntegerPref(
      kDefaultCardanoWallet,
      static_cast<int>(brave_wallet::mojom::DefaultWallet::BraveWallet));
  registry->RegisterStringPref(kDefaultBaseCurrency, "USD");
  registry->RegisterStringPref(kDefaultBaseCryptocurrency, "BTC");
  registry->RegisterBooleanPref(kShowWalletIconOnToolbar, true);
  registry->RegisterDictionaryPref(kBraveWalletKeyrings);
  registry->RegisterBooleanPref(kBraveWalletKeyringEncryptionKeysMigrated,
                                false);
  registry->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  registry->RegisterDictionaryPref(kBraveWalletEip1559CustomChains);
  registry->RegisterDictionaryPref(kBraveWalletHiddenNetworks,
                                   GetDefaultHiddenNetworks());
  registry->RegisterListPref(kBraveWalletHiddenAccounts);
  registry->RegisterDictionaryPref(kBraveWalletSelectedNetworks,
                                   GetDefaultSelectedNetworks());
  registry->RegisterDictionaryPref(kBraveWalletSelectedNetworksPerOrigin,
                                   GetDefaultSelectedNetworksPerOrigin());
  registry->RegisterListPref(kBraveWalletUserAssetsList,
                             GetDefaultUserAssets());
  registry->RegisterIntegerPref(kBraveWalletAutoLockMinutes,
                                kDefaultWalletAutoLockMinutes);
  registry->RegisterDictionaryPref(kBraveWalletEthAllowancesCache);
  registry->RegisterDictionaryPref(kBraveWalletPolkadotChainMetadata);
  registry->RegisterTimePref(kBraveWalletLastDiscoveredAssetsAt, base::Time());

  registry->RegisterBooleanPref(kShouldShowWalletSuggestionBadge, true);
  registry->RegisterBooleanPref(kBraveWalletNftDiscoveryEnabled, false);
  registry->RegisterBooleanPref(kBraveWalletPrivateWindowsEnabled, false);

  registry->RegisterStringPref(kBraveWalletSelectedWalletAccount, "");
  registry->RegisterStringPref(kBraveWalletSelectedEthDappAccount, "");
  registry->RegisterStringPref(kBraveWalletSelectedSolDappAccount, "");
  registry->RegisterStringPref(kBraveWalletSelectedAdaDappAccount, "");

  registry->RegisterIntegerPref(
      kBraveWalletTransactionSimulationOptInStatus,
      static_cast<int>(brave_wallet::mojom::BlowfishOptInStatus::kUnset));
  registry->RegisterStringPref(kBraveWalletEncryptorSalt, "");
  registry->RegisterDictionaryPref(kBraveWalletMnemonic);
  registry->RegisterBooleanPref(kBraveWalletLegacyEthSeedFormat, false);
  registry->RegisterBooleanPref(kBraveWalletMnemonicBackedUp, false);

  // Register Deprecated CryptoWallet prefs
  // We can eventually remove these. Code removed 05/2025
  registry->RegisterIntegerPref(kERCPrefVersionDeprecated, 0);
  registry->RegisterStringPref(kERCAES256GCMSivNonceDeprecated, "");
  registry->RegisterStringPref(kERCEncryptedSeedDeprecated, "");
  registry->RegisterBooleanPref(kERCOptedIntoCryptoWalletsDeprecated, false);
}

void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
  // Deprecated 05/2026
  registry->RegisterBooleanPref(kBraveWalletP3ANewUserBalanceReportedDeprecated,
                                false);
  // Deprecated 05/2026
  registry->RegisterBooleanPref(kBraveWalletP3ANFTGalleryUsedDeprecated, false);
  // Deprecated 05/2026
  registry->RegisterIntegerPref(kBraveWalletP3AOnboardingLastStepDeprecated, 0);
  // Deprecated 05/2026
  registry->RegisterTimePref(kBraveWalletP3AFirstUnlockTimeDeprecated,
                             base::Time());
  // Deprecated 05/2026
  registry->RegisterTimePref(kBraveWalletP3ALastUnlockTimeDeprecated,
                             base::Time());
  // Deprecated 05/2026
  registry->RegisterBooleanPref(kBraveWalletP3AUsedSecondDayDeprecated, false);
}

void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // Deprecated 05/2026
  local_state->ClearPref(kBraveWalletP3ANewUserBalanceReportedDeprecated);
  // Deprecated 05/2026
  local_state->ClearPref(kBraveWalletP3ANFTGalleryUsedDeprecated);
  // Deprecated 05/2026
  local_state->ClearPref(kBraveWalletP3AOnboardingLastStepDeprecated);
  // Deprecated 05/2026
  local_state->ClearPref(kBraveWalletP3AFirstUnlockTimeDeprecated);
  // Deprecated 05/2026
  local_state->ClearPref(kBraveWalletP3ALastUnlockTimeDeprecated);
  // Deprecated 05/2026
  local_state->ClearPref(kBraveWalletP3AUsedSecondDayDeprecated);
}

void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  // Added 05/2026
  registry->RegisterBooleanPref(kBraveWalletEip1559ForCustomNetworksMigrated,
                                false);
  // Added 05/2026
  registry->RegisterBooleanPref(kBraveWalletIsCompressedNftMigrated, false);
  // Added 05/2026
  registry->RegisterBooleanPref(kBraveWalletGoerliNetworkMigrated, false);
  // Added 05/2026
  registry->RegisterBooleanPref(kBraveWalletIsSPLTokenProgramMigrated, false);
  // Added 05/2026
  registry->RegisterBooleanPref(kBraveWalletAuroraMainnetMigrated, false);

  // Added 05/2026
  registry->RegisterDictionaryPref(kBraveWalletP3AActiveWalletDictDeprecated);

  // Added 05/2026
  registry->RegisterDictionaryPref(
      kBraveWalletLastTransactionSentTimeDictDeprecated);

  // Added 08/2026
  registry->RegisterBooleanPref(kBraveWalletLocalhostNetworksMigrated, false);
}

void ClearJsonRpcServiceProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletCustomNetworks);
  prefs->ClearPref(kBraveWalletHiddenNetworks);
  prefs->ClearPref(kBraveWalletSelectedNetworks);
  prefs->ClearPref(kBraveWalletSelectedNetworksPerOrigin);
  prefs->ClearPref(kBraveWalletEip1559CustomChains);
}

void ClearKeyringServiceProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletKeyrings);
  prefs->ClearPref(kBraveWalletEncryptorSalt);
  prefs->ClearPref(kBraveWalletMnemonic);
  prefs->ClearPref(kBraveWalletLegacyEthSeedFormat);
  prefs->ClearPref(kBraveWalletMnemonicBackedUp);
  prefs->ClearPref(kBraveWalletAutoLockMinutes);
  prefs->ClearPref(kBraveWalletSelectedWalletAccount);
  prefs->ClearPref(kBraveWalletSelectedEthDappAccount);
  prefs->ClearPref(kBraveWalletSelectedSolDappAccount);
  prefs->ClearPref(kBraveWalletSelectedAdaDappAccount);
  prefs->ClearPref(kBraveWalletHiddenAccounts);
}

void ClearBraveWalletServicePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletUserAssetsList);
  prefs->ClearPref(kDefaultBaseCurrency);
  prefs->ClearPref(kDefaultBaseCryptocurrency);
  prefs->ClearPref(kBraveWalletEthAllowancesCache);
}

void MigrateCryptoWalletsPrefToBraveWallet(PrefService* prefs) {
  int value = prefs->GetInteger(kDefaultEthereumWallet);
  if (value ==
      static_cast<int>(mojom::DefaultWallet::CryptoWalletsDeprecated)) {
    prefs->SetInteger(
        kDefaultEthereumWallet,
        static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension));
  }
}

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  // Added 07/2023
  MigrateDerivedAccountIndex(prefs);

  // CryptoWallets Removed 05/2025
  MigrateCryptoWalletsPrefToBraveWallet(prefs);

  // Added 05/2026
  prefs->ClearPref(kBraveWalletP3AActiveWalletDictDeprecated);

  // Added 05/2026
  prefs->ClearPref(kBraveWalletLastTransactionSentTimeDictDeprecated);

  // Added 05/2026
  prefs->ClearPref(kBraveWalletEip1559ForCustomNetworksMigrated);
  // Added 05/2026
  prefs->ClearPref(kBraveWalletIsCompressedNftMigrated);
  // Added 05/2026
  prefs->ClearPref(kBraveWalletGoerliNetworkMigrated);
  // Added 05/2026
  prefs->ClearPref(kBraveWalletIsSPLTokenProgramMigrated);
  // Added 05/2026
  prefs->ClearPref(kBraveWalletAuroraMainnetMigrated);

  // Added 08/2026
  BraveWalletService::MaybeMigrateLocalhostNetworks(prefs);

  // Added 10/2026
  ResetObsoleteWalletPermissions(prefs);
}

}  // namespace brave_wallet
