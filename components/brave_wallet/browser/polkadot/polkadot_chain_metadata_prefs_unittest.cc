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
constexpr char kSystemPalletIndex[] = "system_pallet_index";
constexpr char kBalancesPalletIndex[] = "balances_pallet_index";
constexpr char kTransactionPaymentPalletIndex[] =
    "transaction_payment_pallet_index";
constexpr char kTransferAllowDeathCallIndex[] =
    "transfer_allow_death_call_index";
constexpr char kTransferKeepAliveCallIndex[] = "transfer_keep_alive_call_index";
constexpr char kTransferAllCallIndex[] = "transfer_all_call_index";
constexpr char kAssetTxPayment[] = "asset_tx_payment";
constexpr char kSs58Prefix[] = "ss58_prefix";
constexpr char kVersionField[] = "version";

class PolkadotChainMetadataPrefsUnitTest : public testing::Test {
 protected:
  void SetUp() override { RegisterProfilePrefs(profile_prefs_.registry()); }

  PolkadotChainMetadataPrefs MakePrefs() {
    return PolkadotChainMetadataPrefs(profile_prefs_);
  }

  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
};

TEST_F(PolkadotChainMetadataPrefsUnitTest, SetAndGetChainMetadataRoundTrip) {
  struct TestCase {
    std::string test_name;
    uint8_t system_pallet_index;
    uint8_t balances_pallet_index;
    uint8_t transaction_payment_pallet_index;
    uint8_t transfer_allow_death_call_index;
    uint8_t transfer_keep_alive_call_index;
    uint8_t transfer_all_call_index;
    uint16_t ss58_prefix;
    uint32_t spec_version;
    bool asset_tx_payment;
  };

  const TestCase test_cases[] = {
      {"Polkadot Metadata",
       /*system_pallet_index=*/3, /*balances_pallet_index=*/7,
       /*transaction_payment_pallet_index=*/8,
       /*transfer_allow_death_call_index=*/2,
       /*transfer_keep_alive_call_index=*/4,
       /*transfer_all_call_index=*/5,
       /*ss58_prefix=*/42,
       /*spec_version=*/1234, /*asset_tx_payment=*/false},
      {"AssetHub Polkadot Metadata",
       /*system_pallet_index=*/0, /*balances_pallet_index=*/0x0a,
       /*transaction_payment_pallet_index=*/5,
       /*transfer_allow_death_call_index=*/2,
       /*transfer_keep_alive_call_index=*/4,
       /*transfer_all_call_index=*/255,
       /*ss58_prefix=*/std::numeric_limits<uint16_t>::max(),
       /*spec_version=*/12344321, /*asset_tx_payment=*/true}};

  for (const auto& tc : test_cases) {
    SCOPED_TRACE(tc.test_name);

    PolkadotChainMetadata metadata;
    metadata->system_pallet_index = tc.system_pallet_index;
    metadata->balances_pallet_index = tc.balances_pallet_index;
    metadata->transaction_payment_pallet_index =
        tc.transaction_payment_pallet_index;
    metadata->transfer_allow_death_call_index =
        tc.transfer_allow_death_call_index;
    metadata->transfer_keep_alive_call_index =
        tc.transfer_keep_alive_call_index;
    metadata->transfer_all_call_index = tc.transfer_all_call_index;
    metadata->ss58_prefix = tc.ss58_prefix;
    metadata->spec_version = tc.spec_version;
    metadata->asset_tx_payment = tc.asset_tx_payment;

    PolkadotChainMetadataPrefs prefs = MakePrefs();
    EXPECT_TRUE(prefs.SetChainMetadata(mojom::kPolkadotMainnet, metadata));

    auto loaded = prefs.GetChainMetadata(mojom::kPolkadotMainnet);
    ASSERT_TRUE(loaded);

    const auto& loaded_metadata = *loaded;
    EXPECT_EQ(loaded_metadata->system_pallet_index, tc.system_pallet_index);
    EXPECT_EQ(loaded_metadata->balances_pallet_index, tc.balances_pallet_index);
    EXPECT_EQ(loaded_metadata->transaction_payment_pallet_index,
              tc.transaction_payment_pallet_index);
    EXPECT_EQ(loaded_metadata->transfer_allow_death_call_index,
              tc.transfer_allow_death_call_index);
    EXPECT_EQ(loaded_metadata->transfer_keep_alive_call_index,
              tc.transfer_keep_alive_call_index);
    EXPECT_EQ(loaded_metadata->transfer_all_call_index,
              tc.transfer_all_call_index);
    EXPECT_EQ(loaded_metadata->ss58_prefix, tc.ss58_prefix);
    EXPECT_EQ(loaded_metadata->spec_version, tc.spec_version);
    EXPECT_EQ(loaded_metadata->asset_tx_payment, tc.asset_tx_payment);

    const auto& all_metadata =
        profile_prefs_.GetDict(kBraveWalletPolkadotChainMetadata);
    EXPECT_EQ(all_metadata.FindInt(kVersionField),
              PolkadotChainMetadataPrefs::kVersion);
  }
}

TEST_F(PolkadotChainMetadataPrefsUnitTest, GetChainMetadataMissingEntry) {
  PolkadotChainMetadataPrefs prefs = MakePrefs();
  EXPECT_FALSE(prefs.GetChainMetadata(mojom::kPolkadotMainnet));
}

TEST_F(PolkadotChainMetadataPrefsUnitTest, InvalidRangeRejected) {
  {
    ScopedDictPrefUpdate update(&profile_prefs_,
                                kBraveWalletPolkadotChainMetadata);
    update->Set(kVersionField, PolkadotChainMetadataPrefs::kVersion);
    base::DictValue value;
    value.Set(kSystemPalletIndex, 0);
    value.Set(kBalancesPalletIndex, 999);  // Out of uint8 range.
    value.Set(kTransactionPaymentPalletIndex, 1);
    value.Set(kTransferAllowDeathCallIndex, 1);
    value.Set(kTransferKeepAliveCallIndex, 1);
    value.Set(kTransferAllCallIndex, 1);
    value.Set(kAssetTxPayment, false);
    value.Set(kSs58Prefix, 0);
    value.Set(kSpecVersion, 100);
    update->Set(mojom::kPolkadotMainnet, std::move(value));
  }
  PolkadotChainMetadataPrefs prefs = MakePrefs();
  EXPECT_FALSE(prefs.GetChainMetadata(mojom::kPolkadotMainnet));
}

TEST_F(PolkadotChainMetadataPrefsUnitTest, NegativeValueRejected) {
  {
    ScopedDictPrefUpdate update(&profile_prefs_,
                                kBraveWalletPolkadotChainMetadata);
    update->Set(kVersionField, PolkadotChainMetadataPrefs::kVersion);
    base::DictValue value;
    value.Set(kSystemPalletIndex, 0);
    value.Set(kBalancesPalletIndex, 1);
    value.Set(kTransactionPaymentPalletIndex, 1);
    value.Set(kTransferAllowDeathCallIndex,
              -1);  // Negative should be rejected.
    value.Set(kTransferKeepAliveCallIndex, 1);
    value.Set(kTransferAllCallIndex, 1);
    value.Set(kAssetTxPayment, false);
    value.Set(kSs58Prefix, 0);
    value.Set(kSpecVersion, 100);
    update->Set(mojom::kPolkadotMainnet, std::move(value));
  }
  PolkadotChainMetadataPrefs prefs = MakePrefs();
  EXPECT_FALSE(prefs.GetChainMetadata(mojom::kPolkadotMainnet));
}

TEST_F(PolkadotChainMetadataPrefsUnitTest, MissingRequiredFieldRejected) {
  {
    ScopedDictPrefUpdate update(&profile_prefs_,
                                kBraveWalletPolkadotChainMetadata);
    update->Set(kVersionField, PolkadotChainMetadataPrefs::kVersion);
    base::DictValue value;
    value.Set(kSystemPalletIndex, 0);
    value.Set(kBalancesPalletIndex, 3);
    value.Set(kTransactionPaymentPalletIndex, 2);
    value.Set(kTransferAllowDeathCallIndex, 1);
    value.Set(kTransferKeepAliveCallIndex, 4);
    value.Set(kTransferAllCallIndex, 1);
    value.Set(kAssetTxPayment, false);
    value.Set(kSs58Prefix, 42);
    // Missing spec version should reject persisted value.
    update->Set(mojom::kPolkadotMainnet, std::move(value));
  }
  PolkadotChainMetadataPrefs prefs = MakePrefs();
  EXPECT_FALSE(prefs.GetChainMetadata(mojom::kPolkadotMainnet));

  {
    // Missing AssetTxPayment.

    ScopedDictPrefUpdate update(&profile_prefs_,
                                kBraveWalletPolkadotChainMetadata);
    update->Set(kVersionField, PolkadotChainMetadataPrefs::kVersion);
    base::DictValue value;
    value.Set(kSystemPalletIndex, 0);
    value.Set(kBalancesPalletIndex, 3);
    value.Set(kTransactionPaymentPalletIndex, 2);
    value.Set(kTransferAllowDeathCallIndex, 1);
    value.Set(kTransferKeepAliveCallIndex, 4);
    value.Set(kTransferAllCallIndex, 1);
    value.Set(kSs58Prefix, 42);
    value.Set(kSpecVersion, 1234);
    update->Set(mojom::kPolkadotMainnet, std::move(value));
  }
  EXPECT_FALSE(MakePrefs().GetChainMetadata(mojom::kPolkadotMainnet));
}

TEST_F(PolkadotChainMetadataPrefsUnitTest, VersionMismatchClearsMetadataPrefs) {
  {
    ScopedDictPrefUpdate update(&profile_prefs_,
                                kBraveWalletPolkadotChainMetadata);
    update->Set(kVersionField, PolkadotChainMetadataPrefs::kVersion + 1);
    base::DictValue value;
    value.Set(kSystemPalletIndex, 0);
    value.Set(kBalancesPalletIndex, 3);
    value.Set(kTransactionPaymentPalletIndex, 2);
    value.Set(kTransferAllowDeathCallIndex, 1);
    value.Set(kTransferKeepAliveCallIndex, 4);
    value.Set(kTransferAllCallIndex, 1);
    value.Set(kAssetTxPayment, false);
    value.Set(kSs58Prefix, 42);
    value.Set(kSpecVersion, 100);
    update->Set(mojom::kPolkadotMainnet, std::move(value));
  }
  PolkadotChainMetadataPrefs prefs = MakePrefs();
  EXPECT_FALSE(prefs.GetChainMetadata(mojom::kPolkadotMainnet));
  EXPECT_TRUE(
      profile_prefs_.GetDict(kBraveWalletPolkadotChainMetadata).empty());
}

TEST_F(PolkadotChainMetadataPrefsUnitTest, MissingVersionClearsMetadataPrefs) {
  {
    ScopedDictPrefUpdate update(&profile_prefs_,
                                kBraveWalletPolkadotChainMetadata);
    base::DictValue value;
    value.Set(kSystemPalletIndex, 0);
    value.Set(kBalancesPalletIndex, 3);
    value.Set(kTransactionPaymentPalletIndex, 2);
    value.Set(kTransferAllowDeathCallIndex, 1);
    value.Set(kTransferKeepAliveCallIndex, 4);
    value.Set(kTransferAllCallIndex, 1);
    value.Set(kAssetTxPayment, false);
    value.Set(kSs58Prefix, 42);
    value.Set(kSpecVersion, 100);
    update->Set(mojom::kPolkadotMainnet, std::move(value));
  }
  PolkadotChainMetadataPrefs prefs = MakePrefs();
  EXPECT_FALSE(prefs.GetChainMetadata(mojom::kPolkadotMainnet));
  EXPECT_TRUE(
      profile_prefs_.GetDict(kBraveWalletPolkadotChainMetadata).empty());
}

}  // namespace
}  // namespace brave_wallet
