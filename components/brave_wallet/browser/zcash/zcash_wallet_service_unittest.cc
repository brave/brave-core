/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "build/build_config.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_ORCHARD)
#include "base/task/sequenced_task_runner.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_orchard_storage.h"
#endif

using testing::_;
using testing::Eq;
using testing::SaveArg;
using testing::Truly;
using testing::WithArg;

namespace brave_wallet {

namespace {

std::array<uint8_t, 32> GetTxId(const std::string& hex_string) {
  std::vector<uint8_t> vec;
  std::array<uint8_t, 32> sized_vec;

  base::HexStringToBytes(hex_string, &vec);
  std::reverse(vec.begin(), vec.end());
  std::copy_n(vec.begin(), 32, sized_vec.begin());
  return sized_vec;
}

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override {}
  MOCK_METHOD3(GetUtxoList,
               void(const std::string& chain_id,
                    const std::string& address,
                    GetUtxoListCallback callback));

  MOCK_METHOD2(GetLatestBlock,
               void(const std::string& chain_id,
                    GetLatestBlockCallback callback));

  MOCK_METHOD3(GetTransaction,
               void(const std::string& chain_id,
                    const std::string& tx_hash,
                    GetTransactionCallback callback));

  MOCK_METHOD3(SendTransaction,
               void(const std::string& chain_id,
                    base::span<const uint8_t> data,
                    SendTransactionCallback callback));

  MOCK_METHOD5(IsKnownAddress,
               void(const std::string& chain_id,
                    const std::string& addr,
                    uint64_t block_start,
                    uint64_t block_end,
                    IsKnownAddressCallback callback));

  MOCK_METHOD2(GetLatestTreeState,
               void(const std::string& chain_id,
                    GetTreeStateCallback callback));

  MOCK_METHOD3(GetTreeState,
               void(const std::string& chain_id,
                    zcash::mojom::BlockIDPtr block_id,
                    GetTreeStateCallback callback));
};

}  // namespace

class ZCashWalletServiceUnitTest : public testing::Test {
 public:
  ZCashWalletServiceUnitTest() {}

  ~ZCashWalletServiceUnitTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}});
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath db_path(
        temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    auto zcash_rpc = std::make_unique<testing::NiceMock<MockZCashRPC>>();
    zcash_wallet_service_ = std::make_unique<ZCashWalletService>(
        db_path, keyring_service_.get(), std::move(zcash_rpc));
    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
    zcash_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    ASSERT_TRUE(zcash_account_);
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  mojom::AccountIdPtr account_id() const {
    return zcash_account_->account_id.Clone();
  }

  testing::NiceMock<MockZCashRPC>* zcash_rpc() {
    return static_cast<testing::NiceMock<MockZCashRPC>*>(
        zcash_wallet_service_->zcash_rpc());
  }

#if BUILDFLAG(ENABLE_ORCHARD)
  base::SequenceBound<ZCashOrchardStorage>& orchard_storage() {
    return zcash_wallet_service_->orchard_storage();
  }
#endif  // BUILDFLAG(ENABLE_ORCHARD)

  KeyringService* keyring_service() { return keyring_service_.get(); }

 protected:
  base::test::ScopedFeatureList feature_list_;
  base::ScopedTempDir temp_dir_;

  mojom::AccountInfoPtr zcash_account_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<ZCashWalletService> zcash_wallet_service_;

  base::test::TaskEnvironment task_environment_;
};

TEST_F(ZCashWalletServiceUnitTest, GetBalance) {
  GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 1);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 1, 0);

  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                2625446u,
                *PrefixedHexStringToBytes("0x0000000001a01b5fd794e4b071443974c8"
                                          "35b3e0ff8f96bf3600e07afdbf89c5"));
            std::move(callback).Run(std::move(response));
          }));

  ON_CALL(*zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, const std::string& addr,
             uint64_t block_start, uint64_t block_end,
             ZCashRpc::IsKnownAddressCallback callback) {
            // Receiver addresses
            if (addr == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
              std::move(callback).Run(true);
              return;
            }
            if (addr == "t1aW1cW7wf6KMuKrjDinyv9tK6F6hrBkRAY") {
              std::move(callback).Run(true);
              return;
            }
            if (addr == "t1aW1cW7wf6KMuKrjDinyv9tK6F6hrBkRAY") {
              std::move(callback).Run(true);
              return;
            }
            if (addr == "t1MF6q7rTYJMMKLgzQ58mCuo76EVhLfSAkW") {
              std::move(callback).Run(true);
              return;
            }
            std::move(callback).Run(false);
          }));

  ON_CALL(*zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  10u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            } else if (address == "t1aW1cW7wf6KMuKrjDinyv9tK6F6hrBkRAY") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  20u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            } else if (address == "t1MF6q7rTYJMMKLgzQ58mCuo76EVhLfSAkW") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  20u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          }));

  base::MockCallback<ZCashWalletService::GetBalanceCallback> balance_callback;
  EXPECT_CALL(balance_callback, Run(_, _))
      .WillOnce(::testing::Invoke([&](mojom::ZCashBalancePtr balance,
                                      std::optional<std::string> error) {
        EXPECT_EQ(balance->total_balance, 50u);
        EXPECT_EQ(balance->transparent_balance, 50u);
        EXPECT_EQ(balance->shielded_balance, 0u);
      }));
  zcash_wallet_service_->GetBalance(mojom::kZCashMainnet, account_id.Clone(),
                                    balance_callback.Get());
  task_environment_.RunUntilIdle();
}

#if BUILDFLAG(ENABLE_ORCHARD)
TEST_F(ZCashWalletServiceUnitTest, GetBalanceWithShielded) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 1);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 1, 0);

  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                2625446u,
                *PrefixedHexStringToBytes("0x0000000001a01b5fd794e4b071443974c8"
                                          "35b3e0ff8f96bf3600e07afdbf89c5"));
            std::move(callback).Run(std::move(response));
          }));

  ON_CALL(*zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, const std::string& addr,
             uint64_t block_start, uint64_t block_end,
             ZCashRpc::IsKnownAddressCallback callback) {
            // Receiver addresses
            if (addr == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
              std::move(callback).Run(true);
              return;
            }
            std::move(callback).Run(false);
          }));

  ON_CALL(*zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  10u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          }));

  OrchardNote note;
  note.amount = 10u;

  auto update_notes_callback = base::BindLambdaForTesting(
      [](std::optional<ZCashOrchardStorage::Error>) {});

  orchard_storage()
      .AsyncCall(&ZCashOrchardStorage::UpdateNotes)
      .WithArgs(account_id.Clone(), std::vector<OrchardNote>({note}),
                std::vector<OrchardNullifier>(), 50000, "hash50000")
      .Then(std::move(update_notes_callback));

  task_environment_.RunUntilIdle();

  base::MockCallback<ZCashWalletService::GetBalanceCallback> balance_callback;
  EXPECT_CALL(balance_callback, Run(_, _))
      .WillOnce(::testing::Invoke([&](mojom::ZCashBalancePtr balance,
                                      std::optional<std::string> error) {
        EXPECT_EQ(balance->total_balance, 20u);
        EXPECT_EQ(balance->transparent_balance, 10u);
        EXPECT_EQ(balance->shielded_balance, 10u);
      }));
  zcash_wallet_service_->GetBalance(mojom::kZCashMainnet, account_id.Clone(),
                                    balance_callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ZCashWalletServiceUnitTest, GetBalanceWithShielded_FeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "false"}});

  GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 1);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 1, 0);

  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                2625446u,
                *PrefixedHexStringToBytes("0x0000000001a01b5fd794e4b071443974c8"
                                          "35b3e0ff8f96bf3600e07afdbf89c5"));
            std::move(callback).Run(std::move(response));
          }));

  ON_CALL(*zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, const std::string& addr,
             uint64_t block_start, uint64_t block_end,
             ZCashRpc::IsKnownAddressCallback callback) {
            // Receiver addresses
            if (addr == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
              std::move(callback).Run(true);
              return;
            }
            std::move(callback).Run(false);
          }));

  ON_CALL(*zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  10u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          }));

  OrchardNote note;
  note.amount = 10u;

  auto update_notes_callback = base::BindLambdaForTesting(
      [](std::optional<ZCashOrchardStorage::Error>) {});

  orchard_storage()
      .AsyncCall(&ZCashOrchardStorage::UpdateNotes)
      .WithArgs(account_id.Clone(), std::vector<OrchardNote>({note}),
                std::vector<OrchardNullifier>(), 50000, "hash50000")
      .Then(std::move(update_notes_callback));

  task_environment_.RunUntilIdle();

  base::MockCallback<ZCashWalletService::GetBalanceCallback> balance_callback;
  EXPECT_CALL(balance_callback, Run(_, _))
      .WillOnce(::testing::Invoke([&](mojom::ZCashBalancePtr balance,
                                      std::optional<std::string> error) {
        EXPECT_EQ(balance->total_balance, 10u);
        EXPECT_EQ(balance->transparent_balance, 10u);
        EXPECT_EQ(balance->shielded_balance, 0u);
      }));
  zcash_wallet_service_->GetBalance(mojom::kZCashMainnet, account_id.Clone(),
                                    balance_callback.Get());
  task_environment_.RunUntilIdle();
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

// https://zcashblockexplorer.com/transactions/3bc513afc84befb9774f667eb4e63266a7229ab1fdb43476dd7c3a33d16b3101/raw
TEST_F(ZCashWalletServiceUnitTest, SignAndPostTransaction) {
  {
    auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);
    keyring_service_->UpdateNextUnusedAddressForZCashAccount(account_id, 2, 2);
  }

  ZCashTransaction zcash_transaction;
  zcash_transaction.set_locktime(2286687);
  {
    ZCashTransaction::TxInput input;
    input.utxo_outpoint.txid = GetTxId(
        "70f1aa91889eee3e5ba60231a2e625e60480dc2e43ddc9439dc4fe8f09a1a278");
    input.utxo_outpoint.index = 0;

    input.utxo_address = "t1c61yifRMgyhMsBYsFDBa5aEQkgU65CGau";
    input.utxo_value = 537000;
    input.script_pub_key =
        ZCashAddressToScriptPubkey(input.utxo_address, false);

    zcash_transaction.transparent_part().inputs.push_back(std::move(input));
  }

  {
    ZCashTransaction::TxOutput output;
    output.address = "t1KrG29yWzoi7Bs2pvsgXozZYPvGG4D3sGi";
    output.amount = 500000;
    output.script_pubkey = ZCashAddressToScriptPubkey(output.address, false);

    zcash_transaction.transparent_part().outputs.push_back(std::move(output));
  }

  {
    ZCashTransaction::TxOutput output;
    output.address = "t1c61yifRMgyhMsBYsFDBa5aEQkgU65CGau";
    output.script_pubkey = ZCashAddressToScriptPubkey(output.address, false);
    output.amount = 35000;

    zcash_transaction.transparent_part().outputs.push_back(std::move(output));
  }

  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            zcash::mojom::BlockIDPtr response = zcash::mojom::BlockID::New();
            response->height = 2286687;
            std::move(callback).Run(std::move(response));
          }));

  base::MockCallback<ZCashWalletService::SignAndPostTransactionCallback>
      sign_callback;

  ZCashTransaction signed_tx;
  EXPECT_CALL(
      sign_callback,
      Run("3bc513afc84befb9774f667eb4e63266a7229ab1fdb43476dd7c3a33d16b3101", _,
          ""))
      .WillOnce(
          WithArg<1>([&](const ZCashTransaction& tx) { signed_tx = tx; }));

  std::vector<uint8_t> captured_data;
  EXPECT_CALL(*zcash_rpc(), SendTransaction(_, _, _))
      .WillOnce([&](const std::string& chain_id, base::span<const uint8_t> data,
                    ZCashRpc::SendTransactionCallback callback) {
        captured_data = std::vector<uint8_t>(data.begin(), data.end());
        zcash::mojom::SendResponsePtr response =
            zcash::mojom::SendResponse::New();
        response->error_code = 0;
        std::move(callback).Run(std::move(response));
      });

  zcash_wallet_service_->SignAndPostTransaction(
      mojom::kZCashMainnet, account_id(), std::move(zcash_transaction),
      sign_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&sign_callback);

  EXPECT_EQ(ToHex(signed_tx.transparent_part().inputs[0].script_sig),
            "0x47304402202fc68ead746e8e93bb661ac79e71e1d3d84fd0f2aac76a8cb"
            "4fa831a847787ff022028efe32152f282d7167c40d62b07aedad73a66c7"
            "a3548413f289e2aef3da96b30121028754aaa5d9198198ecf5fd1849cbf"
            "38a92ed707e2f181bd354c73a4a87854c67");

  EXPECT_EQ(ToHex(captured_data),
            "0x050000800a27a726b4d0d6c25fe4220073e422000178a2a1098ffec49d43"
            "c9dd432edc8004e625e6a23102a65b3eee9e8891aaf170000000006a473044"
            "02202fc68ead746e8e93bb661ac79e71e1d3d84fd0f2aac76a8cb4fa831a84"
            "7787ff022028efe32152f282d7167c40d62b07aedad73a66c7a3548413f289"
            "e2aef3da96b30121028754aaa5d9198198ecf5fd1849cbf38a92ed707e2f18"
            "1bd354c73a4a87854c67ffffffff0220a10700000000001976a91415af26f9"
            "b71022a01eade958cd05145f7ba5afe688acb8880000000000001976a914c7"
            "cb443e547988b992adc1b47427ce6c40f3ca9e88ac000000");
}

TEST_F(ZCashWalletServiceUnitTest, AddressDiscovery) {
  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            zcash::mojom::BlockIDPtr response = zcash::mojom::BlockID::New();
            response->height = 2286687;
            std::move(callback).Run(std::move(response));
          }));

  ON_CALL(*zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, const std::string& addr,
             uint64_t block_start, uint64_t block_end,
             ZCashRpc::IsKnownAddressCallback callback) {
            EXPECT_EQ(2286687u, block_end);
            // Receiver addresses
            if (addr == "t1c61yifRMgyhMsBYsFDBa5aEQkgU65CGau") {
              std::move(callback).Run(true);
              return;
            }
            if (addr == "t1V7JBWXRYPA19nBLBFTm8669DhQgErMAnK") {
              std::move(callback).Run(true);
              return;
            }
            if (addr == "t1UCYMSUdkGXEyeKPqgwiDn8NwGv5JKmJoL") {
              std::move(callback).Run(false);
              return;
            }
            // Change addresses
            if (addr == "t1RDtGXzcfchmtrE8pGLorefgtspgcNZbrE") {
              std::move(callback).Run(true);
              return;
            }
            if (addr == "t1VUdyCuqWgeBPJvfhWvHLD5iDUfkdLrwWz") {
              std::move(callback).Run(true);
              return;
            }
            if (addr == "t1QJuws2nGqDNJEKsKniUPDNLbMw5R9ixGj") {
              std::move(callback).Run(false);
              return;
            }
          }));

  {
    bool callback_called = false;
    auto discovery_callback = base::BindLambdaForTesting(
        [&](ZCashWalletService::RunDiscoveryResult result) {
          EXPECT_EQ((*result)[0]->address_string,
                    "t1UCYMSUdkGXEyeKPqgwiDn8NwGv5JKmJoL");
          EXPECT_EQ((*result)[1]->address_string,
                    "t1QJuws2nGqDNJEKsKniUPDNLbMw5R9ixGj");
          callback_called = true;
        });

    auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);

    zcash_wallet_service_->RunDiscovery(std::move(account_id),
                                        std::move(discovery_callback));
    task_environment_.RunUntilIdle();

    EXPECT_TRUE(callback_called);
  }

  ON_CALL(*zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, const std::string& addr,
             uint64_t block_start, uint64_t block_end,
             ZCashRpc::IsKnownAddressCallback callback) {
            EXPECT_EQ(2286687u, block_end);
            // Receiver addresses
            if (addr == "t1UCYMSUdkGXEyeKPqgwiDn8NwGv5JKmJoL") {
              std::move(callback).Run(true);
              return;
            }
            if (addr == "t1JEfEPQDGruzd7Q42pdwHmR4sRHGLRF48m") {
              std::move(callback).Run(false);
              return;
            }
            // Change addresses
            if (addr == "t1QJuws2nGqDNJEKsKniUPDNLbMw5R9ixGj") {
              std::move(callback).Run(true);
              return;
            }
            if (addr == "t1gKxueg76TtvVmMQ6swDmvHxtmLTSQv6KP") {
              std::move(callback).Run(false);
              return;
            }
          }));

  {
    bool callback_called = false;
    auto discovery_callback = base::BindLambdaForTesting(
        [&](ZCashWalletService::RunDiscoveryResult result) {
          EXPECT_EQ((*result)[0]->address_string,
                    "t1JEfEPQDGruzd7Q42pdwHmR4sRHGLRF48m");
          EXPECT_EQ((*result)[1]->address_string,
                    "t1gKxueg76TtvVmMQ6swDmvHxtmLTSQv6KP");
          callback_called = true;
        });

    auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);

    zcash_wallet_service_->RunDiscovery(std::move(account_id),
                                        std::move(discovery_callback));
    task_environment_.RunUntilIdle();

    EXPECT_TRUE(callback_called);
  }
}

TEST_F(ZCashWalletServiceUnitTest, AddressDiscovery_FromPrefs) {
  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            zcash::mojom::BlockIDPtr response = zcash::mojom::BlockID::New();
            response->height = 2286687;
            std::move(callback).Run(std::move(response));
          }));

  ON_CALL(*zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, const std::string& addr,
             uint64_t block_start, uint64_t block_end,
             ZCashRpc::IsKnownAddressCallback callback) {
            EXPECT_EQ(2286687u, block_end);
            // Receiver addresses
            if (addr == "t1UCYMSUdkGXEyeKPqgwiDn8NwGv5JKmJoL") {
              std::move(callback).Run(true);
              return;
            }
            if (addr == "t1JEfEPQDGruzd7Q42pdwHmR4sRHGLRF48m") {
              std::move(callback).Run(false);
              return;
            }
            // Change addresses
            if (addr == "t1RDtGXzcfchmtrE8pGLorefgtspgcNZbrE") {
              std::move(callback).Run(false);
              return;
            }
          }));

  {
    auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);
    keyring_service_->UpdateNextUnusedAddressForZCashAccount(account_id, 2,
                                                             std::nullopt);
  }

  {
    bool callback_called = false;
    auto discovery_callback = base::BindLambdaForTesting(
        [&](ZCashWalletService::RunDiscoveryResult result) {
          EXPECT_EQ((*result)[0]->address_string,
                    "t1JEfEPQDGruzd7Q42pdwHmR4sRHGLRF48m");
          EXPECT_EQ((*result)[1]->address_string,
                    "t1RDtGXzcfchmtrE8pGLorefgtspgcNZbrE");
          callback_called = true;
          callback_called = true;
        });

    auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);

    zcash_wallet_service_->RunDiscovery(std::move(account_id),
                                        std::move(discovery_callback));
    task_environment_.RunUntilIdle();

    EXPECT_TRUE(callback_called);
  }
}

TEST_F(ZCashWalletServiceUnitTest, ValidateZCashAddress) {
  // https://github.com/Electric-Coin-Company/zcash-android-wallet-sdk/blob/v2.0.6/sdk-incubator-lib/src/main/java/cash/z/ecc/android/sdk/fixture/WalletFixture.kt

  // Normal transparent address - mainnet
  {
    base::MockCallback<ZCashWalletService::ValidateZCashAddressCallback>
        callback;
    EXPECT_CALL(callback, Run(mojom::ZCashAddressValidationResult::Success));
    zcash_wallet_service_->ValidateZCashAddress(
        "t1JP7PHu72xHztsZiwH6cye4yvC9Prb3EvQ", false, callback.Get());
  }

  // Normal transparent address - testnet
  {
    base::MockCallback<ZCashWalletService::ValidateZCashAddressCallback>
        callback;
    EXPECT_CALL(callback, Run(mojom::ZCashAddressValidationResult::Success));
    zcash_wallet_service_->ValidateZCashAddress(
        "tmP3uLtGx5GPddkq8a6ddmXhqJJ3vy6tpTE", true, callback.Get());
  }

  // Testnet mismatch
  {
    base::MockCallback<ZCashWalletService::ValidateZCashAddressCallback>
        callback;
    EXPECT_CALL(callback,
                Run(mojom::ZCashAddressValidationResult::NetworkMismatch));
    zcash_wallet_service_->ValidateZCashAddress(
        "tmP3uLtGx5GPddkq8a6ddmXhqJJ3vy6tpTE", false, callback.Get());
  }

  // Wrong transparent address
  {
    base::MockCallback<ZCashWalletService::ValidateZCashAddressCallback>
        callback;
    EXPECT_CALL(callback,
                Run(mojom::ZCashAddressValidationResult::InvalidTransparent));
    zcash_wallet_service_->ValidateZCashAddress("t1xxx", false, callback.Get());
  }

  // Unified address - mainnet
  {
    base::MockCallback<ZCashWalletService::ValidateZCashAddressCallback>
        callback;
    EXPECT_CALL(callback, Run(mojom::ZCashAddressValidationResult::Success));
    zcash_wallet_service_->ValidateZCashAddress(
        "u1lmy8anuylj33arxh3sx7ysq54tuw7zehsv6pdeeaqlrhkjhm3uvl9egqxqfd7hcsp3ms"
        "zp6jxxx0gsw0ldp5wyu95r4mfzlueh8h5xhrjqgz7xtxp3hvw45dn4gfrz5j54ryg6reyf"
        "0",
        false, callback.Get());
  }

  // Unified address - testnet
  {
    base::MockCallback<ZCashWalletService::ValidateZCashAddressCallback>
        callback;
    EXPECT_CALL(callback, Run(mojom::ZCashAddressValidationResult::Success));
    zcash_wallet_service_->ValidateZCashAddress(
        "utest1vergg5jkp4xy8sqfasw6s5zkdpnxvfxlxh35uuc3me7dp596y2r05t6dv9htwe3p"
        "f8ksrfr8ksca2lskzjanqtl8uqp5vln3zyy246ejtx86vqftp73j7jg9099jxafyjhfm6u"
        "956j3",
        true, callback.Get());
  }

  // Unified address network mismatch
  {
    base::MockCallback<ZCashWalletService::ValidateZCashAddressCallback>
        callback;
    EXPECT_CALL(callback,
                Run(mojom::ZCashAddressValidationResult::NetworkMismatch));
    zcash_wallet_service_->ValidateZCashAddress(
        "utest1vergg5jkp4xy8sqfasw6s5zkdpnxvfxlxh35uuc3me7dp596y2r05t6dv9htwe3p"
        "f8ksrfr8ksca2lskzjanqtl8uqp5vln3zyy246ejtx86vqftp73j7jg9099jxafyjhfm6u"
        "956j3",
        false, callback.Get());
  }

  // Wrong unified address
  {
    base::MockCallback<ZCashWalletService::ValidateZCashAddressCallback>
        callback;
    EXPECT_CALL(callback,
                Run(mojom::ZCashAddressValidationResult::InvalidUnified));
    zcash_wallet_service_->ValidateZCashAddress("u1xx", false, callback.Get());
  }
}

#if BUILDFLAG(ENABLE_ORCHARD)

TEST_F(ZCashWalletServiceUnitTest, ValidateOrchardUnifiedAddress) {
  // Shielded addresses disabled
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "false"}});

    base::MockCallback<ZCashWalletService::ValidateZCashAddressCallback>
        callback;
    EXPECT_CALL(callback,
                Run(mojom::ZCashAddressValidationResult::InvalidUnified));
    zcash_wallet_service_->ValidateZCashAddress(
        "u1ay3aawlldjrmxqnjf5medr5ma6p3acnet464ht8lmwplq5cd3"
        "ugytcmlf96rrmtgwldc75x94qn4n8pgen36y8tywlq6yjk7lkf3"
        "fa8wzjrav8z2xpxqnrnmjxh8tmz6jhfh425t7f3vy6p4pd3zmqa"
        "yq49efl2c4xydc0gszg660q9p",
        false, callback.Get());
  }

  // Shielded addresses enabled
  {
    base::MockCallback<ZCashWalletService::ValidateZCashAddressCallback>
        callback;
    EXPECT_CALL(callback, Run(mojom::ZCashAddressValidationResult::Success));
    zcash_wallet_service_->ValidateZCashAddress(
        "u1ay3aawlldjrmxqnjf5medr5ma6p3acnet464ht8lmwplq5cd3"
        "ugytcmlf96rrmtgwldc75x94qn4n8pgen36y8tywlq6yjk7lkf3"
        "fa8wzjrav8z2xpxqnrnmjxh8tmz6jhfh425t7f3vy6p4pd3zmqa"
        "yq49efl2c4xydc0gszg660q9p",
        false, callback.Get());
  }
}

TEST_F(ZCashWalletServiceUnitTest, MakeAccountShielded) {
  keyring_service()->Reset();
  keyring_service()->RestoreWallet(
      "gate junior chunk maple cage select orange circle price air tortoise "
      "jelly art frequent fence middle ice moral wage toddler attitude sign "
      "lesson grain",
      kTestWalletPassword, false, base::DoNothing());
  GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);

  auto account_id_1 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);
  auto account_id_2 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 1);

  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                ZCashRpc::GetLatestBlockCallback callback) {
            auto response =
                zcash::mojom::BlockID::New(100000u, std::vector<uint8_t>());
            std::move(callback).Run(std::move(response));
          }));

  ON_CALL(*zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const std::string& chain_id, zcash::mojom::BlockIDPtr block_id,
              ZCashRpc::GetTreeStateCallback callback) {
            EXPECT_EQ(block_id->height, 100000u - kChainReorgBlockDelta);
            auto tree_state = zcash::mojom::TreeState::New(
                "main" /* network */,
                100000u - kChainReorgBlockDelta /* height */,
                "hexhexhex2" /* hash */, 123 /* time */, "" /* sapling tree */,
                "" /* orchard tree */);
            std::move(callback).Run(std::move(tree_state));
          }));

  {
    base::MockCallback<ZCashWalletService::MakeAccountShieldedCallback>
        make_account_shielded_callback;
    EXPECT_CALL(make_account_shielded_callback, Run(Eq(std::nullopt)));

    zcash_wallet_service_->MakeAccountShielded(
        account_id_1.Clone(), make_account_shielded_callback.Get());
    task_environment_.RunUntilIdle();
  }

  {
    base::MockCallback<ZCashWalletService::GetZCashAccountInfoCallback>
        get_zcash_account_info_callback;
    EXPECT_CALL(get_zcash_account_info_callback, Run(_))
        .WillOnce(
            ::testing::Invoke([&](mojom::ZCashAccountInfoPtr account_info) {
              EXPECT_EQ(mojom::ZCashAccountShieldBirthday::New(
                            100000u - kChainReorgBlockDelta, "hexhexhex2"),
                        account_info->account_shield_birthday);
            }));
    zcash_wallet_service_->GetZCashAccountInfo(
        account_id_1.Clone(), get_zcash_account_info_callback.Get());
    task_environment_.RunUntilIdle();
  }

  {
    base::MockCallback<ZCashWalletService::GetZCashAccountInfoCallback>
        get_zcash_account_info_callback;
    EXPECT_CALL(get_zcash_account_info_callback, Run(_))
        .WillOnce(
            ::testing::Invoke([&](mojom::ZCashAccountInfoPtr account_info) {
              EXPECT_TRUE(account_info->account_shield_birthday.is_null());
            }));
    zcash_wallet_service_->GetZCashAccountInfo(
        account_id_2.Clone(), get_zcash_account_info_callback.Get());
    task_environment_.RunUntilIdle();
  }
}

// Disabled on android due timeout failures
#if !BUILDFLAG(IS_ANDROID)

TEST_F(ZCashWalletServiceUnitTest, ShieldFunds_FailsOnNetworkError) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  keyring_service()->Reset();
  keyring_service()->RestoreWallet(
      "gate junior chunk maple cage select orange circle price air tortoise "
      "jelly art frequent fence middle ice moral wage toddler attitude sign "
      "lesson grain",
      kTestWalletPassword, false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(70972);
  GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 1, 0);
  ON_CALL(*zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  500000u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          }));
  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                ZCashRpc::GetLatestBlockCallback callback) {
            auto response =
                zcash::mojom::BlockID::New(100000u, std::vector<uint8_t>());
            std::move(callback).Run(std::move(response));
          }));
  ON_CALL(*zcash_rpc(), GetLatestTreeState(_, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                ZCashRpc::GetTreeStateCallback callback) {
            std::move(callback).Run(base::unexpected("error"));
          }));

  base::MockCallback<ZCashWalletService::ShieldAllFundsCallback>
      shield_funds_callback;
  EXPECT_CALL(shield_funds_callback, Run(_, _))
      .WillOnce([&](const std::optional<std::string>& result,
                    const std::optional<std::string>& error) {
        EXPECT_FALSE(result);
        EXPECT_TRUE(error);
      });
  zcash_wallet_service_->ShieldAllFunds(
      mojom::kZCashMainnet, account_id.Clone(), shield_funds_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&shield_funds_callback);
}

// Shield*Funds tests are disabled on Windows x86 due to timeout.
// See https://github.com/brave/brave-browser/issues/39698.
#if BUILDFLAG(IS_WIN) && defined(ARCH_CPU_X86)
#define MAYBE_ShieldAllFunds DISABLED_ShieldAllFunds
#define MAYBE_ShieldFunds DISABLED_ShieldFunds
#else
#define MAYBE_ShieldAllFunds ShieldAllFunds
#define MAYBE_ShieldFunds ShieldFunds
#endif

// https://3xpl.com/zcash/transaction/5e07d5b298f2862f2332effc833f5fe9157eb9ca00e03f394663e81b397515a9
TEST_F(ZCashWalletServiceUnitTest, MAYBE_ShieldFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  keyring_service()->Reset();
  keyring_service()->RestoreWallet(
      "gate junior chunk maple cage select orange circle price air tortoise "
      "jelly art frequent fence middle ice moral wage toddler attitude sign "
      "lesson grain",
      kTestWalletPassword, false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(10987);
  GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 1);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 1, 0);

  ON_CALL(*zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, const std::string& addr,
             uint64_t block_start, uint64_t block_end,
             ZCashRpc::IsKnownAddressCallback callback) {
            std::move(callback).Run(false);
          }));

  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                2625446u,
                *PrefixedHexStringToBytes("0x0000000001a01b5fd794e4b071443974c8"
                                          "35b3e0ff8f96bf3600e07afdbf89c5"));
            std::move(callback).Run(std::move(response));
          }));

  ON_CALL(*zcash_rpc(), GetLatestTreeState(_, _))
      .WillByDefault(::testing::Invoke([&](const std::string& chain_id,
                                           ZCashRpc::GetTreeStateCallback
                                               callback) {
        auto tree_state = zcash::mojom::TreeState::New(
            "main" /* network */, 2625446 /* height */,
            "0000000001a01b5fd794e4b071443974c835b3e0ff8f96bf3600e07afdbf89c5"
            /* hash */,
            1724753444 /* time */,
            "0173faae56e2d8d74d8f822f264f068d14e13294657280b772923b9e607999542c"
            "014f7fe5593d8884dcd084c0cd715ce7cedf15637b267629098b3c9c64e12de205"
            "1f00013f029c997cdd5f2aabbaeda87464452698ac288471a6da8a52fed40ad858"
            "b03200000001fea7d027088cd5229046540cee27d20ea8300ad5eb3c57bb365b03"
            "901a0ae127000161086dcb6c1c9e9b060a564cd1f68f17f29e49ea4865cb75eabb"
            "12eed9ed5c1f018a91c3eaeefeff1da395c51a7e0ea24b69acde116aaef9f8191f"
            "b27089f23c5e00015dd875137451b1d4af6a06cb3969c99b50d8df3378f4527205"
            "c3f29d48e556060001ae94e769c003efcb6891de2cde1c38b62006f4bfac132d42"
            "d6abc118fac8261100000001b254e3cfba937bcd40621e1d623a943440b0838925"
            "46f081aab37058fd1b763f000000013e2598f743726006b8de42476ed56a55a756"
            "29a7b82e430c4e7c101a69e9b02a011619f99023a69bb647eab2d2aa1a73c3673c"
            "74bb033c3c4930eacda19e6fd93b0000000160272b134ca494b602137d89e528c7"
            "51c06d3ef4a87a45f33af343c15060cc1e0000000000" /* sapling tree */,
            "01b8d435b0d87da1bb5de04c11c1d8601141dfe0904614ec3d59426d27cc3b7824"
            "01f0e38c4140cf2fe8fa6234d07569ec6310910b1dc7df92b88118b0f3a7bc333e"
            "1f0000000000016bc8acce0f0a243b6351b13f53e3ce19a4ec6e494104f506a754"
            "c3268a9a7f15012efa15d83c3fd6589d6d4a5bb8813d1c5f7ff333e62834dec08f"
            "e3558381d61601d05d08c51cbb09059afff5b0c7dc6b3236cbd742936b013d9ad7"
            "91a20998302300000112852a88a1d88b30c5a3bd2f532500e25379fc536934f2bb"
            "e26a2fcdea5dff250185edfe70c36f5d1c3216c4750bbd8f11fe1ab96802f9eaf2"
            "73f2e85ea0709c1d0001f427516d6600178ef99cc049193c413103980c407ea348"
            "9a687b3c3479d2300c00000112278dfeae9949f887b70ae81e084f8897a5054627"
            "acef3efd01c8b29793d522000160040850b766b126a2b4843fcdfdffa5d5cab3f5"
            "3bc860a3bef68958b5f066170001cc2dcaa338b312112db04b435a706d63244dd4"
            "35238f0aa1e9e1598d35470810012dcc4273c8a0ed2337ecf7879380a07e7d427c"
            "7f9d82e538002bd1442978402c01daf63debf5b40df902dae98dadc029f281474d"
            "190cddecef1b10653248a234150001e2bca6a8d987d668defba89dc082196a9226"
            "34ed88e065c669e526bb8815ee1b000000000000" /* orchard tree */);
        std::move(callback).Run(std::move(tree_state));
      }));

  ON_CALL(*zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1aVmWeikjJuR4yfuxQznCMAxPoxZggz82J") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aVmWeikjJuR4yfuxQznCMAxPoxZggz82J" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x6f31c69b7af52379871903fda4396e37117d9e1bbc94a972cda1c9"
                      "77742455fa") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b658ec0220a588bfafb3816eca"
                                            "a03baabd99c0a888ac") /*script*/,
                  200000u /* amount */, 2625431u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          }));

  std::optional<ZCashTransaction> created_transaction;
  base::MockCallback<ZCashWalletService::CreateTransactionCallback>
      create_transaction_callback;
  EXPECT_CALL(create_transaction_callback, Run(_))
      .WillOnce([&](base::expected<ZCashTransaction, std::string> tx) {
        created_transaction = tx.value();
      });

  OrchardMemo memo;
  memo.fill('a');
  zcash_wallet_service_->CreateShieldTransaction(
      mojom::kZCashMainnet, account_id.Clone(),
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzuap4"
      "3ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9ypkss5"
      "72ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xckh2j",
      100000, memo, create_transaction_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&create_transaction_callback);

  std::vector<uint8_t> captured_data;
  EXPECT_CALL(*zcash_rpc(), SendTransaction(_, _, _))
      .WillOnce([&](const std::string& chain_id, base::span<const uint8_t> data,
                    ZCashRpc::SendTransactionCallback callback) {
        captured_data = std::vector<uint8_t>(data.begin(), data.end());
        zcash::mojom::SendResponsePtr response =
            zcash::mojom::SendResponse::New();
        response->error_code = 0;
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<ZCashWalletService::SignAndPostTransactionCallback>
      sign_callback;

  zcash_wallet_service_->SignAndPostTransaction(
      mojom::kZCashMainnet, account_id.Clone(),
      std::move(created_transaction.value()), sign_callback.Get());
  task_environment_.RunUntilIdle();

  EXPECT_EQ(
      "0x050000800a27a726b4d0d6c2a60f2800ba0f2800016f31c69b7af52379871903fda439"
      "6e37117d9e1bbc94a972cda1c977742455fa000000006a47304402207bf92fd0ca8ad354"
      "d7afce8ec9be410597c9bced1cbc9687f51d5f0bd5ecbc7602202b5d068d6eb9753fe7a0"
      "c3f5562abb37fae30514500427d68b5044bee5ad9c940121021ea251ab4b5ad3eb78aaf7"
      "62329627e90a052fac82cf47e758e83ef96a2f1d37ffffffff01084c0100000000001976"
      "a914811b9ccf3596c62713c3fb339567bbae3305d77788ac0000026248c26fa7bfde6e40"
      "5b4e9e319af6f1e0eadfe20d220f184cf01614e851359f69b98b9598c13f6bfbca1a8c61"
      "9aad9a276d74394ffc44a2ff7b9a5510f8613a2194e434351d9ff067fa47ffbb0aa699d4"
      "30880a53c5bb9ef758ea46549e1d3e83e566a0a85fc0b3dbc78b4c099fd81ef13f4dd8b6"
      "7095b33305b178928fcf04c4b86378999e79ae6cb15eade4e99664840491190d0b34fc0e"
      "65b4b6c67c9d3acb3361b2bc5d49760ae76092a6dcc2ca0a299372c65e1c606480340a56"
      "5315a250c6842268c9507fb91ed1ca2823fff598eb4027a815aaa2bae0a7408ac225e5c7"
      "8a5f6acd7c91f483f45e255b2575b2cd3ff9f5f62ff46eeb77d4ea642a437cbeb8d98f78"
      "b667c524fe28c97be265c8c36a1bb183fadd0f911bbc9118ac9caa7e0cb788e622a5d71d"
      "732c4246bcecc73dfaf9109f29baee0bcc4871e39531fb19dd9019efb14b3c056b803927"
      "48008ab65e1c54fd2a3a9d89661acd0d1b6f5c410a1a6da0fddc210e33f54d35fb263768"
      "21cf4f4b9adb8ca14b29fca8b3b253071d2842b39f75f1f793c3a53185f80e89126d4c9a"
      "8f6a196ad3e230a40b6c47e28b821bcb5786224e89b445ddac5d69eaea7f487a42250a12"
      "86bda360ce702bbb2a0fb59f58eb5fd6d2e67000c915ccdb6c32212bfc533471f5838a03"
      "c4a7a53caabc02e78268659d402f398c3c3e05ce463500dd0c426047edb67067bd5073ec"
      "ca28f32546b49dd96a3c8273d316d4ef1d715c700d4eb47c98986fa36eff39d2d46819f5"
      "b90d82b4bebdb46f50801a4ab2a2253c77a4402ccd5eb2f7edf4e76069b676899025dbf6"
      "eb2a2c863708c525d777d38802493b476e8b5e551cd8d6aaaa59576c40bec65c566f24b2"
      "47cdaaf9764de61a97d0fb2e04a74bb25c20b5f12947d0381c85c2f227a1f8128036b1f3"
      "8efa7ef3a4844d2b25a8ba0fe08476a1d96372a3decfbf61e35ff6c561de819d0d498c4d"
      "300e58b36f44d364e522790c67f048f121611124247b757c45afbe58bc8d334beb1d2c57"
      "fb7601913f4899cf332a131f8e59164cc3f4c21ee6cb061c652b61f6cd637d78dfadf200"
      "fce7fe54ad48bb731b9181f5a1d0385d04447de6c9a5fcc7f24c4eccfde0ae320ed9f79a"
      "f944143fb348e2e1e90b44072019dc7fa195f96fea7a410dc6df0362c9adaaf6d09c3c23"
      "d422c593e1d055a0138f22d95eb23b02d1a53c8b46ac84843a8d0d843c06b7e86219083b"
      "201204184dca8df1bc3a17d2a019fa60e2df4a240e286b1ea7e66aa7f86a211a4bb8185d"
      "82a07dcfdb4e08fe4a15ebe6ccd3c51dce0778e49a05303b48e76eb055349caa73e74447"
      "12532be3aab8522b6629b2fba150ebf5a8ac067e3affc965763387436b092a9cdd1731cc"
      "f0def1aea2577c334485e5060ac7230028369772b806ae9a5a8c9f9065c98189b4855fa8"
      "c060bbffcfcaf0e8a31060cb04c3e613da1acd9d27e2eb136538bc4c739ee302ae6d3c01"
      "e67967daff17ff8b329c6d9d17bb107dedd9a83a3e517483c95c3b323102ae4a9289e77d"
      "f7dbe06ab09f2422d10e1ffcd968b921078b51971cc51e5e3b0fbae2c33afac4a1495c59"
      "bbfe753e8c8e62c0707f8a9df438ecb01b903b42138e20e96c2104742436e52b288efafd"
      "8fdeb6ea8612a1082e23071cc4af554975d10e28d546e0a5ebaa5cd4dae6278341104100"
      "34f4da8b234bdfff2f65f0a2937c77a8d192b7840f2d8b94994392edcbf3554c2226f187"
      "2dcedcb13c01c3ccb26a9c5ef633e6382d5465af9002c8ee11e076307766b6d44f06806d"
      "3c801d5db14289eb8bf218e011b1d084bbd60984fea5cf727467a70742dadbd959d32540"
      "8e9dece7dc2e0a9322e5734901a95ef986dd9e5b045389bb0e1b2aec715d260086475861"
      "2ad6dc1d5caa67380166f02dc7d1bc5880d6a5c3718a50df32caf31a8c17415d153e4a41"
      "5a1e8742e17dd1437985bddae1ab614aa476975a42e952e78c11a635df74ec13aba9bfc2"
      "7ec7169578d278fe960b7600bc3ddcb7970064c96e5812d2607bd354a1daaac4c03d7ad5"
      "47a7442483e06eabf2ca79749e264d82c75c9c02bd8c42c2502d9541152ec49b94f441a1"
      "35ba12e862b012dcbd965ba8c83304e93eb5ae6684b7d29f4d9b8899eaba3d4a8578070d"
      "061ac75fc1da2042de615ba5e3f13ff010bf73a7a08221bf653f106c5abf8237d52eb059"
      "86a41fa586aac8974d593254ca346ad8a641bb2f8b9ac97e4e85f35a9fdae4e993c53408"
      "00c16e18c950f52e5207c031d450568bd2b1ecac2a1c332fa3875b7842323c4d771fda85"
      "39ccb0cf8f0ab127ba70c0036079feffffffffff413bc8ac281817b1b753fda68003673e"
      "a30a75c3a1ae06b43d552c35b9bef338fd601cdf5a22664e766e5a7f051db1d1b6f859a9"
      "4910722d640bbdbceed7cc0b13db229bd7e75ff49ddf1c45e2d81d4e418e523017988664"
      "fb670444a4f879c4e0839b6e49f29c7a8e57d38832c93d68e1d10bc0a4f1b774faefb28d"
      "d93a66e41690b1853eea48b73481131a3ae70b11f900ec2f3608c30b86791b864e7648fb"
      "cd191024515a32975169589342f3d8a5e7d03d537b2c25e66e423457fa7103a4e430b48c"
      "87ce98c577a81ff1e8497aeb9cdce2e0b39f8efa2c2ac78b3aca30bc8d0714e01d325343"
      "09875a787d737c5b84fc6bd6ace3bac852c1e6e96684f7cf5438b9f6adee0ce7bc522b40"
      "2634b7be7d5c67b78987fe67c09593b665b08e9a876937e8c470d41e79541bffa0341db4"
      "89b69d48858301a9e70a10a7ec3c465796b4358a33dc06fdfd411514b322f4bc0783fcb7"
      "e5fd507446c75bcd26a5db9b019f9b8c86204a78b0f31fabf59718b3eb92ba0a1ca55f36"
      "3b72b16b829d9f87247a884fea979be29715f3fd6d8ac92778e4e7ded9e614734dec481d"
      "92bc8d4c5ce4136c29ec446df01640e7bc9b67bea01354e0560af5473866e274b9231531"
      "e99d8560bf83040f3dd109926bb85607581340b5d29a0134e86fe29cbad612f8a4c8a550"
      "ee46682bca200e964378d28f4d30328fd2d87a6367c7bc54994003bbd1b6b98c0be3c9ac"
      "f9d953c10882e2595f71c4f7410b0b72a546e0e225f5bca81080a935f069679fefbbdd6f"
      "97983ce3232506c8681f25bb8e69b8d02991c5b6314c05db5910e470006b439c2f4cd432"
      "3b3555e905647d9221ba35ac2ad447a3a11532a79490d08afc665e4d49ba7dbc82043ff1"
      "fcd2c6a1815be2541a166f3209a89fb478d59078043f4ed0941fd425725462f025dd9b6b"
      "616a65fde0ffc1ea9984148d2ba4f60df62b101092af91d1b1c9322118ec9a563160d9b2"
      "f9aa5c05ae633c1487531b12dd7e3558538c7d7851cae4249b3dad00469d313b0c864495"
      "48a02ae53dad0cddf1a745deec603a146433026d0127b1d8c08e6c798475011526543297"
      "3bcde9975543f7bded7525b3cae625b3e89128371b7c89db8aa339b267aa228dc28192f5"
      "d388ba95f034a4338eb13f63ad79f5ff202d1f7cf86bb4e04e18067f297c4558704498a2"
      "115b5ac6e20823302c89cde9caaaed87ddfc38534e04b3d86d3ee2925c6b3e2d3e1e2041"
      "4935f6a04251faaf95d21a45c126b4a1cfcb23823cb016eb8ea0c694e2e0ff6222fb6d75"
      "e7393060fdcd5185c468b2c091e5394b2cc3ef69863d635a6a706fd755a1ffd087b18114"
      "5afbb60312025a5b9229bbfcae63e5c5ef5a3c14fc9b89db9464276247aebcbb16f034aa"
      "5d927b524524b3f92ae4f7331a7506962cbbf23cdff18e0927f7ea441469b777595cdb4e"
      "d382a3951e4a807de1d52a544c8ed4473cd823dbb3e408611f2ec52e2044f4dd29f40ca0"
      "1dc92c5e738c5d559ac68db73d791323017bc7f7bf1c891a26c19b94ba1234aa20c62a23"
      "d22239ac3f78bfb8e7c8c7da24a0ef61357c4eca48d5c444720825f2fcb823e1bad94d2f"
      "407a83e665217b5ffc4ccdd53119a35d692444f06b6a37fdcb4008d61df849171df3f011"
      "7ef96e8ab3884630288f525a14df0c49f20cb7cf77cfbbe40a1b49130990451db770d9f5"
      "b9331e499a4dd7f6e6ee0703facb9d882d093877fe879ca434abaf10e578b1123b718bf8"
      "e5b099d71442f5c962da8f1198e5c3c4dcb99d715d6d7380194d58ad2ccf80c410899cc6"
      "83917cd6ccf925abf03fc3dffecb4dd7ed511271e21d47d5ae18f970ef1783b45ebb1710"
      "cd15a927db34cf07919f52dd28cf8776d91e2acafa98f0f6486e31825e88e87f6fe71d7d"
      "bf26522f8da3278e69abeeffc8bd3202719004649e38bff962fcef60c664ba0e8f55cbd4"
      "c9a2c66cb23b752df0139d7dc840332204685dc2d858783bcd7cb3ec9620416d3a2511ed"
      "556fd17683ee8ad926ab25f8cba8b7e9406f840767fb097b9180ed61fd51f260488f42bb"
      "413f7920bfaa6566b19bb55b4af106d99db908a44a9bec8f5b77ca6ea5fe7a4a12a7bd97"
      "25d40964181c5df248e779b332283c904195a115bf174d1743d967c906aaaffd54ab057f"
      "93181f2c297734dd07ad364e47ad685c616189d09f214ec876254cc37e31ad9b0bcadabd"
      "4a71d9aa70fb38b541eac22ef38e8a2b0bdc1dcbb40f8e3dd0067ccb77bcb7550018005c"
      "3a6b2c0ae605b0c77100aa937c12640abc25c00c54bfa9820a72631e54679e9deda43cf4"
      "1ceaefb00473ee51f3a2517c6e890109b67b51cd1fa9029680047cd47ee3124ecde66d41"
      "268e0077d50db5d17ddfc40d17361ef8a058eb0a9c4a97326a399c5ef043bd0a4f9a4d7a"
      "b61bea81c5af2b55ffcce33cf46cc40724d17f5e2a9986495fabf3d13a8db527d249c88a"
      "6de51dbf9894434b2a06663450e1555214260bd67cd1d80e51c7e1a239f72f2d012663bc"
      "273e957cca3a0eb842e4cbe3b7722f3ee9a85ddb02ae206a99c8da59cbfe35a6d62576ff"
      "c155c9f01b96f781a3090b0b181e2ca17edc826a6b386d90beb3ca66b2a28fd0161fa469"
      "55496c8aca9313a55174a278a41b122a396d1b0d68dd854b9c7620f0e5c2ce9e72d33d1e"
      "1eaf07bc6b4a0abe41483d859afba775091f20a064c23b6c85d66540c50db4a04e7117cd"
      "4429f419d6d1bdd25e897d82ee57a6a32941635dcf344938a0483700ac2a326fbf45b73a"
      "e8547039351e2bd1d0d1d21e2213c85d66b8aeb68bdcca16beb51caecc79e8650e3353c1"
      "60470e1be063966b279a57f15fd9eab27918bf7df6a1381aa25367f0c96143a18e339dbe"
      "96fb6eafc686c52c24115d19892df073c0871fc91bd61a198160e082e43188d50e4d1ad2"
      "b81ab186c1ff5134b798f6dd61950105648273370f6e15bfaf646d3a097d99f9929f2a63"
      "2407102596bc7664ada133d303cc45c451717ec94953fcf21751a9d79a02374cf2c81c34"
      "d96eb1d44ba112a8d2bb5222ba5e6d9f6a33f7aa6a48ec95f6c8a4a629c2628a577119be"
      "1443010ce0a8f83d97c973b2300dde50c120b1dff07ab40e2ce5dd05b04b4271ea922df3"
      "50c23ff6b0aa8c384d7718c53410ed012150192ca75ec1ba817afc0ceb271d9ae6cc5c1d"
      "a1f1cc04ad6e500ef266e9a4b24d6b5853d9940870711a271bbf2390450fe45c91b1915e"
      "63ae5d1a75fa47d446f1b6914abf7b2e8c8d53b84dab3d11bbb416776b68043574241d9b"
      "78907fa82aa4fdd346ca562fb975013625570149aaaa3a7744b957f100ab21f1937b4419"
      "ead8b79c2d9ac7ae472151a1c6352608e48f970bf5e1bd2740d3911162b7361ef7a806e7"
      "16522092347092d7f9981b2a9b11ff8a4079a37a79d8af1bf89451eb9c487aeb30d7e480"
      "44165e3babf1329dd40933c8e9194f4a0df255c48e3f2a53de7e977567b3075a488444a4"
      "8f3e2387d82f974663c25151a534af46ad165b8d326c1de9ea2d5f8486eb44bf32861e89"
      "df6c25a369a30f10e1cc4fce45ceb485e226d293c9dfa0fa4adf07282064200b305d7e53"
      "ad5a2445cdcee87d2c00e00bfb8359ee6e34bd02a4fd13c0bbe702e4ebbd9436a233aa2c"
      "0c496490ee11eaf3fa7532e9ec3b24cff9f95654931a1ee011f332685212f3a9ba8119eb"
      "815795c788722d31e7b96c3dcda18702dff0377fcf94c246a769fba320a13598a99ba793"
      "c81ee9375b4870783aa252198b1e39d099a215284183a9a8dba6f5a9a939841e08984c3a"
      "08f330b5fc923bc38cd4359508fd7dde89a0be6dba5adc2d368d747264ccc27d5d708f09"
      "73eedd6744d33ca54e335e722b26f27a93fa317d9da05811864f2193b3d6b9c8821b5b27"
      "139b0bbb36c60020091b251879096b13e88509ee3a065dff1e741cbdf17af7ffafc227d1"
      "5f634e6ffc451a99325334fa0f95d3a248db8d77c60e697f68d0a546493b09411600de57"
      "579b7fd09923e14d0f5a9633ee53e6ba2d8811f2f8bde0a66e8823c5c9c8dd2b2788bfcf"
      "bc3758b33ea19c23f48fa77a26283d08c12c2355dd011183d1fb4d8ce36cd7c1695717d5"
      "37b41da1a195ec0e920361fbf864001dd38e068130adb79529ff64b799ffbfb6c43f63f5"
      "add1f81de396e97d8635380336b528f3e44854b78a658578a5fa75fd795c3d3e3f6d805b"
      "b837e425a8e1f45a1d0c137ad3fb7ed51f1c43a4c0b288c2435ada4b95a87e1f791c6a19"
      "389b861c9f1b3227a317be1f4da5548fbf9b2667aa86deec014e82f04ff7a87d94c1d156"
      "2f17159bc98e1d595fe807dc1d2a2c0d61844ca91e775e570f2bfd458f6796ae56911903"
      "ae4c5635beddda4b96551d87127936fd7ff70da7d35c5825bedb0475c7400500513fbe81"
      "964fc874769c6f4fb6d0403c1971f095df8ded365f78fe887bc91069eb917792291948fa"
      "0bba7e9b5ef3b42803e614289119dc04e24ecbb718b9158e9829b7f0049a20c52969bd37"
      "aada8c5ebd3188b00c3529c703780cf308900d534612c09a94ba051f844b26dfc5eb03b6"
      "21c5c08a10e9db97ae1f79560db33e925e6a409d30f6f76e26b1e4169c898281e400e432"
      "a5ecbc2cfe9960bba7cb20aed7ff9c1bad1371cc9ee2fafc7d861a1956e512f911c7673e"
      "3ace4607b4ed2bddb7c4d58b81135bb8d9264b0d8a6e119d414a55a081cd5647482e5b59"
      "27df2151fccdbcb7c9b7f4004735396b3a93011d8f18aa5b610fed63fb18189efcda0b59"
      "e6436835882a1797dcc2a3cfe42795a6b87951415380bdf5b21a651c2553069d5eb5d99c"
      "3472cacb2c611b28f40fd8cc607069a51aff6e5b134e288aee60334cb56f363df66ae944"
      "a69f33c900d5f6063c090d27121ced9c212411b887193ba9c0cf65bc52517b7c5b5844b9"
      "ea185b2ed0d175e67e3946e47e7a454bc0b52f067bf948bd3a53c9723c3d524f8983360c"
      "79a596befbb5af9d094ac4f20e1c3819f3780e9db9e16a9350c1df9dc40d53517cfadc94"
      "057337b9c2203024179333faf8ee33c6eae5428e3a3b68d8409e6fadd893a397a2f8eac5"
      "e2f68c9310ef23c3274a44478912f0b004b1b595f405898a68fbb62a18f874c3e68d4852"
      "1e7511928e373f8da06ff81f5234da909778e60da91340b255460350e41c9778147a1cfd"
      "dfbab40ce5c34f221a1c00881d554dbc464e9793a72110314fc551ec41852f9c7f6fd1ee"
      "362ac43943b79cd563c0c493786446bd60e196d2aed25ed6ac0c1ce5caa69d374d8ae46a"
      "e04cc4bad19bf4ce4c517e25a001267a9d5fab152dc91146d1980cfc6c0cb4bd9131262b"
      "d58edd38a2e747cd236322a403ef69b94efa1dee7c048cfdf4de11e52b86a1a916479173"
      "2ac1bff6336e41189dc94e77c5d40c23f13691e4dbd08445a68233757095598fdbce8df8"
      "995b1eafd34d34afc0de0e9d0898b3a090a50e9ca490ab36dd0afb243a3b1cf877eed1e9"
      "5ca244df8b4400d52f6bb1ecaee7c6a871322bd3b73859cf8c7a14e0f59a27285e8cabc2"
      "d50f028b880bb25d5ca64d69b23ec8e065fb36405dade25830491f298c7a1592bb430296"
      "37021abf0f58be80ba8ff6d8ab2581134da3880e00441a237f92e9922d1a0206f889b22f"
      "670859fee7e6ccd1d3bfc8d5bb331a200cad6e82e910d2d88278160ad4f094a59957b9ad"
      "9ef810a1f31d0a37b9a6c09f6f91a1ddc41e747f9ded39c28261acfbb81ff3ab56749ea6"
      "8d1b6012f5a260221a98ef60db9efabf911b0b377e0d6de6b5b9c57709e2973422f55ffa"
      "0a6ef17916f8643063e750867df11970605c1f92dcc25697c2f2969a6268b1e12e748826"
      "a0506601a689316860a63414b3287a56374fbe04c4e8736325dc5cd997565e8d7943fb0f"
      "08b6593febc31a0774f1b09e444acd3d2549e24d7f2aff59f8d96e353b4c538aa10ceb54"
      "8dd82b144de1ebd6c4f96fdeb2effbcddbf3e969fdc776ec711ffbfd893f73558c1f3296"
      "d69252a4e2ababa6297e7e645ab2e1845d3014d4df35fb8fe7a84f59bb0b15d6d7443aff"
      "eed9dda55e3ead9501cae34bfb67fc246a4ce04085d52775a849110310f6fe5022fe01a4"
      "40d3f1567a61004f9263f832cfc89deb37d8a8c5758709ffb6d2cd266b2a78a11231e152"
      "9b47c0c12a9f9aeb8d473a7b49b88089ed853d91b996819a3eafc374b25a043714e1f9d5"
      "f6f2c73f8ac38b8ca2724057364705fbf478e26623793a2d0c340c9e6e354e52cbfa03a7"
      "10153b338ea79fe7852d153419ba191a6a6423ee7dd783f602e56d70abd14d6f043c174a"
      "7f0303809e9d30bd1fa42b386aa6489b7166264d354458f6c4a5a5ff52bc1833f1d09f47"
      "4a09308cabf5b69734eb08610c889a4626f435c766064c799ea4152690341e061dad06c5"
      "09ad6d182d237e6eebdf03a600ba3dfd8c849e7266a88669707cba8fa1a428f02f1c6795"
      "e2cd53ae0f8e2a2ce09c45e86283c2bddcb12bb3988eb48967e7229751b40ee1546c1bbc"
      "9b908da9080ac1b0ba8e7e7c41a177e1bf034e1c3426232e3d0325c20a0e813a8a978284"
      "9408ac0632b86facd327d5a6f14059707f3317cfdbfc045cde2f04bbb3ba3ea9c783123e"
      "8e27b124574b1a509f3b2904cfaf35a6b763ca79ca0493b37e1a57112077622515c7fc05"
      "2566299037dcc3842c7432f20fa5106ff90254d1f246be1d5943fb295bc655f7d8277cfe"
      "048cc2d18ede08a957d49066ff060d19bd0f27f45986ba2f042c68d92341dac971929dfe"
      "5f592d160ef72771585f357a42839bcf8e39a22e6b9754705c6c1cd1f1430a61267906c6"
      "e6321057d12eb5510b393c86cd76810639e97989ff749eef59969f490f540ae642aaa5b9"
      "d7c0a45b15eacfb79319105ced5bf5c3a42e0f30849d9057d73a361f6f685f99a7bbeee6"
      "9568a18ab5002bd09b7e03914d2062d1a91a9ff802a42aa1657bdfe49653c0b4af9b947a"
      "a5d706479aeed07aad2a1d52e697be988ef61ad53798aeb20be62f9af713ee27230af263"
      "6581e67988d3bb1461966e4299d81a1e7c2298e459610796a195d686561084d2320d4731"
      "910d8a7c6ef45b8097ad391b0731486315b1128548dbb1a1e9cd947830bb0bd135fb1d89"
      "cf27a2ac604f33c9f02f08da6f41a7a943ddb28af6942b6e2f49845f55011a55f87dfec0"
      "ab750e0ae348b9302137ee274607da7a917b4c304ac06920e931229acb07d03aebd91b32"
      "223430f1e006cf4723044c9d14e8c6f4ba49abfa2f50231f6e93a66fa5401ec74dc7691f"
      "f5455959d3f2cbbbca43382c64ea0d332e45bca951b663c1df5b3f3008f50e9c68967c90"
      "009f2b9138297bc208ad16dc1bbf35aa1f047b3e3b8b2d76ee530465cc0274dcd7c3c364"
      "94a77b58cc7c443ae651343ce2e96f44d7e1316fea36300886a974b3fdf892e2c507dd40"
      "d5c60752efd24e4c83daf5fa685114026212d9631314480f015bec9f64c1a5d6a0a59862"
      "f5628d57b40340fd1f5433ae7e6db929c32426253715bfc5cc4efd3274a6da2f35d431ae"
      "7fcbce3949c8148b87f14248a9f95b67e24bcdf923a0d6a9ebba1f5610b848e0a02eee19"
      "23302c3489994211465d9cffc38dff8f6b996b18c102217df405592497663ed2a1a92746"
      "4fc5f56a12aa04a425dea0120f0c7675f160be1a333d77696d8e13a266120fa462e2b9c9"
      "cc3025d85aa8909690fb617f3e7ae88be3e679fc2696d6e669a11946bd8632fca5784fae"
      "6cc955a7e87962a2f606fb41eb468c3decd4a4691c5d01da1fef9f0c43c58bf9e8617d1e"
      "23459e860b95a906a6357404af6f20e1a1c81471cc2ce89d2fe9943293d7e5aff9a3a556"
      "42814e24f5c876c7a6c4a39ae9a12afd1cffe507f52bce887c559cc5bdb497848802098b"
      "7317826ddeddff0af802040eb5cf0e9e1717c416737a0a520c5125b565b163046a71903d"
      "d6560261038805d58702072b12ed88b61dfdca71073637096bbae2260a7a9872a56a890b"
      "2f303a6fef64000950a120cc3752ce8c4b3c94f71cfc837c8a8c033624c05a812d1a1ed9"
      "6ddf4156f1fa4e9a7ffb5b83787b37281a0535b37dd59cc1637101cd3d48298af75d3019"
      "db273180ce66f81ade02e16b9cce55350b41e0c11b7c6e714eb12ce994a2dac0bad8f412"
      "3b3f9eca16a2e7cfc46264eb32bdc8329108776e55da1c75aeb9fbd302cb9e5c54afa69c"
      "8d20f977450be88186cb65a0c4afc43af3042ad5b6779f13875f0f78d60a6fdb0cfcd03a"
      "5c37673214b491581aca785a62f931c39400c28e319f066e65f46b5eef0f88265000ebf9"
      "2888610e0d4c6086e02934057f2eec47967b1a9ea3dc1cea2da9e63977e55bb81b845045"
      "7d3f39bdef2520db9f77549df5a2aa79b415f78aec8712fd10c5b0b11827c0a3ab590afe"
      "1e9516de06e8cef4ea14653db41a9e569eb9a3c4a6d2e672e93adcd66d9df6287d412d64"
      "b9ff5e0fc92b85228b6d20ddb6a3bda264c79921eb8b1892ccc5e0f3831b3a95b521cefb"
      "425abfa363dc65eba8a9c16c9de928e0df78241e759e192ea7401a59bafbc7eb0c49802e"
      "319af6b7b509ddbabae561df66c93eae9e5e588db4a51e3f98da4cd1d3aa24b456e77ceb"
      "91c5027eafe8ca1c2f6cfce8eeeadc8ee8eb330975197c2149e11994fa0fe1edb2c58313"
      "b3776f1013db380fc8c86d5d39370a5eb6154859979f9151adc010eff1dae2ea48c94eba"
      "1e5d0eeae9a4a781b9da2c50652a29039c8476f9b483d51addf71962c915d13cba6bd4c0"
      "e93bdf57a500155bd2f08b557a4d6c1b532233b4ec6be0d4b3da901fdf9ff82cccb57b7a"
      "c3211170567358fa5b8e0bce8542b4a427f484d27f8ec899dc2759daee56c4d67b8f3d9c"
      "f65829564d2215f8cfa1bfa4c45f21df56096b0d77f4c5f27c9116af7f7036807bd2477d"
      "48af6cfa27d88bf7ca8a9499ab81c2b60c8de3cfb75ba4b2e91a11ecceb04b0997cffa6e"
      "dd49dabff0a88035f5fffd30dd66b26f51f93cca7f668d2cc0cff04ae2f270a43f2dcca3"
      "c84c8d34bff78717db4b34cdd99b4e1cb3ed2e951d16478883aa375305dddd7703962d03"
      "ef145a55fb1dbc99572f41d1ed4a2339e4cbb9073a54850a9aadb668d7272f69ee29f27d"
      "d879441f9e7f328e45913786f5de74037c0dfb6bff91b0db619e91f3f8e9bbe79eb07a8a"
      "eb30f36a806b1d9be5e933feb5fdfac1af4a217dde7ebe9064aab95c411f060d133a3b49"
      "9e613f1c1d68a28b9fd423b09feade89498037e85c4d04f3a04079fe70b0a4fb23512cc8"
      "0d15a6752941b7f61431e993ce0fad96539f281aca549df191be6f1559eb2c99969f88e5"
      "e3b0ea15b42d10206d10fda513828ebdec3786f17db5bd91e8990f3e9bc8b7d072dc31c9"
      "40ca90543af02af245ad47fde53bab4cae526c224bedbf583a24dccd5e0d07542342572f"
      "dbb94a7c714fe505b9754b5866ecb3249cd9245367dd786a3e7c847790d02026480b08fb"
      "256b735e41f4481bfe47450fa30580b33375e22d2a55663ce85ff973ce09ec81b7b4cad0"
      "4f6beb385430e45e174c1e2b947f332c4c60ae149d6177f0e06499021a76d73eb57e8b81"
      "44fc2f2d64a3b87fa680e55a2056c03ded367bdb986781e8adb50ed9889a053f8d1ea97e"
      "454b32c1470e1e1015104879300aec3a229b37af94c54704af6d4d9a2cd37208faf5204b"
      "a39199590e29ad9f641584da7d652a2652b0f61a0eea3f50727a5227a65dae53f7b52be3"
      "27b88754b9e41e60c533949c21ee453559dab08ac6f689b34720b364c1143097d95668d4"
      "55b1373904b24cb34300f6950d8228e6bf8d41472acf2de3b0977a8a70622e2de4351922"
      "315efdb64405d3208374391e9ab77800db70800aa1a81116aad3abd2a64fd5889ac9ec48"
      "ac84a3ab1bcf330fed261bb713d50e5fadb0348645adf79558ebb025fdff0bd50f67bc99"
      "e36228833e0c90f48266a339b34c290e993e9214a20b55529fc3600394d2920e60210b87"
      "ed8512f2379bb4a74e2a477c1f6aea74ed6dadf9734d4a17c26b62f7b1eb766abac9b0bd"
      "a9dc9532d46fa98d46219cec1844656aff10d610eff055b2842c2a9b8e1880e0dcf58f1d"
      "2f70e472bee1fa4b0c21a556142060a3f3353db45118eaa5751a8f491b849b3a5c82de05"
      "aabaec2cc00de57b2f4691ba85261d398c60380ae34aa5a8a21d2771bdf04260ba181d06"
      "57f3335f2c6904434836c6f25b7d9c79f6c9b5fa5174b4c05fdd8ed57954d6701512a4e6"
      "74bc105ac0eac78c53f4ea26ff9840e680e18371cbcb76ae1354a069a0b6ad7c7daac354"
      "3d71f56309e53b5b15e600732ddc0aeaa4b4523338ab3983c3f428ee686282211bbf76f0"
      "98541194e4d6d212ee7126066a7d150878e17fc8cf0a0b9fd84edc304012488b8ed383cc"
      "164ecac1ff6912fab90fdf8028caea74348b00a31a1b763001ecae5bb343cd0e4afdc13c"
      "66f734dbd92198f42ab415db8f2170a4b2a3204d922ace5483ff835f14a875737cff842a"
      "f00b4244d366481a1280633be94d98646aa072c5359c357b4da8e203a51317ce69e0063e"
      "e47cffceaff0eb60627a401d655263a1115dd90e4a47ac05ebd038dcf1d8fae48fdb98ee"
      "d2192005fa3fbde30094fb855c3ef6998fcdc7b9dcc205",
      ToHex(captured_data));
}

// https://zcashblockexplorer.com/transactions/9956437828356014ae531b71f6a0337bb7980abf5e4d3d572a117a3f97db8a15/raw
TEST_F(ZCashWalletServiceUnitTest, MAYBE_ShieldAllFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  keyring_service()->Reset();
  keyring_service()->RestoreWallet(
      "gate junior chunk maple cage select orange circle price air tortoise "
      "jelly art frequent fence middle ice moral wage toddler attitude sign "
      "lesson grain",
      kTestWalletPassword, false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(70972);
  GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 1, 0);

  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                2468414u,
                *PrefixedHexStringToBytes("0x0000000000b9f12d757cf10d5164c8eb2d"
                                          "ceb79efbebd15939ac0c2ef69857c5"));
            std::move(callback).Run(std::move(response));
          }));

  ON_CALL(*zcash_rpc(), GetLatestTreeState(_, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                ZCashRpc::GetTreeStateCallback callback) {
            auto tree_state = zcash::mojom::TreeState::New(
                "main" /* network */, 2468414 /* height */,
                "0000000000b9f12d757cf10d5164c8eb2dceb79efbebd15939ac0c2ef69857"
                "c5" /* hash */,
                1712914512 /* time */,
                "01740ed9378b958778ac2d8a128e36848f67903994a4cf565b425a53e5affd"
                "871501473fede979cebe2b429991465790546bef4901f61e778d37d61221e3"
                "2586105d1a0001cd1db7e5bc10f29fb18c9e55a9e184d766134e87ac4968fb"
                "b8b844fc0671894a000001ec648eb9058abba1de4682536919524da9ad66ba"
                "ee597381975ad457e0f7f76b0001dfab397aca94c3d3f406ac25b4d37f29dd"
                "82f4fa4fd68b6312cbdef65c330e090000000001f5b1b613f54794f437ed0a"
                "2b77e4fe97ecefb18c2476bf5d3ec87f3c94903d2901067187080be7f0b727"
                "3e4331b7505d7c985974c9c461fa602d80cc999289432501e3644c42c5d7ec"
                "d832a2c662dcd397824930ebe37b37bc61ec5af84f724c2571000132c52534"
                "3fc4ebe79ab6515e9d9fceb916d920394bad5926a1afe7f46badef420001d1"
                "b36bbba8e6e1be8f09baf2b829bafc4ccd89ad25fb730d2b8a995b60fc3a67"
                "01d8ccea507421a590ed38116b834189cdc22421b4764179fa4e364803dfa6"
                "6d56018cf6f5034a55fed59d1d832620916a43c756d2379e50ef9440fc4c3e"
                "7a29aa2300011619f99023a69bb647eab2d2aa1a73c3673c74bb033c3c4930"
                "eacda19e6fd93b0000000160272b134ca494b602137d89e528c751c06d3ef4"
                "a87a45f33af343c15060cc1e" /* sapling tree */,
                "01f53cd6d046829fa6bde0d6517f00cb395f6276d29f54d6ee8cc03a2589f3"
                "ad0a01a021c60843a6a4f385a94d8d595ba1135fd2db406e451066fd361907"
                "cf13a7191f0161e3b206d91efbeef0e45e83127ff401976364df189a2a1762"
                "cc46b61ce0c61300000138ae9fd5b9da68da1b63af95d4c32863d02729960e"
                "70c6eb7cc6144ee994ec0301f8e1b87f0a823799d6e516dfdb7e1dd44e2696"
                "cc82ee6c16a73b22104604ef36000001288222e46d9892f23a2e2d6fbf6e93"
                "31f6aa139f2ca3e7e0e7e1fc83c7778708016c33eca8e2b92ca46f0cdfd799"
                "5778fd0eb625df98f488ab1d1a90fa8825b43f00010d2fb3b0880ce34e2643"
                "18a8faeace5e051b47afcbcba1edfab81aa66bd7ff2f00000001be7e467513"
                "4c4441539879962acf4c9ce2523471c82f11a2bfe90d910e5ac11901d38650"
                "8c9fabdc60836bfe3c7251fcbdd4617180a804d40fa29dc25fb9c0aa3401cf"
                "3bf92f69798e68555548afcce1648add1fb2548d64fa9a1ec22a3e26e78901"
                "01e637281deb58dff0c44ba13149b784a95da1b493005efd057e6f4ac20ef5"
                "d81d000001cc2dcaa338b312112db04b435a706d63244dd435238f0aa1e9e1"
                "598d35470810012dcc4273c8a0ed2337ecf7879380a07e7d427c7f9d82e538"
                "002bd1442978402c01daf63debf5b40df902dae98dadc029f281474d190cdd"
                "ecef1b10653248a234150001e2bca6a8d987d668defba89dc082196a922634"
                "ed88e065c669e526bb8815ee1b000000000000" /* orchard tree */);
            std::move(callback).Run(std::move(tree_state));
          }));

  ON_CALL(*zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  500000u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          }));

  std::vector<uint8_t> captured_data;
  EXPECT_CALL(*zcash_rpc(), SendTransaction(_, _, _))
      .WillOnce([&](const std::string& chain_id, base::span<const uint8_t> data,
                    ZCashRpc::SendTransactionCallback callback) {
        captured_data = std::vector<uint8_t>(data.begin(), data.end());
        zcash::mojom::SendResponsePtr response =
            zcash::mojom::SendResponse::New();
        response->error_code = 0;
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<ZCashWalletService::ShieldAllFundsCallback>
      shield_funds_callback;
  EXPECT_CALL(shield_funds_callback, Run(_, _));

  zcash_wallet_service_->ShieldAllFunds(
      mojom::kZCashMainnet, account_id.Clone(), shield_funds_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&shield_funds_callback);

  EXPECT_EQ(
      "0x050000800a27a726b4d0d6c23eaa250052aa2500011b7a7109cec77ae38e57f4f0ec53"
      "a4046b08361abb92c62d9567ace684f633ab000000006a47304402200abf02ccf7a5a6b3"
      "d8496e0546844ebd033a2f6099139c9a5bce0c27aea21932022050b713ad76062ff97ca9"
      "ea9d6b2e3a236077a013b597a8ffc7beb20e476a3bf0012103e1dc8e28b0318ef94cadbf"
      "a0c9781ff69a09c1424c0b5ec3333132b1fe3aae8bffffffff00000002bcbc1c4f50f320"
      "e7ded91927aaca380ad95d1c1e0aaa9be6ea362cf435f34d87da3403adcf62535c29608e"
      "673e0bb1a6be7992f08af8a2c35e6f7cefd7067f068812f9f058fa57ffc1c0910e6a19ad"
      "0a488a4eeec8a19d78f9910eac9e44b9acb0b90dd58b6427ac434ad73d2484bc25213f1c"
      "6b7251bac7d8152f63412a9d26f575246c6ab3195b8f96736ecabc45eee31c414f26f4d0"
      "178d666a7064532809a0db6c0f5e2286bd9e4c80f67d277ef9507b193634884bddc4e983"
      "08b54b2485ac2b0204dc3071c5ce33c4de04bf323cdef4a7d3ca46c8ba3d3f1d943e5363"
      "49d4104f1be5ac10e4bdb98bb3c9f87ad75489f25da171e64b8bf0829e6d15bec11a3191"
      "6c80723483552f77bf6b2d2562cbc306e67c30fb5c39e22c8636b4c62c84213426bbc233"
      "71a9461a98a7c6ea52913319056df7e050f4f99133620dbd159d070fd514bddebfe75b0c"
      "4b1c441e227bccc77be84c284850296568a2096f3661fbd7d38da9e3ebfee74b272ad1c5"
      "4a55e85ef323d7775cfbf84a532100395623a7bc6a0c9d82c9363d9d6b9e0efa33b04a3d"
      "1991f13edd4f29187075540549171a26b8e745dfe1cd0291a59617b06eb5e6d6612845e1"
      "9328a825aa7506dd7bc2ecc93a3d2dcd24b258fd372dc35b00ffe7925b5734267d532bda"
      "8dc0feac21023c2813a25e7aa301df3168e421b1b27db4fe97f54eab444707e346dc8107"
      "228ad218892da1dc28bc31046636d0d69e9310e0e7c1262051facf580c34bb2ff07029bc"
      "3d11f20f5bb76f12d5bab32772cb334811f7d1c64d9a4cba13cdbc12203597edba8f5a13"
      "8c54ba58dc27bd846453534b16906c67a49438ccf3504a007b90ab149e645ceb831472cf"
      "cc08b704f9f596039725c4021b8fed8e0a2a0b858f0e3d6e3ed58ab70dd7527ebc9aec93"
      "f36e3df6bd29fb69ff23d4857db3ab92c1ae80f81318c457a8b3d8eecb9f5c1d7154f590"
      "d4d2812f3029d594f1a452b5b8eedfa590169a9f0fcd572fb0936d5fadae183fffe470ff"
      "e7f6ff60e8c003db15cdcdd6f03b959e6004928284f41c31a0282eb7d186d03ea9f26482"
      "41012ee034d71b8bba3c4a5a4bdfe494feb4dabeed06b9823ab0e4ea61acf7026383baa9"
      "0ab7b6e56fc8323c3060a4e5c23d990c8bc0fc18a75dcd16aec6bfe05662a2f659d710ee"
      "aff78a16d44abbc27c287d8d2dadb5f29c206b982f9e02bfd34c275b3bd34c8260181c5f"
      "b97083d5b8b84cdc34ee3fd33be74c3b8d1caea377878b62630b4f4bd52ed2d32f397a45"
      "5b7ead654f234cc9af8dd531cf2b2f4076eaddacf1cf1e45fa98cdc5bc38c1e6fd8ace97"
      "9f354aab32423786051373c83b6d3c14e1d29c7345a04884fd4c83a8be7880c737a2ca13"
      "1b64f1483c435118e0e5c1fe33a1dfc8fc1fe01abbd10ab9713008cdc393fcb7a4dd0a01"
      "406fa8c0dca05f7ccb345a4dfac1f935d10fc7cc93e79e1bdca161efe66c95b57b621ebb"
      "fe89d4e8c9d45af606d0c0e350e699c719dcb6560f18d085b76c9f19ddada83f905bd554"
      "9737f196c447db36a270551a0a1408771c50cc37baab706ebdc1af83b09be320292256b3"
      "99e1ca72e140b51658b177994aa8dac731e89ce5dd25d70eda9767785dd81a998a583207"
      "8788b1cb1602bb4196b954bee804abacb4fbbe83babca9dea818125711e9871e1c3c179c"
      "1c7c653a639f36d3a6bc3c91f54a70fad9a1f68860601c8700ddda8f4e1f1ca4ef77dcc1"
      "04a34c2ba32152f9966a87f615e7f6dba9b799ed0afaf5bc6576a854976e7b34f2798ff3"
      "0599e15ed753ee5ddf3b35518da09719e1f1a143593f7b158a55f113b1ed657a6ba59e4e"
      "f9a6b2c536b107bd278deac4f6246d4c64a1b429ba78c2643eeea586057e557544e5a1a9"
      "17bcc1f9535f6d3ffef6bfe3fff51a97933eab9e1902a2383c8ccdc6a87b7bfda6818131"
      "368854483c4eb152b7530734855175cebba1f9579e419a10f98eede39388b67b7d90b5de"
      "73424c6519895e0f8e234acca9fc7a3b4cee7d9521eb76b17132f9cd20a250446e97c4c5"
      "031772d17aeeb24ae7fcfa8e48f76c2cea5e7f6dee4c5945e398fc6f7c56ca91552b5100"
      "1d50aabce4ed1e44447c8d9d2707aa6a6e68569d00144b5d8e053cdd3e886912836f60a4"
      "c78ffef279aef069a42f9753b458a77c6e9c3f6dd1a752fc404493a5099b80fe20b24f38"
      "4ebf72b9738cb416151a39a1feaea762ffc73a0c32cb35ac9a5723d8545c60e17117a83a"
      "5e8f5b99203679a96a9fa04943c369d3b02e481eee85d0e5db785d677b8f5e0964061c5e"
      "6617ac368292b93f87d007b30f037899f8ffffffffff11866eca74dc2209d5c4922bc6c0"
      "e7cd062c02ff95fff75ba31b3fe935b4cb28fd601c33c24e059f1316ec2d1beaa747f6c3"
      "04ee7f4daf0fed91565c1b90e2f769842dd05f1770d1a9b5f9429715238ad90349b3304b"
      "efaf41be596f6b6ed318677db496b76ce6d488231bab277c15808af3fc7e218537ded70e"
      "6540ffef33f6bccf05a9c839203fb1b289e773466dde397ce02ca0e8bbb90be82566b34c"
      "e7e0652b17920c839f0b3efbd17cf9d9d59780041be196b11827d482668efa056406050e"
      "9e847d8cff6600a2aa08deb5c65f59470e006a61e47626c4d94343128e3ae89d1b216e16"
      "17ce2ea2780f81ef9856dde3b6b751723ada34d488be578efa7bb168034dee63d80c6294"
      "b539ae275309c24730757646ddc3ea6f639d50ec4398d9b5276881285243be9c6318e23d"
      "7caea3475e522e15bff42fc5dd900b90caed622d862628abd64494d7f886b9dc68869950"
      "6409b3a287f813bdb032b00c2bef26ef16b06b4b9dd0063572dd1fa92b6c6f965822b9a1"
      "d885bb9cef2712a45d4b1bfe0ba4714ea7259046067a606be22636225990a6bb0981e29b"
      "8abe95a63398fb2dbdfad7d32a50c43a5a49a659143b0ba621ce40a015f71b2d96c0f46e"
      "9865ee2ca9e9a74dc0ffcac5fa9f69c43844f117ef7c5bea4a583e4fe8fde554c7f163c5"
      "322290579f41067cda5704fcba18060dbb4659cbeedad6d9971f231f3a9ffdea25292834"
      "db8a2ec98fdc6e296ac784d4632e615c74edad3dc1bf6e09a08c24433ee0ccb7fba249fa"
      "405a50702c5494121578b18f3f0d451795e0c5188bb09a000a64ab7d413ebd0f771dbbee"
      "d8d984ad184d7076c27bb6ca3d01913fc7b10dd72fc54eeac41de7dac17c22e4b4d00656"
      "4d34609f90e10ac6b002a0ee41e289da35d4b21c826216ac90873f05d7a9f76b1a3f4eb5"
      "0db77265c8206a776217b359bb3d1d0e441f6c91105d7328c5478794f699769e08696783"
      "f65fd117bb89d0053bbe815e3ef58562723c475675413e7083299d7f416283e573e96dca"
      "7de3a07f8be2aba8846125663ec39b77822cd8c1c348055300cd2eeeb47bae94ca821dae"
      "afc01cb8568e09b10ad7f7d1af4a8f4852823140515348676f172062d1d72ee80a1ecd3d"
      "db62afd8c88a752a810d728876f7505e8867feca6782ba563e7b696619d1570d993f26fc"
      "2c3c0f984e91d67ec780a431c093759e5133ef0677b0f6af33ca3ea4b1972168cea79040"
      "3d45a2f24378a9e94df75955948bf20d7a9f98b98305b258334c625f4b4179ad3d3e9ae9"
      "a3cae32f7375fb099ee0b4d3526af37c83d445711710d5e3e07c826d4b3fb622797d127f"
      "ab6b7b5fda599e8a9f479a56b5789a813ad47eb6282b2c6d23ff6537cb5a96d8d677e0d3"
      "628fac9b1cf43bf505125989b0f917762f727e0ab30df48a326dd7dc1869ebde0cb4f763"
      "715d6d428f91ba7700107adc4e71353f94763b03f86154fdab95a1b12c76b960d6368cd3"
      "290c5a7a34841e8337d0bdb1e4775e53b3b3fd39df0375f9cb414aabeb7702c7a5eb8334"
      "a021d86dcc7c96a7ba2d3a36b15047f8df4d97e07341189b008301483fcc990f37d70783"
      "9412aa4a65b0cb922868f054b29f319148daad482a580431b26817a06f5c9c8fa9b67c8b"
      "36dd31b7dcbe6af2ae1ffe7ac9af29eade831158a785f86543d282cc2d5845233d7a870d"
      "0252b87652c47207357108f06ce43f1a9b5643f68712f3e4bd067ea1a377e66cced8fa14"
      "c3b32fd6cf6d84e1429662dba421d07ca0ece1af657905e3aba8844e7b86930484144d5c"
      "d030d585076e0f568fdc962e90ab1d179472bffdcebff037943ac9acc8034ed983a047b7"
      "56e7081bac4053365037c2fe69644af538273d641645293981249ee852661b7523297392"
      "2c4052abeceed6563995cf1614864da674a084bf7b1aef1d14d7242369ba914594ae74f2"
      "dc417c6e4f60e0fce85e02457c19508a624943e7ae41f5377f33a3f80cf98ab7f300dd0c"
      "7badabe577e2a41fdaf5b746635db05c7c6951e72b70094c1ef0c9cce2b79f0b30da4504"
      "91c2fafacaa60a074b5c4efd7751171fc8200e0e03a4d89c16a37db3c2476c719a88e4a7"
      "852d16ac8408eafbcec296efe843c87db5761c78c5c535e3c43469ba1c609a3b23a3b714"
      "b27294b3d80b4598d8a594be830fc87d53f68dc82a6a550e0b9d89ca81dd72613bb156b9"
      "63402b1271ca0b942b910c3d205dfda73e7d371677d94c7b43b1c439d165cc51c0ebda49"
      "ac366a5e3b14245edd7adacc37aa2cea1f4391a4375f16286bb430c48818ed5b3c1dbc64"
      "a1a1f3d1ce1ed0e06d26c280083a006ea5fe6cc3d3a2d1a9c0400a70512ea2f02543bcc6"
      "740ec5694984b13314ba9bcccf033a78a3a020894ad123cee730e1ce21da8f64726ef4a0"
      "4f52b90af8ee21273ee5422cfdc8522d7f3031fc2aad6728a9ce454bcf9dac64765f6c4a"
      "d74eb271eafd90f2d314435fcc157241df8354fc0367c66b608ca532d320322e6e7d6890"
      "08eb7e030de7d9c2de2ec4f30d61ca7e37348df1158fbf45c89acf80d57911fa647c1639"
      "3a9ecc97d81793991743eda70136b954e2b5686074a7671f4ba1cad0eece5bd8447890c7"
      "0f9862a0b64ec3b211e3d73e6eb8a09254235e8bc4e3da2a115aef2872e14ac864521e53"
      "fbbd3c0f3bee9f36bd5aeda0a3d7aa084f6ef61900a23fd4098ba671684f93ec89ab6ee4"
      "3839b7e9b1c53c4d70ddf3a31646273ab0ef01704394f35c28b1d7c7053730ad145eb6bf"
      "c38d3a88622da7f7c00218be6e8dcc3dc1521811aaab7d5eb56db5a522d23f44b7d88028"
      "0277078299d3996ca20576333c34eb77a3f63b396e6499881ef08bb616b7bef278ee61f6"
      "06025d06e79d88ede6b7a5d1cceed0dc37fd461d3b89d6f00f5ca6161a05b64eb862c85c"
      "954a65253c4388c5a208fd6e32d7645d17a3a484c30e6b3acbad29a9086fe23b53f89f61"
      "4f731f44cab33812eb18c7e50d86df11c9abcc6b4c5797d890949fa8c4aac6f3e3b79073"
      "52372545e9116bfe1eab657bab8c4af094d68b32cb89a65b4708114be137a8fb19c91aa8"
      "94ee96c0041900fbec4a787ea4af9e9bb5abc568140a745a8c3ac8e6b7b8d5a38fd27433"
      "0443fbaa5734bfdfd1f976d17007d050fae6efde1fbac0d35aee6c652b4133613930bee7"
      "0977442f81c6fe91821bf0ab491626541799baae09fe4f81ef81b0c00db3ab772c0df60f"
      "cd3bc1086f05dc04a970b3a939e47995e35110af86eca4d23ad0134935d6daa435d91709"
      "7cded5d9f189ce5a50514b0d09e7d3c05d7e2b42139d1e7f076517e18939ca733d30bf3d"
      "50373e8f7068b52f5f8f73788b80010309e6274f2efe8aa0ed923aa597d5a4e4617a3377"
      "3455e00c58ae42cb34c80d8234b64241eef685ddcdc9f6fa2cc6c846057cc1b82858c30f"
      "b91b448838be04f43ae38027cd86e7c7025cf839335990c5b66845f27d7a8a3150d5b9c6"
      "40f6ad410d4d48fe97eb0e443923d2373259cb3134c1e0d23336608846f04d8b4580d559"
      "1cec8c5cc1e09eff4973f5313a9d147c15d5cba3010c7d8d3c2c808bb499b8f926286485"
      "f72a0c6f597ece43d86a4659edf014fc56b97da9e14b7a2c2400605c2acb713370c3c279"
      "297635ae720eccd881d26ef2ed88796a5a6f08cb9afd18c2338822f5ca0c84b7ce0a559f"
      "25bff113c1d31e7cebb7f0b9ebe47cf3f6202f5206b413d39c3529ce10e9a8ca4eac2362"
      "173c4a2da233ab1327aac4e704e31d8c3f9c754b3fbbc27c5f15a40aeef4caad8781893d"
      "6c51bd47d7895f6f61770e7c2224acbfb0eed0a20be5df4077f57181d1b515ed8f6bf2cb"
      "9b10f1eb020c200f2ff3827ed66ee3d9cf1aca11958fb0bfc89eee88e05dc26faf1ad4b5"
      "828ce6103a1f57998a3172a7940c1d79aeb3941d030c4198fc2f5a62bae5039c73690f63"
      "3655445bb466bc9bf2cd3dd28b06803a26132dd01c7d5f6ff63af1cafb39a05925a09d0a"
      "1d000a100ad7d6bf8f2375f35a992320ff31210058e021099715515b2d5dbe460c5508bf"
      "8c72f096d072c20ff2df2348047a92b6344798b64ac46a830ebbd9ad5a2dd9ab2fae622c"
      "a13da34f4130ee3c37973f71043e01da485e35120a8f321e37ab268e4081077b5cb995dd"
      "676dc1d3ed1ea187b1a49aaf16b3276830361f4ebc2ae0c33767f273a602cc72b05d9a44"
      "3d2393e912c0181d278b6f5a19e0591bfd03d93761e986302fed9435aa574837503a8f35"
      "c7306d3aba56979a1a9217d0afb2a84dcfae836da5eeb1ec948110a73f648bcf81e2672b"
      "2b724c2d2d4389b9bee1f89fd9666cec638d5185aedca78d0e1c12184211aaa27704580f"
      "345893ec68347b34d25a113cecad24a51332dc97559c281108e7cc47a171137839bab555"
      "fc1db45fecb84def5587fe467d485c3aead1083adefd21853ff96d7b34180d2dc0c04836"
      "ed80e6db954222acecc26a2ae9a0591b86ede2f24798e3cd2dc9ad3730aa4046b4d8c934"
      "042a068d325de4b409a88cf52d31bcdb2c78f4ae1f383a528b159398c88d3ad66ff2bacb"
      "9b9bc4d91b3e27da7604989685b55d6d209358a4074d1da5c19426edfcb278a3325e3293"
      "b4bcdbe651ce9ddbbbc908fb224e96edc73e03418da0ebee76b789aa9099371a228787ad"
      "8ad7153a03b212850428c82323fa52756c08a26a3cbdd52c7040c76ebf0f0456038c58af"
      "f10f7956212cf338d44ac70f3c75a962f23171c36ecfcc37da351f43e66e154234a4a0d2"
      "062a94d1ff819108b45228446446cf0c8415ceb0893d8d949bba74112ed1417f398c3c59"
      "c7c43e50c4aa42d68d919012027de8fa2082c4eb693a827ade5f13e812ed4b1cf68af837"
      "523c261c45e720d272250a5ca0190e0a1cd36a2b2ea8822a0c77dbfd0c256c164ea25269"
      "31cd5873bf5bec70580279818fe431d81cefdb5d2fb7ad5ca27e3168d6160d621970168b"
      "cd5ef9a769ad6436aedefdfd5de531860e5b2e28461a6107cbbc17c21456b6c6ab3ad9ea"
      "bd0b8bbaf27b910e70e2ee3e1dffca7597aa6e55699a81944693a7d4b522301e3cf90fdd"
      "d702e5ac0dd5b4b914459ae0a706fe533ed99327d3d1a3c167692218156962107ccd8f9e"
      "a4c8314d18ecfe8b31af4b4a4f38551c308deff92c971093280dd912b9e7f5c93ada08c1"
      "1198d3a61ea81e9f53dded8489d5e539dfa890c7f992e63a824d8bf264bfd07d3f27d1e5"
      "7583b295bf07e165142f0174a685385c7a912688ee523d00691db7a829ccf95495bac44f"
      "0c948f6f0ad2476eb0defd9d5160e3a645ba4b15ceb52bd82d31f19f9274082d2b24e851"
      "5fa0df87146f700e08de41fecbd6371659ecb2a91d098e068ddca50b4e153d448beffaf2"
      "bb059113c21407a4210c80fc8128548c2b0abc76e34bfd525f3ee70a8fb1847e32a13ef0"
      "fb56f8bf10f51d9327431e570b73b0c3d3dbcf29d2861a056196772f0e0ec22f5650893a"
      "8a77ee4196a6c501144cf7fd4998b45bc6e6e4ff436be122c462accbb368aa545df00d3d"
      "443f08290fec370dd5c3d32eccd0f8933ee9949833ef1630535161dff0685f406004d850"
      "24b4df09a9f03fb97b4c1d88393c7d8543a21491c8f78120f581e7c0a65f6660067d15b2"
      "ac4c8b973c9dbf0538a5738d9173cd61a5cd1230a6aecac74ac375082b5f78e219109b25"
      "acd5d353575a483338baee767ca83cbdf2016c099357c9b00adbed0e3ec16f4cf30eed8b"
      "f44e1ffe2c9be54083bcf12d60e4a24e95d4d4330e34564b2667f177cae46232523793cc"
      "a7104ff8f242d650dc6c8a37d6d54ad132f31749b0ebd261cb740fa7fcd7dbc3495e8ec7"
      "050f8da6b03e15bdf31341bd14620f0799baeab8769a58492a905aa904e3520a6b309327"
      "59d9d75dab616224250bf5587ab084b7a5632a0eeb890d9e8fef51781ff279b7561b117b"
      "e0de3bf90fe5e3629b845eca5c18c32793190b2f0ebee7dbce577ba9fd39aa1f54e841e7"
      "379176c226e48f7a15d25e2bc9dddd34e3ca74f7785e47c5a9d737466b64630c20270bc8"
      "90067d2cc09ad258142ec552d3ac363ef3405218c298cd07c5d4d5902ec47eeb35d8bad7"
      "8464648f2a82373640d03a790a5300c6f83963f264a4175c0b1e02de54aa2bf48c862591"
      "fac4cf5c325c4f2af9c4c04d4853569a924d9d361c63a8aede2e6042a59cc1192178bb1a"
      "4e86346b74924e8ba1c1ad8b8f3dff4f16ae672740060fccd0d81f6e9cb8cd8c205ffef1"
      "5c0b123216f8d91b7b2150ba17a63161ba221ade481025588b6bc21ff7ab71527abda0de"
      "85309f1ae0c0bd5b3f212fb7cf81283ed83a1c7b9eaa9632e6b1311da9f5c6f9b84841e5"
      "57829f97315172f4b0efbd5bb2806b50d6967e35f0d8f8d7793b60deefd87bf063b28ffa"
      "3f7a7f4ec26d2cddb4f08e6f1581fb49c884a747b13b415e44fcee44190c168c3a85517d"
      "55e170577e5988adfebc435b219852750beddafcba3f87e4b2b3c7ab10dba14475e13fff"
      "7e1f969225a6d069bf9e84044983b4b689712b1016cbc03a13d52aab204561d07550d914"
      "e28432f342db6d9af0c6acc8014de7a2fca22425293155c8d5375db6288b549be929a490"
      "729243aa909e9bc131dd29e6cac1048c13a3c290138ea44acb1696a6b1ad9f59a792170f"
      "fa31adf935fa01477cbfedf421c8b53a7ec51325de72818acd81609ed24f99989d4f8534"
      "7015f64b2a49d5b6310f851d367aecc11a0c37c48633eab02a9602c6afc243cb3b440dc8"
      "7d64377a12cc9c264bc7bd0d1ad1a82e72522be84a6ee11c254455b3c4ad9f92ad6b4cf1"
      "1c0d1c8269e753c27904c4276ba9e4f6b8340ab102721c7b3f3d3546a1acc4981868828f"
      "6de93365b507f3b75acf7fbf87fb627a15068732c10f259f53509b4a2c05d7f0b9d5618f"
      "beedd838daa54c177ed5424a14e21b39b5e639fbba1bc92937a64bdce30f1c36257203c9"
      "377b39b958112012d4c40cec4e9580bbc8285723193d7f3b082014d8265aa59e5f2779b2"
      "6618688871da749340b8dcc21beb915208af79c9be0f71e8734dae659dc89636070a9b2e"
      "325d1b2e97cdecccdda809b83ee02187e06ba1764223cad8832f4df10ed6b493b33765db"
      "47485ebde5214ee73a6ec581a4fa49e32c386111fac72dd7fe927497ebf5e76d9b293b1e"
      "21387aaa17d73a57d7b257ad00b9bdf9c51419f5c92869b481da721c5d7602c3dadb7898"
      "04a6821cc39d9b10411b127c4c5fd476334eb13d89dd52e49ef487ddd94ad8382c19fec5"
      "9c79143cdb97411980d83ac2c3db2629b2f298e8383b839fb91fcdf81b596ecad11be6e5"
      "e799f8d92464d395d44086761e0f04770fdab5a1c326408713fafe3fc15c13b998fe16aa"
      "7f1928d29ecad76063186870923580b44974e2282f99c961a4cb8c48160eb644efdbbec9"
      "08cb8be7ae3353a952b6f88dbdea292d388e5500541e328bc5d151a87aaff1ae3e90c36d"
      "9bd03bc6cc1734984437085221b7a2ce16c1dbb52e6384d2576560e0c78715a54ab29b29"
      "66d8eee5c03b08fa0358e89e655efe3586d73d82c8a85e88647a34bb76f672f659a95af0"
      "bca3a77f1c81d2fffc4578271e4526b3b86db5a07726eb4ca4eb69932ca38f8d2b0c22d7"
      "349012bfc9e3d4507df0f0b293c4955e1d5ffe4410d98525424a4a0ef31399d002e71857"
      "cc7898f25a04c341b14cf059216410a441e4876752416dba4d1ebe5b15db2199126dbdce"
      "b64819aa7b0aaf69c12f1ffe30cbcf750bb3055a24303fa93ad289631f1b29ab6cf806ad"
      "e6f4adfbf99a0b7c62e50ef2a6735070531ea626026afdeb3176fdb50011d04e9fe5520b"
      "86088935e5f91dd7f13eb3b8b45af20110d907b2a45e9ca726126a79a98c29de61fabccf"
      "af78289f72b4b084dd6c973f082f7a04189cc44f540adacc2b1c1c448c69815f9f2c4e77"
      "f9a0965dd88c7b87028b764a8f6eb16b0569bd72b0028a7211da1521971dbab6e3d66959"
      "c7ec0ed70922c12d26edc14fc1c7ff68936f6d8e86463e8619c5f1033b16566a27d118cc"
      "25f0a032616ae9d91b50c50d6663d50b0aa6f77232670f62829490c68322f01c3a026baf"
      "d3438fa26ca7b1b062f9820c92a31c058413f183a0ca01547aee6d813d4feea8b66285aa"
      "543e3232f6c2f701efabf434f8498944feae42a29b7c3df72d9c5d9d096565a4b3d3f514"
      "8b08a9a2d6153620c5227368e596c5decc3d63691e5319bdefe4dd1362897cbd192425fd"
      "861c70560243f8c0924509c30d8ea5f100a7639167cef87a3e8a0bdca2176d620d2f4766"
      "dba782c1530543b5f7906b4d364a43775aebce9d9e011d850bd646d2e6f2439123f3afd0"
      "9caa358df7d8df892b77501b127ddd63035eeda62056b066e87f560b4e3d18c2a54392ab"
      "14972b01136ac32b5e543bfdf1eee5be1d89235f5433266ff64c21daeed62bb0d8de1c66"
      "1a0d00f59732fccc51efb0650877e39613b238738281e768597962c44341b9dc3ee326fb"
      "3a468d0d19c0381951d8de89bce562bcfcea036bca0ce778b4a093f1114a30833c610563"
      "195cce6c62ddda2e469653234049c6bb7ba5698c4741b3b32febc850b23ea9dd5e6da58a"
      "dd1eb381e23e16e02f2dab9b888e2377fff650d92833e86a9cce46d6ba229bb096afbcc1"
      "787803bd8e1c87fcccef5d9acdda9a350dd3570142ec351171d01642a582cab4b473395b"
      "e349e41660a346558d14aaa622013cc4cc7290b820d1293c9011e90f5fac1c55feff0f50"
      "e3c06d8424e6b2ea05613a2e76d0a6bab75aea72768c839516cc56c550e88d18f0f046d1"
      "6870ce5c3beca0fb140bc37fa71e417a82b724537af9f36c4631f42c491e83a6509d7c58"
      "39094b9f5b1e937835480eb9a264e1b8d1b456ad6392e2344d1b7edd7decd16421b84ecd"
      "238f9773d06cb71948c93a479c9841a4d9b98bcba9bd63b49f4b70193046a4ab899cc232"
      "800b4916269ec95294747194ea3bfba5e7dabcfb44b9b08010aedc4acbaec7b5ad48ac49"
      "9585ed27f597976b310aa50e91af5772c9df21c031fcaaf4a40ff771bdc2a53bc1a89f5a"
      "e6c979d6aa622f98deede470b4c464fd36b122dbf1557c7fa9dac7e5cfe045e0e9600254"
      "2716b6f58bc84c7ea22b97e1136de913db947c80bc4931c0d3817fb714bb36250a0421f2"
      "67da1b5073ce0e781c660ff553869bbd69788a8e6cd3f1315df28a7648ea32f1c3011390"
      "ea9d416b13e7f8702158cd50ce9d8d2add88ca9e1e70dd301c797981dbe005a1427be180"
      "00d6902ca596c1412ac412dbce32a0a96fb1ed775abf794e76931712e8d23a7c135b5708"
      "b1f525adc45e9e229814ed45e5173dd0a795d56d3c749748c7e7fe97b784d51070627451"
      "8013d4ee72acf8e88e8bad08227fde5485d7118522e7455fb193cef5306605abb2615b3d"
      "2e2fd1ce9076a96b18bee1a104349ae11477d16c95bfa6b36c072c04281bf8508b4c85cd"
      "fdabd61932dfd5a942694fa2dc31bd27903195b6fbea06b23dd86f99c48c78fad8135ce3"
      "08e4c3ed171668539a45deef83ca3c71d20f3d2d4b6d768dbe63bb4232a3afa494f7e2f9"
      "8ef667f38cdfb6a213aad7ad3cb733a945e1d2e8af7452d2869f2c8930a19cf5a80dccfc"
      "62cda9888fc51cc6145afba8a9b10cf4170def45b7fe9b8f5a2ddbb7859eef215fc5b2f1"
      "8551fcd7a4b4235efb11efe5a893decdb1177c98995f240a829e4ec11d7180e7910a7ffb"
      "ec4058c2f74a6ad80ba79a8126a654df75485fe9469e074d5c61e050a1172c385ada6729"
      "387b380fd3c15809617b7ebc1a07566fbd903b6e56ac951982e629e581d60395c62add11"
      "252d87de43bc5e18799d932f03f043e7013d715086806a6b96d3a104c293c7082ab972d6"
      "b500313331ba7776d8a2ed7ff8c151490ef4f1697ece816352908859865e6b75ae27830d"
      "c4902d822b51136c4bc299921dcc6bd51dd22864d90e42fcd2df03a0ac65d426cbc9e939"
      "6091aab5bf91e85c25e3a4a000bf990a0afe4993b53015b47249a14746b9573d89d77695"
      "b164e79704ab793d64553714c35664ebdfa83665257014559fed5cadfb6a9861f12375b6"
      "2ab4b923ac37915ca4db52a8b93bac78852f580ab0f920f5fe71e33e878e905811243e54"
      "d1268d817e08e90f92147387b57c4ffcb27a04e0fe9d1cf7eac70fcd164e739333098c53"
      "707281a0e43f5e29d8d0c03f2a4642707ff93a47361a5b178fad56bd12dea5fe3eb66b12"
      "a2213e10169eea4b465bf71f1c457db36e011248b67958f3a4c7def635fa88ab74ae1a2b"
      "75b546e9cbcbfc72506757fd2523212603b7ee456f0e4317647023915aae01ba299fe89f"
      "1c2886f617a70fc0f51877af00b44c9c6f18a664a738a29ff62abad692d9828fbe4bea10"
      "c9f9b02678ffac04982a17bd6294f5154e1ccf4fbbe64930aeb61b0a93c5107d3aef2a4b"
      "f33ae9ed1bd2eb2df6219598a92720c82fbb051d7e3582473a083736274d46b2201ba9a3"
      "303367794c9853b4d5d10dde38479960e11015cc3e75b01e50c6f780fccbe86239ea3226"
      "f57438f7a7252812a609260bd1a7b692ed75a16966b899c95e100d4e357c720f11b6c9b1"
      "a0cc1ee2786fcff9586d891fd0748430170aeba911fc42a632",
      ToHex(captured_data));
}

#endif
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace brave_wallet
