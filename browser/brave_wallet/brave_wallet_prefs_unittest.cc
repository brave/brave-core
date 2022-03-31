/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"

#include <memory>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

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
                GetPrefs()->GetInteger(kDefaultWallet2)));

  // BraveWallet changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kBraveWalletWeb3ProviderDeprecated,
                         static_cast<int>(mojom::DefaultWallet::BraveWallet));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultWallet2)));

  // BraveWalletPreferExtension remains BraveWalletPreferExtension
  GetPrefs()->SetInteger(
      kBraveWalletWeb3ProviderDeprecated,
      static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultWallet2)));

  // Ask changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kBraveWalletWeb3ProviderDeprecated,
                         static_cast<int>(mojom::DefaultWallet::AskDeprecated));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultWallet2)));

  // CryptoWallets changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kBraveWalletWeb3ProviderDeprecated,
                         static_cast<int>(mojom::DefaultWallet::CryptoWallets));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultWallet2)));

  // None remains None
  GetPrefs()->SetInteger(kBraveWalletWeb3ProviderDeprecated,
                         static_cast<int>(mojom::DefaultWallet::None));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::None,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultWallet2)));
}

TEST_F(BraveWalletPrefsUnitTest,
       MigrateObsoleteProfilePrefsDefaultWalletDeprecated) {
  // AskDeprecated changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kDefaultWalletDeprecated,
                         static_cast<int>(mojom::DefaultWallet::AskDeprecated));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultWallet2)));

  // BraveWallet changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kDefaultWalletDeprecated,
                         static_cast<int>(mojom::DefaultWallet::BraveWallet));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultWallet2)));

  // BraveWalletPreferExtension remains BraveWalletPreferExtension
  GetPrefs()->SetInteger(
      kDefaultWalletDeprecated,
      static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultWallet2)));

  // Ask changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kDefaultWalletDeprecated,
                         static_cast<int>(mojom::DefaultWallet::AskDeprecated));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultWallet2)));

  // CryptoWallets changes to BraveWalletPreferExtension
  GetPrefs()->SetInteger(kDefaultWalletDeprecated,
                         static_cast<int>(mojom::DefaultWallet::CryptoWallets));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::BraveWalletPreferExtension,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultWallet2)));

  // None remains None
  GetPrefs()->SetInteger(kDefaultWalletDeprecated,
                         static_cast<int>(mojom::DefaultWallet::None));
  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_EQ(mojom::DefaultWallet::None,
            static_cast<mojom::DefaultWallet>(
                GetPrefs()->GetInteger(kDefaultWallet2)));
}

TEST_F(BraveWalletPrefsUnitTest,
       MigrateObsoleteProfilePrefsBraveWalletEthereumTransactionsCoinType) {
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
  ASSERT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated));
  base::Value tx1(base::Value::Type::DICTIONARY);
  tx1.SetStringKey("id", "0x1");
  tx1.SetIntKey("status", 1);

  base::Value tx2(base::Value::Type::DICTIONARY);
  tx2.SetStringKey("id", "0x2");
  tx2.SetIntKey("status", 2);

  {
    DictionaryPrefUpdate update(GetPrefs(), kBraveWalletTransactions);
    base::Value* update_dict = update.Get();
    update_dict->SetPath("mainnet.meta1", base::Value(tx1.Clone()));
    update_dict->SetPath("mainnet.meta2", base::Value(tx2.Clone()));
    update_dict->SetPath("ropsten.meta3", base::Value(tx1.Clone()));
  }

  MigrateObsoleteProfilePrefs(GetPrefs());
  const base::Value* dict = GetPrefs()->GetDictionary(kBraveWalletTransactions);
  const base::Value* tx1_value = dict->FindDictPath("ethereum.mainnet.meta1");
  const base::Value* tx2_value = dict->FindDictPath("ethereum.mainnet.meta2");
  const base::Value* tx3_value = dict->FindDictPath("ethereum.ropsten.meta3");
  ASSERT_TRUE(tx1_value && tx2_value && tx3_value);
  EXPECT_EQ(*tx1_value, tx1);
  EXPECT_EQ(*tx2_value, tx2);
  EXPECT_EQ(*tx3_value, tx1);
  EXPECT_EQ(dict->DictSize(), 1u);
  EXPECT_EQ(dict->FindDictKey("ethereum")->DictSize(), 2u);
  EXPECT_EQ(dict->FindDictPath("ethereum.mainnet")->DictSize(), 2u);
  EXPECT_EQ(dict->FindDictPath("ethereum.ropsten")->DictSize(), 1u);
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated));

  // Migration when kBraveWalletTransactions is an empty dict.
  GetPrefs()->ClearPref(kBraveWalletEthereumTransactionsCoinTypeMigrated);
  GetPrefs()->ClearPref(kBraveWalletTransactions);
  {
    DictionaryPrefUpdate update(GetPrefs(), kBraveWalletTransactions);
    base::Value* update_dict = update.Get();
    update_dict->SetPath("mainnet.meta1", base::Value(tx1.Clone()));
    update_dict->RemovePath("mainnet");
  }
  EXPECT_TRUE(pref && !pref->IsDefaultValue());
  EXPECT_TRUE(GetPrefs()->GetDictionary(kBraveWalletTransactions)->DictEmpty());

  MigrateObsoleteProfilePrefs(GetPrefs());
  EXPECT_TRUE(pref && pref->IsDefaultValue());
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated));
}

}  // namespace brave_wallet
