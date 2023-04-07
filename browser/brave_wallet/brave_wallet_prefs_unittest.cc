/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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

TEST_F(BraveWalletPrefsUnitTest, MigrateObsoleteProfilePrefsWeb3Provider) {
  // AskDeprecated changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kBraveWalletWeb3ProviderDeprecated,
                         static_cast<int>(mojom::DefaultWallet::AskDeprecated));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));

  // BraveWallet changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kBraveWalletWeb3ProviderDeprecated,
                         static_cast<int>(mojom::DefaultWallet::BraveWallet));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));

  // BraveWalletPreferExtension remains BraveWalletPreferExtension
  GetPrefs()->SetInteger(
      kBraveWalletWeb3ProviderDeprecated,
      static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));

  // Ask changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kBraveWalletWeb3ProviderDeprecated,
                         static_cast<int>(mojom::DefaultWallet::AskDeprecated));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));

  // CryptoWallets changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kBraveWalletWeb3ProviderDeprecated,
                         static_cast<int>(mojom::DefaultWallet::CryptoWallets));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));

  // None remains None
  GetPrefs()->SetInteger(kBraveWalletWeb3ProviderDeprecated,
                         static_cast<int>(mojom::DefaultWallet::None));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::None,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));
}

TEST_F(BraveWalletPrefsUnitTest,
       MigrateObsoleteProfilePrefsDefaultWalletDeprecated) {
  // AskDeprecated changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kDefaultWalletDeprecated,
                         static_cast<int>(mojom::DefaultWallet::AskDeprecated));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));

  // BraveWallet changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kDefaultWalletDeprecated,
                         static_cast<int>(mojom::DefaultWallet::BraveWallet));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));

  // BraveWalletPreferExtension remains BraveWalletPreferExtension
  GetPrefs()->SetInteger(
      kDefaultWalletDeprecated,
      static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));

  // Ask changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kDefaultWalletDeprecated,
                         static_cast<int>(mojom::DefaultWallet::AskDeprecated));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));

  // CryptoWallets changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kDefaultWalletDeprecated,
                         static_cast<int>(mojom::DefaultWallet::CryptoWallets));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));

  // None remains None
  GetPrefs()->SetInteger(kDefaultWalletDeprecated,
                         static_cast<int>(mojom::DefaultWallet::None));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::None,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultEthereumWallet)));
}

TEST_F(BraveWalletPrefsUnitTest,
       MigrateObsoleteProfilePrefsBraveWalletEthereumTransactionsCoinType) {
  GetPrefs()->SetBoolean(kBraveWalletTransactionsChainIdMigrated, true);
  // Migration when kBraveWalletTransactions is default value (empty dict).
  ASSERT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated));
  auto* pref = GetPrefs()->FindPreference(kBraveWalletTransactions);
  ASSERT_TRUE(pref && pref->IsDefaultValue());
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_TRUE(pref && pref->IsDefaultValue());
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated));

  // Migration with existing transactions.
  GetPrefs()->ClearPref(kBraveWalletEthereumTransactionsCoinTypeMigrated);
  GetPrefs()->ClearPref(kBraveWalletTransactions);
  GetPrefs()->SetBoolean(kBraveWalletTransactionsChainIdMigrated, true);
  ASSERT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated));
  base::Value::Dict tx1;
  tx1.Set("id", "0x1");
  tx1.Set("status", 1);

  base::Value::Dict tx2;
  tx2.Set("id", "0x2");
  tx2.Set("status", 2);

  {
    ScopedDictPrefUpdate update(GetPrefs(), kBraveWalletTransactions);
    update->SetByDottedPath("mainnet.meta1", tx1.Clone());
    update->SetByDottedPath("mainnet.meta2", tx2.Clone());
    update->SetByDottedPath("ropsten.meta3", tx1.Clone());
  }

  MigrateObsoleteProfilePrefs(GetPrefs());
  const auto& dict = GetPrefs()->GetDict(kBraveWalletTransactions);
  const auto* tx1_value = dict.FindDictByDottedPath("ethereum.mainnet.meta1");
  const auto* tx2_value = dict.FindDictByDottedPath("ethereum.mainnet.meta2");
  const auto* tx3_value = dict.FindDictByDottedPath("ethereum.ropsten.meta3");
  ASSERT_TRUE(tx1_value && tx2_value && tx3_value);
  EXPECT_EQ(*tx1_value, tx1);
  EXPECT_EQ(*tx2_value, tx2);
  EXPECT_EQ(*tx3_value, tx1);
  EXPECT_EQ(dict.size(), 1u);
  EXPECT_EQ(dict.FindDict("ethereum")->size(), 2u);
  EXPECT_EQ(dict.FindDictByDottedPath("ethereum.mainnet")->size(), 2u);
  EXPECT_EQ(dict.FindDictByDottedPath("ethereum.ropsten")->size(), 1u);
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated));

  // Migration when kBraveWalletTransactions is an empty dict.
  GetPrefs()->ClearPref(kBraveWalletEthereumTransactionsCoinTypeMigrated);
  GetPrefs()->ClearPref(kBraveWalletTransactions);
  {
    ScopedDictPrefUpdate update(GetPrefs(), kBraveWalletTransactions);
    update->SetByDottedPath("mainnet.meta1", tx1.Clone());
    update->Remove("mainnet");
  }
  EXPECT_TRUE(pref && !pref->IsDefaultValue());
  EXPECT_TRUE(GetPrefs()->GetDict(kBraveWalletTransactions).empty());

  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_TRUE(pref && pref->IsDefaultValue());
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated));
}

TEST_F(BraveWalletPrefsUnitTest, MigrateShowTestNetworksToggle) {
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kShowWalletTestNetworksDeprecated));

  // Hiding non-test network which should not be touched by migration.
  AddHiddenNetwork(GetPrefs(), mojom::CoinType::FIL, "0x123");

  // Test networks are hidden by default.
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::ETH),
              ElementsAreArray({mojom::kGoerliChainId, mojom::kSepoliaChainId,
                                mojom::kLocalhostChainId,
                                mojom::kFilecoinEthereumTestnetChainId}));
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::FIL),
              ElementsAreArray({mojom::kFilecoinTestnet,
                                mojom::kLocalhostChainId, "0x123"}));
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::SOL),
              ElementsAreArray({mojom::kSolanaDevnet, mojom::kSolanaTestnet,
                                mojom::kLocalhostChainId}));

  MigrateObsoleteProfilePrefs(GetPrefs());
  // Still same when nothing to migrate.
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::ETH),
              ElementsAreArray({mojom::kGoerliChainId, mojom::kSepoliaChainId,
                                mojom::kLocalhostChainId,
                                mojom::kFilecoinEthereumTestnetChainId}));
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::FIL),
              ElementsAreArray({mojom::kFilecoinTestnet,
                                mojom::kLocalhostChainId, "0x123"}));
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::SOL),
              ElementsAreArray({mojom::kSolanaDevnet, mojom::kSolanaTestnet,
                                mojom::kLocalhostChainId}));

  GetPrefs()->SetBoolean(kShowWalletTestNetworksDeprecated, false);

  MigrateObsoleteProfilePrefs(GetPrefs());
  // Still same when test networks toggle was explicitly off.
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::ETH),
              ElementsAreArray({mojom::kGoerliChainId, mojom::kSepoliaChainId,
                                mojom::kLocalhostChainId,
                                mojom::kFilecoinEthereumTestnetChainId}));
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::FIL),
              ElementsAreArray({mojom::kFilecoinTestnet,
                                mojom::kLocalhostChainId, "0x123"}));
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::SOL),
              ElementsAreArray({mojom::kSolanaDevnet, mojom::kSolanaTestnet,
                                mojom::kLocalhostChainId}));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kShowWalletTestNetworksDeprecated));

  GetPrefs()->SetBoolean(kShowWalletTestNetworksDeprecated, true);

  MigrateObsoleteProfilePrefs(GetPrefs());
  // Test networks are removed from hidden list after successfull migration.
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::ETH), SizeIs(0));
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::FIL),
              ElementsAreArray({"0x123"}));
  EXPECT_THAT(GetHiddenNetworks(GetPrefs(), mojom::CoinType::SOL), SizeIs(0));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kShowWalletTestNetworksDeprecated));
}

TEST_F(BraveWalletPrefsUnitTest, MigrateAddChainIdToTransactionInfo) {
  GetPrefs()->SetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated,
                         true);
  EXPECT_FALSE(
      GetPrefs()->HasPrefPath(kBraveWalletTransactionsChainIdMigrated));
  EXPECT_FALSE(GetPrefs()->GetBoolean(kBraveWalletTransactionsChainIdMigrated));

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
  MigrateObsoleteProfilePrefs(GetPrefs());

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
