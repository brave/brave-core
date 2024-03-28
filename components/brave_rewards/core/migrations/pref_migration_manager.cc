/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/migrations/pref_migration_manager.h"

#include <string>
#include <utility>

#include "base/base64.h"
#include "brave/components/brave_rewards/core/common/user_prefs.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

namespace brave_rewards::internal {

namespace {

constexpr int kOldestSupportedVersion = 10;
constexpr int kCurrentVersion = 14;

static_assert(kOldestSupportedVersion <= kCurrentVersion,
              "Oldest pref version cannot be less than the current version");

}  // namespace

PrefMigrationManager::PrefMigrationManager(RewardsEngine& engine)
    : RewardsEngineHelper(engine) {}

PrefMigrationManager::~PrefMigrationManager() = default;

UserPrefs& PrefMigrationManager::user_prefs() {
  return Get<UserPrefs>();
}

// Some guidelines for writing pref migrations:
//
// * Add a comment describing what the migration does and the version and date
//   when it was added. This will help us determine when the migration is no
//   longer required.
// * Migrations cannot fail, as that could leave the user in an unrecoverable
//   state. Do not perform any actions where failure is an expected outcome.
// * Migrations should only act upon locally-store state. If something complex
//   needs to happen, then consider setting a pref flag, performing the action
//   on startup if the pref flag has been set, and then clearing the flag when
//   the action is complete.

template <>
void PrefMigrationManager::MigrateToVersion<11>(base::OnceClosure callback) {
  // Description: In version 7 encryption was added for |kWalletBrave|. However
  // due to wallet corruption, users copying their profiles to new computers or
  // reinstalling their operating system, that change was reverted.
  // Version: 1.31 (Sep 2021)
  auto data = user_prefs().GetString(prefs::kWalletBrave);

  if (data.empty()) {
    std::move(callback).Run();
    return;
  }

  engine().Log(FROM_HERE) << "Decrypting stored Rewards payment ID";

  if (!base::Base64Decode(data, &data)) {
    engine().LogError(FROM_HERE) << "Base64 decoding failed for payment ID";
    std::move(callback).Run();
    return;
  }

  std::optional<std::string> json;
  client().DecryptString(data, &json);
  if (!json || json->empty()) {
    engine().LogError(FROM_HERE) << "Unable to decrypt payment ID";
    std::move(callback).Run();
    return;
  }

  user_prefs().SetString(prefs::kWalletBrave, *json);
  std::move(callback).Run();
}

template <>
void PrefMigrationManager::MigrateToVersion<12>(base::OnceClosure callback) {
  // Description: Fixes stored `mojom::WalletStatus` values that have been
  // removed.
  // Version: 1.47 (Nov 2022)
  std::array providers{constant::kWalletBitflyer, constant::kWalletGemini,
                       constant::kWalletUphold, constant::kWalletZebPay};

  for (auto provider : providers) {
    if (auto wallet = wallet::GetWallet(engine(), provider)) {
      std::optional<mojom::WalletStatus> new_status;
      switch (int status = static_cast<int>(wallet->status)) {
        case 1:  // CONNECTED
        case 3:  // DISCONNECTED_NOT_VERIFIED
        case 5:  // PENDING
          new_status = mojom::WalletStatus::kNotConnected;
          break;
        case 2:  // VERIFIED
          if (wallet->token.empty() || wallet->address.empty()) {
            new_status = mojom::WalletStatus::kLoggedOut;
          }
          break;
      }
      if (new_status.has_value()) {
        wallet->status = *new_status;
        Log(FROM_HERE) << "Updating external wallet status to "
                       << wallet->status;
        wallet::SetWallet(engine(), std::move(wallet));
      }
    }
  }

  std::move(callback).Run();
}

template <>
void PrefMigrationManager::MigrateToVersion<13>(base::OnceClosure callback) {
  // Description: Notifies the RewardsEngineClient if the user is connected to
  // an external wallet provider. This was required as part of the Rewards 2.5
  // updates in order to allow the Ads service to reset state for connected
  // users.
  // Version: 1.48 (Dec 2022)
  std::array providers{constant::kWalletBitflyer, constant::kWalletGemini,
                       constant::kWalletUphold, constant::kWalletZebPay};

  for (auto provider : providers) {
    auto wallet = wallet::GetWallet(engine(), provider);
    if (wallet && wallet->status == mojom::WalletStatus::kConnected) {
      Log(FROM_HERE) << "Notifying client of connected wallet status";
      client().ExternalWalletConnected();
      break;
    }
  }

  std::move(callback).Run();
}

template <>
void PrefMigrationManager::MigrateToVersion<14>(base::OnceClosure callback) {
  // Description: Fixes an issue where kExternalWalletType might be empty for a
  // user that has a connected external wallet.
  // Version: 1.62 (Nov 2023)
  if (user_prefs().GetString(prefs::kExternalWalletType).empty()) {
    std::array providers{constant::kWalletBitflyer, constant::kWalletGemini,
                         constant::kWalletUphold, constant::kWalletZebPay};

    for (auto provider : providers) {
      auto wallet = wallet::GetWallet(engine(), provider);
      if (wallet && wallet->status != mojom::WalletStatus::kNotConnected) {
        Log(FROM_HERE) << "Updating external wallet type preference";
        user_prefs().SetString(prefs::kExternalWalletType, provider);
        break;
      }
    }
  }
  std::move(callback).Run();
}

void PrefMigrationManager::MigratePrefs(base::OnceCallback<void()> callback) {
  int user_version = user_prefs().GetInteger(prefs::kVersion);

  if (user_version <= 0 || user_version >= kCurrentVersion) {
    user_prefs().SetInteger(prefs::kVersion, kCurrentVersion);
    std::move(callback).Run();
    return;
  }

  if (user_version < kOldestSupportedVersion) {
    LogError(FROM_HERE)
        << "Unsupported preferences version detected - resetting user state to "
           "unconnected";

    user_prefs().ClearPref(prefs::kExternalWalletType);
    user_prefs().ClearPref(prefs::kWalletBitflyer);
    user_prefs().ClearPref(prefs::kWalletUphold);
    user_prefs().ClearPref(prefs::kWalletGemini);
    user_prefs().ClearPref(prefs::kWalletZebPay);
    user_prefs().ClearPref(prefs::kWalletSolana);
    user_prefs().SetInteger(prefs::kVersion, kCurrentVersion);

    std::move(callback).Run();
    return;
  }

  if constexpr (kCurrentVersion == kOldestSupportedVersion) {
    std::move(callback).Run();
  } else {
    PerformMigration<kOldestSupportedVersion + 1>(kCurrentVersion,
                                                  std::move(callback));
  }
}

void PrefMigrationManager::MigratePrefsForTesting(int target_version,
                                                  base::OnceClosure callback) {
  if constexpr (kCurrentVersion == kOldestSupportedVersion) {
    std::move(callback).Run();
  } else {
    PerformMigration<kOldestSupportedVersion + 1>(target_version,
                                                  std::move(callback));
  }
}

int PrefMigrationManager::GetCurrentVersionForTesting() {
  return kCurrentVersion;
}

template <int version>
void PrefMigrationManager::PerformMigration(int target_version,
                                            base::OnceClosure callback) {
  static_assert(version > kOldestSupportedVersion,
                "Invalid user pref migration index");

  if constexpr (version > kCurrentVersion) {
    std::move(callback).Run();
  } else {
    if (user_prefs().GetInteger(prefs::kVersion) < version &&
        target_version >= version) {
      Log(FROM_HERE) << "Migrating to prefs version " << version;
      user_prefs().SetInteger(prefs::kVersion, version);
      MigrateToVersion<version>(base::BindOnce(
          &PrefMigrationManager::PerformMigration<version + 1>,
          weak_factory_.GetWeakPtr(), target_version, std::move(callback)));
    } else {
      PrefMigrationManager::PerformMigration<version + 1>(target_version,
                                                          std::move(callback));
    }
  }
}

}  // namespace brave_rewards::internal
