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
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

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
                    const std::string& data,
                    SendTransactionCallback callback));

  MOCK_METHOD5(IsKnownAddress,
               void(const std::string& chain_id,
                    const std::string& addr,
                    uint64_t block_start,
                    uint64_t block_end,
                    IsKnownAddressCallback callback));
};

}  // namespace

class ZCashWalletServiceUnitTest : public testing::Test {
 public:
  ZCashWalletServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~ZCashWalletServiceUnitTest() override = default;

  void SetUp() override {
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

 protected:
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletZCashFeature};

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
    auto account_id = MakeZCashAccountId(mojom::CoinType::ZEC,
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

    zcash_transaction.inputs().push_back(std::move(input));
  }

  {
    ZCashTransaction::TxOutput output;
    output.address = "t1KrG29yWzoi7Bs2pvsgXozZYPvGG4D3sGi";
    output.amount = 500000;
    output.script_pubkey = ZCashAddressToScriptPubkey(output.address, false);

    zcash_transaction.outputs().push_back(std::move(output));
  }

  {
    ZCashTransaction::TxOutput output;
    output.address = "t1c61yifRMgyhMsBYsFDBa5aEQkgU65CGau";
    output.script_pubkey = ZCashAddressToScriptPubkey(output.address, false);
    output.amount = 35000;

    zcash_transaction.outputs().push_back(std::move(output));
  }

  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            mojom::BlockIDPtr response = mojom::BlockID::New();
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

  std::string captured_data;
  EXPECT_CALL(*zcash_rpc(), SendTransaction(_, _, _))
      .WillOnce([&](const std::string& chain_id, const std::string& data,
                    ZCashRpc::SendTransactionCallback callback) {
        captured_data = data;
        mojom::SendResponsePtr response = mojom::SendResponse::New();
        response->error_code = 0;
        std::move(callback).Run(std::move(response));
      });
  zcash_wallet_service_->SignAndPostTransaction(
      mojom::kZCashMainnet, account_id(), std::move(zcash_transaction),
      sign_callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&sign_callback);

  EXPECT_EQ(ToHex(signed_tx.inputs()[0].script_sig),
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
            mojom::BlockIDPtr response = mojom::BlockID::New();
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

    auto account_id = MakeZCashAccountId(mojom::CoinType::ZEC,
                                         mojom::KeyringId::kZCashMainnet,
                                         mojom::AccountKind::kDerived, 0);

    zcash_wallet_service_->RunDiscovery(std::move(account_id),
                                        std::move(discovery_callback));
    base::RunLoop().RunUntilIdle();

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

    auto account_id = MakeZCashAccountId(mojom::CoinType::ZEC,
                                         mojom::KeyringId::kZCashMainnet,
                                         mojom::AccountKind::kDerived, 0);

    zcash_wallet_service_->RunDiscovery(std::move(account_id),
                                        std::move(discovery_callback));
    base::RunLoop().RunUntilIdle();

    EXPECT_TRUE(callback_called);
  }
}

TEST_F(ZCashWalletServiceUnitTest, AddressDiscovery_FromPrefs) {
  ON_CALL(*zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            mojom::BlockIDPtr response = mojom::BlockID::New();
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
    auto account_id = MakeZCashAccountId(mojom::CoinType::ZEC,
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

    auto account_id = MakeZCashAccountId(mojom::CoinType::ZEC,
                                         mojom::KeyringId::kZCashMainnet,
                                         mojom::AccountKind::kDerived, 0);

    zcash_wallet_service_->RunDiscovery(std::move(account_id),
                                        std::move(discovery_callback));
    base::RunLoop().RunUntilIdle();

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

}  // namespace brave_wallet
