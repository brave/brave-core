/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"

#include <memory>
#include <utility>

#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAreArray;
using testing::SizeIs;

namespace brave_wallet {

class BraveWalletPrefsUnitTest : public testing::Test {
 public:
  BraveWalletPrefsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~BraveWalletPrefsUnitTest() override = default;

 protected:
  void SetUp() override {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    // RegisterProfilePrefsForMigration(prefs->registry());
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(BraveWalletPrefsUnitTest, MigrateAddChainIdToTransactionInfo) {
  // Note: The testing profile has already performed the prefs migration by the
  // time this test runs, so undo its effects here for testing purposes
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletTransactionsChainIdMigrated));
  EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletTransactionsChainIdMigrated));
  GetPrefs()->SetBoolean(kBraveWalletTransactionsChainIdMigrated, false);

  base::Value::Dict txs;
  const char ethTxId[] = "b1e8dda1";
  const auto ethPath = base::JoinString({"mainnet", ethTxId}, ".");
  base::Value::Dict tx1;
  tx1.SetByDottedPath(base::JoinString({ethPath, "id"}, "."), ethTxId);

  const char solTxId[] = "887e878f";
  const auto solPath = base::JoinString({"devnet", solTxId}, ".");
  base::Value::Dict tx2;
  tx2.SetByDottedPath(base::JoinString({solPath, "id"}, "."), solTxId);

  const char filTxId[] = "197ea1e5";
  const auto filPath = base::JoinString({"testnet", filTxId}, ".");
  base::Value::Dict tx3;
  tx3.SetByDottedPath(base::JoinString({filPath, "id"}, "."), filTxId);

  {
    ScopedDictPrefUpdate update(GetPrefs(), kBraveWalletTransactions);
    update->SetByDottedPath("ethereum", std::move(tx1));
    update->SetByDottedPath("solana", std::move(tx2));
    update->SetByDottedPath("filecoin", std::move(tx3));
  }
  brave_wallet::MigrateObsoleteProfilePrefs(GetPrefs());

  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletTransactionsChainIdMigrated));
  EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletTransactionsChainIdMigrated));

  const base::Value::Dict* transactions =
      &GetPrefs()->GetDict(kBraveWalletTransactions);

  const std::string* ethChainId = transactions->FindStringByDottedPath(
      base::JoinString({"ethereum", ethPath, "chain_id"}, "."));

  EXPECT_TRUE(ethChainId != nullptr);
  EXPECT_EQ(*ethChainId, "0x1");

  const std::string* solChainId = transactions->FindStringByDottedPath(
      base::JoinString({"solana", solPath, "chain_id"}, "."));

  EXPECT_FALSE(solChainId == nullptr);
  EXPECT_EQ(*solChainId, "0x67");

  const std::string* filChainId = transactions->FindStringByDottedPath(
      base::JoinString({"filecoin", filPath, "chain_id"}, "."));

  EXPECT_FALSE(filChainId == nullptr);
  EXPECT_EQ(*filChainId, "t");
}

}  // namespace brave_wallet
