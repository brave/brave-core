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
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    auto zcash_rpc = std::make_unique<testing::NiceMock<MockZCashRPC>>();
    zcash_wallet_service_ = std::make_unique<ZCashWalletService>(
        keyring_service_.get(), std::move(zcash_rpc));
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

  KeyringService* keyring_service() { return keyring_service_.get(); }

 protected:
  base::test::ScopedFeatureList feature_list_;

  mojom::AccountInfoPtr zcash_account_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<ZCashWalletService> zcash_wallet_service_;

  base::test::TaskEnvironment task_environment_;
};

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
      .WillOnce(WithArg<1>(
          [&](const ZCashTransaction& tx) { signed_tx = tx.Clone(); }));

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
  ON_CALL(*zcash_rpc(), GetLatestTreeState(_, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                ZCashRpc::GetTreeStateCallback callback) {
            std::move(callback).Run(base::unexpected("error"));
          }));

  base::MockCallback<ZCashWalletService::ShieldFundsCallback>
      shield_funds_callback;
  EXPECT_CALL(shield_funds_callback, Run(_, _))
      .WillOnce([&](const std::optional<std::string>& result,
                    const std::optional<std::string>& error) {
        EXPECT_FALSE(result);
        EXPECT_TRUE(error);
      });
  zcash_wallet_service_->ShieldFunds(mojom::kZCashMainnet, account_id.Clone(),
                                     shield_funds_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&shield_funds_callback);
}

// ShieldFunds test is disabled on Windows x86 due to timeout.
// See https://github.com/brave/brave-browser/issues/39698.
#if BUILDFLAG(IS_WIN) && defined(ARCH_CPU_X86)
#define MAYBE_ShieldFunds DISABLED_ShieldFunds
#else
#define MAYBE_ShieldFunds ShieldFunds
#endif
// https://zcashblockexplorer.com/transactions/9956437828356014ae531b71f6a0337bb7980abf5e4d3d572a117a3f97db8a15/raw
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

  base::MockCallback<ZCashWalletService::ShieldFundsCallback>
      shield_funds_callback;
  EXPECT_CALL(shield_funds_callback, Run(_, _));

  zcash_wallet_service_->ShieldFunds(mojom::kZCashMainnet, account_id.Clone(),
                                     shield_funds_callback.Get());
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
