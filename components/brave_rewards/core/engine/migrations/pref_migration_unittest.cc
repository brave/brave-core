/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/engine/migrations/pref_migration_manager.h"
#include "brave/components/brave_rewards/core/engine/test/rewards_engine_test.h"
#include "brave/components/brave_rewards/core/engine/util/rewards_prefs.h"
#include "brave/components/brave_rewards/core/engine/wallet/wallet_util.h"

namespace brave_rewards::internal {

class RewardsPrefMigrationTest : public RewardsEngineTest {
 protected:
  void ExecuteMigration(int version) {
    prefs().SetInteger(prefs::kVersion, version - 1);

    WaitFor([&](auto callback) {
      engine().Get<PrefMigrationManager>().MigratePrefsForTesting(
          version, std::move(callback));
    });

    EXPECT_EQ(prefs().GetInteger(prefs::kVersion), version);
  }

  RewardsPrefs& prefs() { return engine().Get<RewardsPrefs>(); }

  int current_version() {
    return PrefMigrationManager::GetCurrentVersionForTesting();
  }
};

TEST_F(RewardsPrefMigrationTest, NewUser) {
  InitializeEngine();
  EXPECT_EQ(prefs().GetInteger(prefs::kVersion), current_version());
}

TEST_F(RewardsPrefMigrationTest, UnsupportedVersion) {
  prefs().SetInteger(prefs::kVersion, 1);
  InitializeEngine();
  EXPECT_EQ(prefs().GetInteger(prefs::kVersion), current_version());
}

TEST_F(RewardsPrefMigrationTest, CurrentVersion) {
  prefs().SetInteger(prefs::kVersion, current_version());
  InitializeEngine();
  EXPECT_EQ(prefs().GetInteger(prefs::kVersion), current_version());
}

TEST_F(RewardsPrefMigrationTest, FutureVersion) {
  prefs().SetInteger(prefs::kVersion, current_version() + 1);
  InitializeEngine();
  EXPECT_EQ(prefs().GetInteger(prefs::kVersion), current_version());
}

TEST_F(RewardsPrefMigrationTest, Migration11) {
  std::string json = R"({
        "payment_id": "abc",
        "recovery_seed": "123
      })";

  prefs().SetString(prefs::kWalletBrave,
                    FakeEncryption::Base64EncryptString(json));

  ExecuteMigration(11);
  EXPECT_EQ(prefs().GetString(prefs::kWalletBrave), json);
}

TEST_F(RewardsPrefMigrationTest, Migration12_CONNECTED) {
  prefs().SetString(prefs::kWalletUphold,
                    FakeEncryption::Base64EncryptString(R"({ "status": 1 })"));

  ExecuteMigration(12);
  auto wallet = wallet::GetWallet(engine(), "uphold");
  ASSERT_TRUE(wallet);
  EXPECT_EQ(wallet->status, mojom::WalletStatus::kNotConnected);
}

TEST_F(RewardsPrefMigrationTest, Migration12_DISCONNECTED_NOT_VERIFIED) {
  prefs().SetString(prefs::kWalletUphold,
                    FakeEncryption::Base64EncryptString(R"({ "status": 3 })"));

  ExecuteMigration(12);
  auto wallet = wallet::GetWallet(engine(), "uphold");
  ASSERT_TRUE(wallet);
  EXPECT_EQ(wallet->status, mojom::WalletStatus::kNotConnected);
}

TEST_F(RewardsPrefMigrationTest, Migration12_PENDING) {
  prefs().SetString(prefs::kWalletUphold,
                    FakeEncryption::Base64EncryptString(R"({ "status": 5 })"));

  ExecuteMigration(12);
  auto wallet = wallet::GetWallet(engine(), "uphold");
  ASSERT_TRUE(wallet);
  EXPECT_EQ(wallet->status, mojom::WalletStatus::kNotConnected);
}

TEST_F(RewardsPrefMigrationTest, Migration13) {
  prefs().SetString(prefs::kWalletUphold,
                    FakeEncryption::Base64EncryptString(R"({ "status": 2 })"));

  ExecuteMigration(13);

  EXPECT_TRUE(std::ranges::contains(client().GetObserverEventsForTesting(),
                                    "external-wallet-connected"));
}

TEST_F(RewardsPrefMigrationTest, Migration14) {
  prefs().SetString(prefs::kWalletGemini,
                    FakeEncryption::Base64EncryptString(R"({ "status": 0 })"));

  prefs().SetString(prefs::kWalletUphold,
                    FakeEncryption::Base64EncryptString(R"({ "status": 2 })"));

  ExecuteMigration(14);

  EXPECT_EQ(prefs().GetString(prefs::kExternalWalletType), "uphold");
}

TEST_F(RewardsPrefMigrationTest, Migration15) {
  prefs().SetUint64(prefs::kServerPublisherListStamp, 1234);
  ExecuteMigration(15);
  EXPECT_EQ(prefs().GetUint64(prefs::kServerPublisherListStamp), 0ull);
}

}  // namespace brave_rewards::internal
