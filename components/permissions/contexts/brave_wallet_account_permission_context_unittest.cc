/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/contexts/brave_wallet_account_permission_context.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

using brave_wallet::mojom::AccountIdPtr;
using brave_wallet::mojom::CoinType;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::UnorderedElementsAre;

namespace permissions {

namespace {

// The keys of the objects stored for `origin`. Grants are compared by
// unique_key, the identity AccountId equality uses.
std::vector<std::string> GrantedKeys(
    BraveWalletAccountPermissionContext& context,
    const url::Origin& origin) {
  std::vector<std::string> keys;
  for (const auto& object : context.GetGrantedObjects(origin)) {
    keys.push_back(context.GetKeyForObject(object->value));
  }
  return keys;
}

}  // namespace

class BraveWalletAccountPermissionContextTest : public testing::Test {
 protected:
  BraveWalletAccountPermissionContextTest() {
    // These keyrings are off by default on some platforms.
    feature_list_.InitWithFeatures(
        {brave_wallet::features::kBraveWalletCardanoFeature,
         brave_wallet::features::kBraveWalletPolkadotFeature},
        {});
  }

  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterProfilePrefsForMigration(prefs_.registry());
    settings_map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &prefs_, /*is_off_the_record=*/false, /*store_last_modified=*/false,
        /*restore_session=*/false, /*should_record_metrics=*/false);
    wallet_service_ = std::make_unique<brave_wallet::BraveWalletService>(
        url_loader_factory_.GetSafeWeakWrapper(),
        brave_wallet::TestBraveWalletServiceDelegate::Create(), &prefs_,
        &local_state_);
    account_utils().CreateWallet(brave_wallet::kMnemonicDivideCruise,
                                 brave_wallet::kTestWalletPassword);
    context_ = std::make_unique<BraveWalletAccountPermissionContext>(
        settings_map_.get(), wallet_service_.get());
  }

  void TearDown() override {
    context_.reset();
    wallet_service_.reset();
    settings_map_->ShutdownOnUIThread();
  }

  brave_wallet::AccountUtils account_utils() {
    return brave_wallet::AccountUtils(wallet_service_->keyring_service());
  }

  BraveWalletAccountPermissionContext& context() { return *context_; }

  // Replaces the store with one that has no wallet service.
  void ResetContextWithoutWalletService() {
    context_.reset();
    context_ = std::make_unique<BraveWalletAccountPermissionContext>(
        settings_map_.get(), /*wallet_service=*/nullptr);
  }

  static url::Origin SiteA() {
    return url::Origin::Create(GURL("https://a.test/"));
  }
  static url::Origin SiteB() {
    return url::Origin::Create(GURL("https://b.test/"));
  }

 private:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<HostContentSettingsMap> settings_map_;
  std::unique_ptr<brave_wallet::BraveWalletService> wallet_service_;
  std::unique_ptr<BraveWalletAccountPermissionContext> context_;
};

TEST_F(BraveWalletAccountPermissionContextTest, GrantHasAndRevokePerCoin) {
  std::vector<AccountIdPtr> accounts;
  accounts.push_back(account_utils().EnsureEthAccount(0)->account_id.Clone());
  accounts.push_back(account_utils().EnsureSolAccount(0)->account_id.Clone());
  accounts.push_back(account_utils().EnsureAdaAccount(0)->account_id.Clone());
  accounts.push_back(account_utils().EnsureDotAccount(0)->account_id.Clone());
  for (const auto& account : accounts) {
    SCOPED_TRACE(account->unique_key);
    EXPECT_FALSE(context().HasAccount(SiteA(), account));

    context().GrantAccount(SiteA(), account);
    EXPECT_TRUE(context().HasAccount(SiteA(), account));
    EXPECT_THAT(GrantedKeys(context(), SiteA()),
                ElementsAre(account->unique_key));
    // Grants are per site.
    EXPECT_FALSE(context().HasAccount(SiteB(), account));

    context().RevokeAccount(SiteA(), account);
    EXPECT_FALSE(context().HasAccount(SiteA(), account));
    EXPECT_THAT(GrantedKeys(context(), SiteA()), IsEmpty());
  }
}

TEST_F(BraveWalletAccountPermissionContextTest, OnlyTheUniqueKeyIsStored) {
  AccountIdPtr account =
      account_utils().EnsureEthAccount(0)->account_id.Clone();
  context().GrantAccount(SiteA(), account);

  auto granted = context().GetGrantedObjects(SiteA());
  ASSERT_EQ(granted.size(), 1u);
  EXPECT_EQ(granted[0]->value,
            base::DictValue().Set("unique_key", account->unique_key));
  // The grant is keyed on the unique_key, which is what makes two AccountIds
  // equal.
  EXPECT_EQ(context().GetKeyForObject(granted[0]->value), account->unique_key);
}

TEST_F(BraveWalletAccountPermissionContextTest,
       GrantsOfDifferentAccountsAreIndependent) {
  // Each account is its own grant, whether it shares a coin with another.
  AccountIdPtr eth0 = account_utils().EnsureEthAccount(0)->account_id.Clone();
  AccountIdPtr eth1 = account_utils().EnsureEthAccount(1)->account_id.Clone();
  AccountIdPtr sol0 = account_utils().EnsureSolAccount(0)->account_id.Clone();
  context().GrantAccount(SiteA(), eth0);
  context().GrantAccount(SiteA(), eth1);
  context().GrantAccount(SiteA(), sol0);
  EXPECT_THAT(GrantedKeys(context(), SiteA()),
              UnorderedElementsAre(eth0->unique_key, eth1->unique_key,
                                   sol0->unique_key));

  context().RevokeAccount(SiteA(), eth0);
  EXPECT_FALSE(context().HasAccount(SiteA(), eth0));
  EXPECT_TRUE(context().HasAccount(SiteA(), eth1));
  EXPECT_TRUE(context().HasAccount(SiteA(), sol0));
}

TEST_F(BraveWalletAccountPermissionContextTest, ObjectKeyAndValidity) {
  brave_wallet::mojom::AccountInfoPtr eth = account_utils().EnsureEthAccount(0);
  base::DictValue object =
      base::DictValue().Set("unique_key", eth->account_id->unique_key);
  EXPECT_TRUE(context().IsValidObject(object));
  EXPECT_EQ(context().GetKeyForObject(object), eth->account_id->unique_key);

  // An object needs a non-empty string unique_key to be a grant.
  EXPECT_FALSE(context().IsValidObject(base::DictValue()));
  EXPECT_FALSE(context().IsValidObject(
      base::DictValue().Set("unique_key", std::string())));
  EXPECT_FALSE(context().IsValidObject(base::DictValue().Set("unique_key", 1)));
  EXPECT_FALSE(
      context().IsValidObject(base::DictValue().Set("address", eth->address)));
}

TEST_F(BraveWalletAccountPermissionContextTest,
       GrantedAccountsAreRebuiltFromTheKeyring) {
  brave_wallet::mojom::AccountInfoPtr eth = account_utils().EnsureEthAccount(0);
  brave_wallet::mojom::AccountInfoPtr sol = account_utils().EnsureSolAccount(0);
  context().GrantAccount(SiteA(), eth->account_id);
  context().GrantAccount(SiteA(), sol->account_id);

  // Only accounts of the asked coin come back, with all their fields.
  std::vector<AccountIdPtr> granted =
      context().GetGrantedAccounts(SiteA(), CoinType::ETH);
  ASSERT_EQ(granted.size(), 1u);
  EXPECT_EQ(granted[0], eth->account_id);

  auto all_granted = context().GetAllGrantedAccounts(CoinType::SOL);
  ASSERT_EQ(all_granted.size(), 1u);
  EXPECT_EQ(all_granted[0].first, SiteA());
  EXPECT_EQ(all_granted[0].second, sol->account_id);
}

TEST_F(BraveWalletAccountPermissionContextTest, DisplayNameIsTheAccountName) {
  brave_wallet::mojom::AccountInfoPtr eth = account_utils().EnsureEthAccount(0);
  context().GrantAccount(SiteA(), eth->account_id);

  auto granted = context().GetGrantedObjects(SiteA());
  ASSERT_EQ(granted.size(), 1u);
  EXPECT_EQ(context().GetObjectDisplayName(granted[0]->value),
            base::UTF8ToUTF16(eth->name));
}

// A grant for an account the wallet does not have stays stored but is not
// returned as an account, and its entry is shown by its key.
TEST_F(BraveWalletAccountPermissionContextTest,
       GrantsForUnknownAccountsAreNotReturned) {
  brave_wallet::mojom::AccountInfoPtr known =
      account_utils().EnsureEthAccount(0);
  AccountIdPtr unknown = account_utils().EthUnkownAccountId();
  context().GrantAccount(SiteA(), known->account_id);
  context().GrantAccount(SiteA(), unknown);

  EXPECT_TRUE(context().HasAccount(SiteA(), unknown));
  std::vector<AccountIdPtr> granted =
      context().GetGrantedAccounts(SiteA(), CoinType::ETH);
  ASSERT_EQ(granted.size(), 1u);
  EXPECT_EQ(granted[0], known->account_id);

  for (const auto& object : context().GetGrantedObjects(SiteA())) {
    if (context().GetKeyForObject(object->value) == unknown->unique_key) {
      EXPECT_EQ(context().GetObjectDisplayName(object->value),
                base::UTF8ToUTF16(unknown->unique_key));
    }
  }
}

TEST_F(BraveWalletAccountPermissionContextTest,
       WithoutWalletServiceGrantsAreNotReturnedAsAccounts) {
  AccountIdPtr eth = account_utils().EnsureEthAccount(0)->account_id.Clone();
  AccountIdPtr sol = account_utils().EnsureSolAccount(0)->account_id.Clone();
  ResetContextWithoutWalletService();
  context().GrantAccount(SiteA(), eth);
  context().GrantAccount(SiteB(), sol);

  // The grants are stored and found by key...
  EXPECT_TRUE(context().HasAccount(SiteA(), eth));
  EXPECT_TRUE(context().HasAccount(SiteB(), sol));
  // ...but with no keyring to ask, there is nothing to rebuild an AccountId
  // from.
  EXPECT_THAT(context().GetGrantedAccounts(SiteA(), CoinType::ETH), IsEmpty());
  EXPECT_THAT(context().GetAllGrantedAccounts(CoinType::ETH), IsEmpty());
  EXPECT_THAT(context().GetAllGrantedAccounts(CoinType::SOL), IsEmpty());
}

}  // namespace permissions
