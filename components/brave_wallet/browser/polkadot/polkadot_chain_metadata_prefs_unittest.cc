/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata_prefs.h"

#include "base/values.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

constexpr char kSpecVersion[] = "spec_version";
constexpr char kBalancesPalletIndex[] = "balances_pallet_index";
constexpr char kTransferAllowDeathCallIndex[] =
    "transfer_allow_death_call_index";
constexpr char kSs58Prefix[] = "ss58_prefix";

class PolkadotChainMetadataPrefsUnitTest : public testing::Test {
 protected:
  void SetUp() override { RegisterProfilePrefs(profile_prefs_.registry()); }

  PolkadotChainMetadataPrefs MakePrefs() {
    return PolkadotChainMetadataPrefs(profile_prefs_);
  }

  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
};

TEST_F(PolkadotChainMetadataPrefsUnitTest, SetAndGetChainMetadataRoundTrip) {
  auto metadata = PolkadotChainMetadata::FromFields(
      /*balances_pallet_index=*/7, /*transfer_allow_death_call_index=*/2,
      /*ss58_prefix=*/42, /*spec_version=*/1234);
  ASSERT_TRUE(metadata);

  PolkadotChainMetadataPrefs prefs = MakePrefs();
  EXPECT_TRUE(prefs.SetChainMetadata(mojom::kPolkadotMainnet, *metadata));

  auto loaded = prefs.GetChainMetadata(mojom::kPolkadotMainnet);
  ASSERT_TRUE(loaded);
  EXPECT_EQ(loaded->GetBalancesPalletIndex(), 7u);
  EXPECT_EQ(loaded->GetTransferAllowDeathCallIndex(), 2u);
  EXPECT_EQ(loaded->GetSs58Prefix(), 42u);
  EXPECT_EQ(loaded->GetSpecVersion(), 1234u);
}

TEST_F(PolkadotChainMetadataPrefsUnitTest, GetChainMetadataMissingEntry) {
  PolkadotChainMetadataPrefs prefs = MakePrefs();
  EXPECT_FALSE(prefs.GetChainMetadata(mojom::kPolkadotMainnet));
}

TEST_F(PolkadotChainMetadataPrefsUnitTest, InvalidRangeRejected) {
  ScopedDictPrefUpdate update(&profile_prefs_, kBraveWalletPolkadotChainMetadata);
  base::DictValue value;
  value.Set(kBalancesPalletIndex, 999);  // Out of uint8 range.
  value.Set(kTransferAllowDeathCallIndex, 1);
  value.Set(kSs58Prefix, 0);
  
  update->Set(mojom::kPolkadotMainnet, std::move(value));

  PolkadotChainMetadataPrefs prefs = MakePrefs();
  EXPECT_FALSE(prefs.GetChainMetadata(mojom::kPolkadotMainnet));
}

TEST_F(PolkadotChainMetadataPrefsUnitTest, NegativeValueRejected) {
  ScopedDictPrefUpdate update(&profile_prefs_, kBraveWalletPolkadotChainMetadata);
  base::DictValue value;
  value.Set(kBalancesPalletIndex, 1);
  value.Set(kTransferAllowDeathCallIndex, -1);  // Negative should be rejected.
  value.Set(kSs58Prefix, 0);
  update->Set(mojom::kPolkadotMainnet, std::move(value));

  PolkadotChainMetadataPrefs prefs = MakePrefs();
  EXPECT_FALSE(prefs.GetChainMetadata(mojom::kPolkadotMainnet));
}

TEST_F(PolkadotChainMetadataPrefsUnitTest, MissingSpecVersionDefaultsToZero) {
  ScopedDictPrefUpdate update(&profile_prefs_, kBraveWalletPolkadotChainMetadata);
  base::DictValue value;
  value.Set(kSpecVersion, 100);
  value.Set(kBalancesPalletIndex, 3);
  value.Set(kTransferAllowDeathCallIndex, 1);
  value.Set(kSs58Prefix, 42);
  update->Set(mojom::kPolkadotMainnet, std::move(value));

  PolkadotChainMetadataPrefs prefs = MakePrefs();
  auto loaded = prefs.GetChainMetadata(mojom::kPolkadotMainnet);
  ASSERT_TRUE(loaded);
  EXPECT_EQ(loaded->GetSpecVersion(), 100u);
  EXPECT_EQ(loaded->GetBalancesPalletIndex(), 3u);
  EXPECT_EQ(loaded->GetTransferAllowDeathCallIndex(), 1u);
  EXPECT_EQ(loaded->GetSs58Prefix(), 42u);
}

}  // namespace
}  // namespace brave_wallet
