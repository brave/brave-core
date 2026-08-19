/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_tx_manager.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// Mainnet unified address and the transparent receiver it wraps.
// https://github.com/Electric-Coin-Company/zcash-android-wallet-sdk/blob/v2.0.6/sdk-incubator-lib/src/main/java/cash/z/ecc/android/sdk/fixture/WalletFixture.kt
constexpr char kUnifiedAddress[] =
    "u1lmy8anuylj33arxh3sx7ysq54tuw7zehsv6pdeeaqlrhkjhm3uvl9egqxqfd7hcsp3mszp6"
    "jxxx0gsw0ldp5wyu95r4mfzlueh8h5xhrjqgz7xtxp3hvw45dn4gfrz5j54ryg6reyf0";
constexpr char kTransparentAddress[] = "t1JP7PHu72xHztsZiwH6cye4yvC9Prb3EvQ";

}  // namespace

class ZCashTxManagerUnitTest : public testing::Test {
 public:
  ZCashTxManagerUnitTest() = default;

  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        url_loader_factory_.GetSafeWeakWrapper(), network_manager_.get(),
        &prefs_, &local_state_);
    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        &prefs_, &local_state_);

    zcash_wallet_service_ = std::make_unique<ZCashWalletService>(
        *keyring_service_,
        std::make_unique<ZCashRpc>(network_manager_.get(),
                                   url_loader_factory_.GetSafeWeakWrapper()));

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    tx_service_ = std::make_unique<TxService>(
        json_rpc_service_.get(), nullptr, zcash_wallet_service_.get(), nullptr,
        nullptr, *keyring_service_, &prefs_,
        CreateTxStorageForTest(temp_dir_.GetPath()));

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
    zcash_account_ = GetAccountUtils().EnsureZecAccount(0);
    ASSERT_TRUE(zcash_account_);
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  // Adds a transaction sending to `address_to` and returns the error message.
  // None of these tests get far enough to create a transaction.
  std::string AddUnapprovedTransaction(const std::string& address_to) {
    base::test::TestFuture<bool, const std::string&, const std::string&> future;
    tx_service_->AddUnapprovedZCashTransaction(
        mojom::NewZCashTransactionParams::New(
            mojom::kZCashMainnet, zcash_account_->account_id->Clone(),
            address_to, 1000u, /*sending_max_amount=*/false,
            /*memo=*/std::nullopt, mojom::ZCashTokenType::kTransparent,
            /*swap_info=*/nullptr),
        future.GetCallback());

    const auto& [success, tx_meta_id, error] = future.Get();
    EXPECT_FALSE(success);
    EXPECT_TRUE(tx_meta_id.empty());
    return error;
  }

 protected:
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletZCashFeature};
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;

  mojom::AccountInfoPtr zcash_account_;

  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<ZCashWalletService> zcash_wallet_service_;
  std::unique_ptr<TxService> tx_service_;
};

TEST_F(ZCashTxManagerUnitTest, RestrictedTransparentRecipient) {
  BlockchainRegistry::ScopedRestrictedAddressesForTesting scoped_restricted(
      {base::ToLowerASCII(kTransparentAddress)});

  std::string address_to = kTransparentAddress;

  base::test::TestFuture<bool, const std::string&, const std::string&> future;
  tx_service_->AddUnapprovedZCashTransaction(
      mojom::NewZCashTransactionParams::New(
          mojom::kZCashMainnet, zcash_account_->account_id->Clone(), address_to,
          1000u, /*sending_max_amount=*/false,
          /*memo=*/std::nullopt, mojom::ZCashTokenType::kTransparent,
          /*swap_info=*/nullptr),
      future.GetCallback());

  const auto& [success, tx_meta_id, error] = future.Get();
  EXPECT_FALSE(success);
  EXPECT_TRUE(tx_meta_id.empty());
  EXPECT_EQ(error, WalletInternalErrorMessage());
}

TEST_F(ZCashTxManagerUnitTest, RestrictedUnifiedRecipientTransparentPart) {
  // A unified address must be rejected when its transparent receiver is
  // restricted: that receiver is what ends up in the transaction output.

  ASSERT_EQ(ExtractTransparentPart(kUnifiedAddress, /*is_testnet=*/false),
            kTransparentAddress);

  BlockchainRegistry::ScopedRestrictedAddressesForTesting scoped_restricted(
      {base::ToLowerASCII(kTransparentAddress)});

  std::string address_to = kUnifiedAddress;

  base::test::TestFuture<bool, const std::string&, const std::string&> future;
  tx_service_->AddUnapprovedZCashTransaction(
      mojom::NewZCashTransactionParams::New(
          mojom::kZCashMainnet, zcash_account_->account_id->Clone(), address_to,
          1000u, /*sending_max_amount=*/false,
          /*memo=*/std::nullopt, mojom::ZCashTokenType::kTransparent,
          /*swap_info=*/nullptr),
      future.GetCallback());

  const auto& [success, tx_meta_id, error] = future.Get();
  EXPECT_FALSE(success);
  EXPECT_TRUE(tx_meta_id.empty());
  EXPECT_EQ(error, WalletInternalErrorMessage());
}

}  // namespace brave_wallet
