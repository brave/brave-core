/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_manager.h"

#include <memory>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_forward.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/account_resolver_delegate_impl.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

class PolkadotTxManagerUnitTest : public testing::Test {
 public:
  PolkadotTxManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletPolkadotFeature);

    RegisterProfilePrefs(profile_prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefsForMigration(profile_prefs_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&profile_prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        shared_url_loader_factory_, network_manager_.get(), &profile_prefs_,
        &local_state_);
    keyring_service_ = std::make_unique<KeyringService>(
        json_rpc_service_.get(), &profile_prefs_, &local_state_);

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    polkadot_wallet_service_ = std::make_unique<PolkadotWalletService>(
        *keyring_service_, *network_manager_, shared_url_loader_factory_);

    tx_service_ = std::make_unique<TxService>(
        json_rpc_service_.get(), nullptr, nullptr, nullptr,
        polkadot_wallet_service_.get(), *keyring_service_, &profile_prefs_,
        temp_dir_.GetPath(), base::SequencedTaskRunner::GetCurrentDefault());

    WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());

    account_resolver_delegate_ =
        std::make_unique<AccountResolverDelegateImpl>(*keyring_service_);

    polkadot_tx_manager_ = std::make_unique<PolkadotTxManager>(
        *tx_service_, *polkadot_wallet_service_, *keyring_service_,
        *tx_service_->GetDelegateForTesting(), *account_resolver_delegate_);

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);

    polkadot_mainnet_account_ = keyring_service_->AddAccountSync(
        mojom::CoinType::DOT, mojom::KeyringId::kPolkadotMainnet,
        "mainnet_account");
    polkadot_testnet_account_ = keyring_service_->AddAccountSync(
        mojom::CoinType::DOT, mojom::KeyringId::kPolkadotTestnet,
        "testnet_account");

    UnlockWallet();
    EXPECT_EQ(url_loader_factory_.NumPending(), 2);
  }

  void UnlockWallet() {
    keyring_service_->Unlock(
        kTestWalletPassword,
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, bool unlocked) {
              EXPECT_TRUE(unlocked);
              quit_closure.Run();
            },
            task_environment_.QuitClosure()));

    task_environment_.RunUntilQuit();
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

 protected:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedFeatureList feature_list_;
  mojom::AccountInfoPtr polkadot_mainnet_account_;
  mojom::AccountInfoPtr polkadot_testnet_account_;

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  base::ScopedTempDir temp_dir_;

  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<PolkadotWalletService> polkadot_wallet_service_;
  std::unique_ptr<TxService> tx_service_;
  std::unique_ptr<AccountResolverDelegateImpl> account_resolver_delegate_;
  std::unique_ptr<PolkadotTxManager> polkadot_tx_manager_;
};

TEST_F(PolkadotTxManagerUnitTest, GetCoinType) {
  EXPECT_EQ(polkadot_tx_manager_->GetCoinType(), mojom::CoinType::DOT);
}

TEST_F(PolkadotTxManagerUnitTest, AddUnapprovedTransaction) {
  auto tx_data_union = mojom::TxDataUnion::NewPolkadotTxData(
      mojom::PolkadotTxdata::New("", Uint128ToMojom(uint128_t{0}),
                                 Uint128ToMojom(uint128_t{0}), false));

  auto account_id = mojom::AccountId::New();
  account_id->coin = mojom::CoinType::DOT;
  account_id->keyring_id = mojom::KeyringId::kPolkadotMainnet;
  account_id->kind = mojom::AccountKind::kDerived;
  account_id->address = "test_address";

  polkadot_tx_manager_->AddUnapprovedTransaction(
      "polkadot_mainnet", std::move(tx_data_union), account_id, std::nullopt,
      nullptr,
      base::BindOnce([](bool success, const std::string& tx_meta_id,
                        const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(tx_meta_id.empty());
        EXPECT_EQ(error_message, "Not implemented");
      }));
}

namespace {

// Use the BOB account here:
// https://westend.subscan.io/account/5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
inline constexpr const char kBob[] =
    "0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48";

inline constexpr const char kBobSS58[] =
    "14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3";

}  // namespace

TEST_F(PolkadotTxManagerUnitTest, AddUnapprovedPolkadotTransaction) {
  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  {
    // Normal happy path flow of well-formatted data into an accepted unapproved
    // transaction committed to storage.

    std::string chain_id = mojom::kPolkadotMainnet;

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, polkadot_mainnet_account_->account_id->Clone(), kBob,
        mojom::uint128::New(0, 1234), false, nullptr);

    polkadot_tx_manager_->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params),
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, TxService* tx_service,
               bool success, const std::string& tx_meta_id,
               const std::string& err_str) {
              EXPECT_TRUE(success);
              EXPECT_FALSE(tx_meta_id.empty());
              EXPECT_EQ(err_str, "");

              const auto& txs = tx_service->GetDelegateForTesting()->GetTxs();
              const auto* tx = txs.FindDict(tx_meta_id);
              EXPECT_TRUE(tx);

              const auto* polkadot_tx = tx->FindDict("tx");

              EXPECT_EQ(
                  *polkadot_tx->FindString("recipient"),
                  R"(8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48)");
              EXPECT_EQ(*polkadot_tx->FindString("amount"),
                        "d2040000000000000000000000000000");
              EXPECT_EQ(*polkadot_tx->FindString("fee"),
                        "00000000000000000000000000000000");
              EXPECT_EQ(*polkadot_tx->FindBool("transfer_all"), false);
              EXPECT_EQ(polkadot_tx->FindInt("ss58_prefix"), std::nullopt);

              quit_closure.Run();
            },
            task_environment_.QuitClosure(), tx_service_.get()));

    task_environment_.RunUntilQuit();
  }

  {
    // Send a u128::MAX.

    std::string chain_id = mojom::kPolkadotMainnet;

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, polkadot_mainnet_account_->account_id->Clone(), kBobSS58,
        mojom::uint128::New(0xffffffffffffffff, 0xffffffffffffffff), false,
        nullptr);

    polkadot_tx_manager_->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params),
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, TxService* tx_service,
               bool success, const std::string& tx_meta_id,
               const std::string& err_str) {
              EXPECT_TRUE(success);
              EXPECT_FALSE(tx_meta_id.empty());
              EXPECT_EQ(err_str, "");

              const auto& txs = tx_service->GetDelegateForTesting()->GetTxs();
              const auto* tx = txs.FindDict(tx_meta_id);
              EXPECT_TRUE(tx);

              const auto* polkadot_tx = tx->FindDict("tx");

              EXPECT_EQ(
                  *polkadot_tx->FindString("recipient"),
                  R"(8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48)");
              EXPECT_EQ(*polkadot_tx->FindString("amount"),
                        "ffffffffffffffffffffffffffffffff");
              EXPECT_EQ(*polkadot_tx->FindString("fee"),
                        "00000000000000000000000000000000");
              EXPECT_EQ(*polkadot_tx->FindBool("transfer_all"), false);
              EXPECT_EQ(*polkadot_tx->FindInt("ss58_prefix"), 0);

              quit_closure.Run();
            },
            task_environment_.QuitClosure(), tx_service_.get()));

    task_environment_.RunUntilQuit();
  }

  {
    // Provide an invalid destination address to the backend.

    std::string chain_id = mojom::kPolkadotMainnet;

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, polkadot_mainnet_account_->account_id->Clone(), "0x1234",
        mojom::uint128::New(0, 1234), false, nullptr);

    polkadot_tx_manager_->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params),
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, bool success,
               const std::string& tx_meta_id, const std::string& err_str) {
              EXPECT_FALSE(success);
              EXPECT_TRUE(tx_meta_id.empty());
              EXPECT_EQ(err_str, WalletInternalErrorMessage());

              quit_closure.Run();
            },
            task_environment_.QuitClosure()));

    task_environment_.RunUntilQuit();
  }

  {
    // Provide an incompatible ss58-based address.

    std::string chain_id = mojom::kPolkadotMainnet;

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, polkadot_mainnet_account_->account_id->Clone(),
        "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty",
        mojom::uint128::New(0, 1234), false, nullptr);

    polkadot_tx_manager_->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params),
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, bool success,
               const std::string& tx_meta_id, const std::string& err_str) {
              EXPECT_FALSE(success);
              EXPECT_TRUE(tx_meta_id.empty());
              EXPECT_EQ(err_str, WalletInternalErrorMessage());

              quit_closure.Run();
            },
            task_environment_.QuitClosure()));

    task_environment_.RunUntilQuit();
  }

  {
    // Provide an invalid chain_id to the backend (i.e. not Polkadot).

    std::string chain_id = mojom::kZCashTestnet;

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, polkadot_mainnet_account_->account_id->Clone(), kBob,
        mojom::uint128::New(0, 1234), false, nullptr);

    polkadot_tx_manager_->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params),
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, bool success,
               const std::string& tx_meta_id, const std::string& err_str) {
              EXPECT_FALSE(success);
              EXPECT_TRUE(tx_meta_id.empty());
              EXPECT_EQ(err_str, WalletInternalErrorMessage());

              quit_closure.Run();
            },
            task_environment_.QuitClosure()));

    task_environment_.RunUntilQuit();
  }

  {
    // Provide an invalid account id (test account resolution failure).

    std::string chain_id = mojom::kPolkadotMainnet;

    auto account_id = polkadot_mainnet_account_->account_id->Clone();
    account_id->address = "invalid_address";

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, std::move(account_id), kBob, mojom::uint128::New(0, 1234),
        false, nullptr);

    tx_service_->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params),
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, bool success,
               const std::string& tx_meta_id, const std::string& err_str) {
              EXPECT_FALSE(success);
              EXPECT_TRUE(tx_meta_id.empty());
              EXPECT_EQ(err_str, l10n_util::GetStringUTF8(
                                     IDS_WALLET_SEND_TRANSACTION_FROM_EMPTY));

              quit_closure.Run();
            },
            task_environment_.QuitClosure()));

    task_environment_.RunUntilQuit();
  }
}

TEST_F(PolkadotTxManagerUnitTest,
       AddUnapprovedPolkadotTransaction_InvalidChainData) {
  // Test the transaction manager when the remote RPC nodes have given us
  // invalid chain data or we've failed the network request (we should be
  // storing a `base::unexpected` in both of these cases).

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  // Note that these are error responses, can't be parsed.
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "error": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "error": "Polkadot",
      "id": 1 })");

  std::string chain_id = mojom::kPolkadotMainnet;

  auto transaction_params = mojom::NewPolkadotTransactionParams::New(
      chain_id, polkadot_mainnet_account_->account_id.Clone(), kBob,
      mojom::uint128::New(0, 1234), false, nullptr);

  polkadot_tx_manager_->AddUnapprovedPolkadotTransaction(
      std::move(transaction_params),
      base::BindOnce(
          [](base::RepeatingClosure quit_closure, bool success,
             const std::string& tx_meta_id, const std::string& err_str) {
            EXPECT_FALSE(success);
            EXPECT_TRUE(tx_meta_id.empty());
            EXPECT_NE(err_str, "");

            quit_closure.Run();
          },
          task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();
}

TEST_F(PolkadotTxManagerUnitTest, ApproveTransaction) {
  polkadot_tx_manager_->ApproveTransaction(
      "test_tx_id",
      base::BindOnce([](bool success, mojom::ProviderErrorUnionPtr error_union,
                        const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(error_union);
        EXPECT_EQ(error_message, "Not implemented");
      }));
}

TEST_F(PolkadotTxManagerUnitTest, SpeedupOrCancelTransaction) {
  polkadot_tx_manager_->SpeedupOrCancelTransaction(
      "test_tx_id", false,  // false = speedup, true = cancel
      base::BindOnce([](bool success, const std::string& tx_meta_id,
                        const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(tx_meta_id.empty());
        EXPECT_EQ(error_message, "Not implemented");
      }));
}

TEST_F(PolkadotTxManagerUnitTest, RetryTransaction) {
  polkadot_tx_manager_->RetryTransaction(
      "test_tx_id",
      base::BindOnce([](bool success, const std::string& tx_meta_id,
                        const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(tx_meta_id.empty());
        EXPECT_EQ(error_message, "Not implemented");
      }));
}

}  // namespace brave_wallet
