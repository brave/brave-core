/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Eq;
using testing::SaveArg;
using testing::Truly;
using testing::WithArg;

namespace brave_wallet {

namespace bitcoin_rpc {
bool operator==(const UnspentOutput& l, const UnspentOutput& r) {
  return l.status.confirmed == r.status.confirmed && l.txid == r.txid &&
         l.value == r.value && l.vout == r.vout;
}

}  // namespace bitcoin_rpc

namespace mojom {
void PrintTo(const BitcoinBalancePtr& balance, ::std::ostream* os) {
  *os << balance->total_balance << "/" << balance->available_balance << "/"
      << balance->pending_balance;
}
}  // namespace mojom

class BitcoinWalletServiceUnitTest : public testing::Test {
 public:
  BitcoinWalletServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~BitcoinWalletServiceUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    bitcoin_test_rpc_server_ =
        std::make_unique<BitcoinTestRpcServer>(keyring_service_.get(), &prefs_);
    bitcoin_wallet_service_ = std::make_unique<BitcoinWalletService>(
        keyring_service_.get(), &prefs_,
        bitcoin_test_rpc_server_->GetURLLoaderFactory());
    bitcoin_wallet_service_->SetArrangeTransactionsForTesting(true);

    keyring_service_->CreateWallet(kMnemonicDivideCruise, kTestWalletPassword,
                                   base::DoNothing());

    btc_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kBitcoin84, 0);
    ASSERT_TRUE(btc_account_);
    bitcoin_test_rpc_server_->SetUpBitcoinRpc(btc_account_->account_id);
    keyring_service_->UpdateNextUnusedAddressForBitcoinAccount(
        btc_account_->account_id, 5, 5);
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  mojom::AccountIdPtr account_id() const {
    return btc_account_->account_id.Clone();
  }

 protected:
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletBitcoinFeature};

  mojom::AccountInfoPtr btc_account_;

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<BitcoinTestRpcServer> bitcoin_test_rpc_server_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<BitcoinWalletService> bitcoin_wallet_service_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(BitcoinWalletServiceUnitTest, GetBalance) {
  base::MockCallback<BitcoinWalletService::GetBalanceCallback> callback;

  // GetBalance works.
  auto expected_balance = mojom::BitcoinBalance::New();
  auto addresses = keyring_service_->GetBitcoinAddresses(account_id());
  for (const auto& addr : *addresses) {
    expected_balance->balances[addr->address_string] = 0;
  }

  auto address_0 = bitcoin_test_rpc_server_->Address0();
  auto address_6 = bitcoin_test_rpc_server_->Address6();

  expected_balance->balances[address_0] = 10000 - 5000 + 8888 - 2222;
  expected_balance->balances[address_6] = 100000 - 50000 + 88888 - 22222;
  expected_balance->total_balance = expected_balance->balances[address_0] +
                                    expected_balance->balances[address_6];
  expected_balance->available_balance =
      10000 + 100000 - 5000 - 50000 - 2222 - 22222;
  expected_balance->pending_balance = 8888 + 88888 - 2222 - 22222;
  EXPECT_CALL(callback,
              Run(EqualsMojo(expected_balance), std::optional<std::string>()));
  bitcoin_wallet_service_->GetBalance(account_id(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  bitcoin_test_rpc_server_->AddMempoolBalance(address_0, 1000, 10000000);
  bitcoin_test_rpc_server_->AddTransactedAddress(address_6);

  expected_balance->balances[address_0] = 0;
  expected_balance->balances[address_6] = 0;
  expected_balance->total_balance = expected_balance->balances[address_0] +
                                    expected_balance->balances[address_6];
  expected_balance->available_balance = 0;
  expected_balance->pending_balance = 1000 - 10000000;  // negative
  EXPECT_CALL(callback,
              Run(EqualsMojo(expected_balance), std::optional<std::string>()));
  bitcoin_wallet_service_->GetBalance(account_id(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, GetBitcoinAccountInfo) {
  base::MockCallback<BitcoinWalletService::GetBitcoinAccountInfoCallback>
      callback;

  auto expected_bitcoin_account_info = mojom::BitcoinAccountInfo::New();
  expected_bitcoin_account_info->next_receive_address =
      mojom::BitcoinAddress::New("bc1qe68jzwhglrs9lm0zf8ddqvzrdcxeg8ej5nd0rc",
                                 mojom::BitcoinKeyId::New(0, 5));
  expected_bitcoin_account_info->next_change_address =
      mojom::BitcoinAddress::New("bc1q9khch2y932xktwxxzplvaxw6r7h0pw2yeelvj7",
                                 mojom::BitcoinKeyId::New(1, 5));
  auto& stats_map = bitcoin_test_rpc_server_->address_stats_map();
  for (auto i = 0; i < 7; ++i) {
    auto addr = keyring_service_->GetBitcoinAddress(
        account_id(), mojom::BitcoinKeyId::New(0, 5 + i));
    if (i == 6) {
      stats_map[addr->address_string] =
          BitcoinTestRpcServer::EmptyAddressStats(addr->address_string);
    } else {
      stats_map[addr->address_string] =
          BitcoinTestRpcServer::TransactedAddressStats(addr->address_string);
    }
  }

  EXPECT_CALL(callback, Run(EqualsMojo(expected_bitcoin_account_info)));
  bitcoin_wallet_service_->GetBitcoinAccountInfo(account_id(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, RunDiscovery) {
  base::MockCallback<BitcoinWalletService::RunDiscoveryCallback> callback;
  auto bitcoin_account_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account_id());
  EXPECT_EQ(bitcoin_account_info->next_receive_address->key_id,
            mojom::BitcoinKeyId::New(0, 5));
  EXPECT_EQ(bitcoin_account_info->next_change_address->key_id,
            mojom::BitcoinKeyId::New(1, 5));

  auto& stats_map = bitcoin_test_rpc_server_->address_stats_map();

  for (auto i = 0; i < 23; ++i) {
    auto receive_addr = keyring_service_->GetBitcoinAddress(
        account_id(), mojom::BitcoinKeyId::New(0, 5 + i));
    if (i == 5) {  // (0, 5 + 5) will become next receive address
      stats_map[receive_addr->address_string] =
          BitcoinTestRpcServer::EmptyAddressStats(receive_addr->address_string);
    } else {
      stats_map[receive_addr->address_string] =
          BitcoinTestRpcServer::TransactedAddressStats(
              receive_addr->address_string);
    }

    auto change_addr = keyring_service_->GetBitcoinAddress(
        account_id(), mojom::BitcoinKeyId::New(1, 5 + i));
    if (i == 3) {  // (0, 5 + 3) will become next change address
      stats_map[change_addr->address_string] =
          BitcoinTestRpcServer::EmptyAddressStats(change_addr->address_string);
    } else {
      stats_map[change_addr->address_string] =
          BitcoinTestRpcServer::TransactedAddressStats(
              change_addr->address_string);
    }
  }

  auto expected_receive_address = keyring_service_->GetBitcoinAddress(
      account_id(), mojom::BitcoinKeyId::New(0, 10));
  EXPECT_CALL(callback, Run(Eq(std::ref(expected_receive_address)),
                            std::optional<std::string>()));
  bitcoin_wallet_service_->RunDiscovery(account_id(), false, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  bitcoin_account_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account_id());
  EXPECT_EQ(bitcoin_account_info->next_receive_address->key_id,
            mojom::BitcoinKeyId::New(0, 10));
  EXPECT_EQ(bitcoin_account_info->next_change_address->key_id,
            mojom::BitcoinKeyId::New(1, 5));

  auto expected_change_address = keyring_service_->GetBitcoinAddress(
      account_id(), mojom::BitcoinKeyId::New(1, 8));
  EXPECT_CALL(callback, Run(Eq(std::ref(expected_change_address)),
                            std::optional<std::string>()));
  bitcoin_wallet_service_->RunDiscovery(account_id(), true, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  bitcoin_account_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account_id());
  EXPECT_EQ(bitcoin_account_info->next_receive_address->key_id,
            mojom::BitcoinKeyId::New(0, 10));
  EXPECT_EQ(bitcoin_account_info->next_change_address->key_id,
            mojom::BitcoinKeyId::New(1, 8));
}

TEST_F(BitcoinWalletServiceUnitTest, GetUtxos) {
  using GetUtxosResult =
      base::expected<BitcoinWalletService::UtxoMap, std::string>;
  base::MockCallback<BitcoinWalletService::GetUtxosCallback> callback;

  // GetUtxos works.
  BitcoinWalletService::UtxoMap expected_utxos;
  auto addresses = keyring_service_->GetBitcoinAddresses(account_id());
  for (auto& addr : *addresses) {
    expected_utxos[addr->address_string].clear();
  }
  auto& utxo_0 =
      expected_utxos[bitcoin_test_rpc_server_->Address0()].emplace_back();
  utxo_0.txid = kMockBtcTxid1;
  utxo_0.vout = "1";
  utxo_0.value = "5000";
  utxo_0.status.confirmed = true;
  auto& utxo_6 =
      expected_utxos[bitcoin_test_rpc_server_->Address6()].emplace_back();
  utxo_6.txid = kMockBtcTxid2;
  utxo_6.vout = "7";
  utxo_6.value = "50000";
  utxo_6.status.confirmed = true;
  EXPECT_EQ(expected_utxos.size(), 12u);

  GetUtxosResult actual_utxos;
  EXPECT_CALL(callback, Run(Truly([&](const GetUtxosResult& arg) {
                EXPECT_TRUE(arg.has_value());
                EXPECT_EQ(arg.value(), expected_utxos);
                return true;
              })));
  bitcoin_wallet_service_->GetUtxos(account_id(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, CreateTransaction_UpdatesChangeAddress) {
  using CreateTransactionResult =
      base::expected<BitcoinTransaction, std::string>;
  base::MockCallback<BitcoinWalletService::CreateTransactionCallback> callback;

  BitcoinTransaction actual_tx;
  EXPECT_CALL(callback, Run(Truly([&](const CreateTransactionResult& arg) {
                EXPECT_TRUE(arg.has_value());
                actual_tx = arg.value();
                return true;
              })));

  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring_service_
          ->GetBitcoinAddress(account_id(), mojom::BitcoinKeyId::New(1, 5))
          ->address_string);

  bitcoin_wallet_service_->CreateTransaction(account_id(), kMockBtcAddress,
                                             48000, false, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  auto change_address_6 =
      keyring_service_
          ->GetBitcoinAddress(account_id(), mojom::BitcoinKeyId::New(1, 6))
          ->address_string;
  EXPECT_EQ(change_address_6, actual_tx.ChangeOutput()->address);
  EXPECT_EQ(change_address_6,
            keyring_service_->GetBitcoinAccountInfo(account_id())
                ->next_change_address->address_string);
}

TEST_F(BitcoinWalletServiceUnitTest, CreateTransaction) {
  using CreateTransactionResult =
      base::expected<BitcoinTransaction, std::string>;
  base::MockCallback<BitcoinWalletService::CreateTransactionCallback> callback;

  BitcoinTransaction actual_tx;
  EXPECT_CALL(callback, Run(Truly([&](const CreateTransactionResult& arg) {
                EXPECT_TRUE(arg.has_value());
                actual_tx = arg.value();
                return true;
              })));
  bitcoin_wallet_service_->CreateTransaction(account_id(), kMockBtcAddress,
                                             48000, false, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_EQ(actual_tx.locktime(), 12345u);
  EXPECT_EQ(actual_tx.amount(), 48000u);
  EXPECT_EQ(actual_tx.to(), kMockBtcAddress);

  EXPECT_EQ(actual_tx.EffectiveFeeAmount(), 4879u);  // ceil(23.456*208)
  EXPECT_EQ(actual_tx.TotalInputsAmount(), 50000u + 5000u);
  EXPECT_EQ(actual_tx.TotalOutputsAmount(), 50000u + 5000u - 4879u);

  EXPECT_EQ(actual_tx.inputs().size(), 2u);
  auto& input_0 = actual_tx.inputs().at(0);
  EXPECT_EQ(input_0.utxo_address, bitcoin_test_rpc_server_->Address0());
  EXPECT_EQ(base::HexEncode(input_0.utxo_outpoint.txid),
            base::ToUpperASCII(kMockBtcTxid1));
  EXPECT_EQ(input_0.utxo_outpoint.index, 1u);
  EXPECT_EQ(input_0.utxo_value, 5000u);
  EXPECT_TRUE(input_0.script_sig.empty());
  EXPECT_TRUE(input_0.witness.empty());

  auto& input_1 = actual_tx.inputs().at(1);
  EXPECT_EQ(input_1.utxo_address, bitcoin_test_rpc_server_->Address6());
  EXPECT_EQ(base::HexEncode(input_1.utxo_outpoint.txid),
            base::ToUpperASCII(kMockBtcTxid2));
  EXPECT_EQ(input_1.utxo_outpoint.index, 7u);
  EXPECT_EQ(input_1.utxo_value, 50000u);
  EXPECT_TRUE(input_1.script_sig.empty());
  EXPECT_TRUE(input_1.witness.empty());

  EXPECT_EQ(actual_tx.outputs().size(), 2u);

  auto& output_0 = actual_tx.outputs().at(0);
  EXPECT_EQ(output_0.address, kMockBtcAddress);
  EXPECT_EQ(base::HexEncode(output_0.script_pubkey),
            "0014751E76E8199196D454941C45D1B3A323F1433BD6");
  EXPECT_EQ(output_0.amount, 48000u);

  auto& output_1 = actual_tx.outputs().at(1);
  EXPECT_EQ(output_1.address,
            keyring_service_->GetBitcoinAccountInfo(account_id())
                ->next_change_address->address_string);
  EXPECT_EQ(output_1.amount, 50000u + 5000u - 48000u - 4879u);
  EXPECT_EQ(base::HexEncode(output_1.script_pubkey),
            "00142DAF8BA8858A8D65B8C6107ECE99DA1FAEF0B944");
}

TEST_F(BitcoinWalletServiceUnitTest, SignAndPostTransaction) {
  using CreateTransactionResult =
      base::expected<BitcoinTransaction, std::string>;
  base::MockCallback<BitcoinWalletService::CreateTransactionCallback> callback;

  base::MockCallback<BitcoinWalletService::SignAndPostTransactionCallback>
      sign_callback;

  BitcoinTransaction initial_tx;
  EXPECT_CALL(callback, Run(Truly([&](const CreateTransactionResult& arg) {
                EXPECT_TRUE(arg.has_value());
                initial_tx = arg.value();
                return true;
              })));
  bitcoin_wallet_service_->CreateTransaction(account_id(), kMockBtcAddress,
                                             48000, false, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  BitcoinTransaction signed_tx;
  EXPECT_CALL(sign_callback, Run(kMockBtcTxid3, _, ""))
      .WillOnce(
          WithArg<1>([&](const BitcoinTransaction& tx) { signed_tx = tx; }));
  bitcoin_wallet_service_->SignAndPostTransaction(
      account_id(), std::move(initial_tx), sign_callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&sign_callback);

  EXPECT_EQ(BitcoinSerializer::CalcTransactionWeight(signed_tx, false), 832u);
  EXPECT_EQ(BitcoinSerializer::CalcTransactionVBytes(signed_tx, false), 208u);
  EXPECT_EQ(signed_tx.EffectiveFeeAmount(), 4879u);
  EXPECT_EQ(signed_tx.TotalInputsAmount(), 50000u + 5000u);
  EXPECT_EQ(signed_tx.TotalOutputsAmount(), 50000u + 5000u - 4879u);

  EXPECT_EQ(
      bitcoin_test_rpc_server_->captured_raw_tx(),
      "020000000001"  // version/marker/flag

      "02"  // inputs
      "C5E29F841382F02A49BEAFAC756D14A211EC9089AD50E153767625B7508F38AA01000000"
      "00FDFFFFFF"
      "BEC2C52B2448A8733648E967D2B4559D0F1AA4BBBB93E53E9F516A12FB9C1CBD07000000"
      "00FDFFFFFF"

      "02"  // outputs
      "80BB000000000000160014751E76E8199196D454941C45D1B3A323F1433BD6"
      "49080000000000001600142DAF8BA8858A8D65B8C6107ECE99DA1FAEF0B944"

      // witness 0
      "02473044022010F337726DFB2E31D87279418EA0E2A305176CC7D9EC8566F8E27EAF658D"
      "5A700220696354637317BF41F7FB5C599A169BA015E033245E7B7949C8E59BEF4FF19005"
      "0121028256AD805CC35647890DEFD92AE6EF9BE31BA254E7E7D2834F8C403766C65FE7"

      // witness 1
      "0247304402205D08864CE25C834F50B624CDB35D9D05183A259F1C85513168FC1FE10B0B"
      "50DB02205E375EED2DB73F7A149B62A84D98299852CCCBA359DA968CA82A91899CF0DD05"
      "0121038F616FB0894BD77263DA0111E3BAB673AB9B77309FD7247177975698FEB2CDDE"

      "39300000"  // locktime
  );
}

TEST_F(BitcoinWalletServiceUnitTest, DiscoverAccount) {
  base::MockCallback<BitcoinWalletService::DiscoverAccountCallback> callback;
  bitcoin_test_rpc_server_->SetUpBitcoinRpc({});

  // By default discovered receive and change indexes are zero.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                return arg.value() == expected;
              })));
  bitcoin_wallet_service_->DiscoverAccount(mojom::KeyringId::kBitcoin84, 0,
                                           callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                expected.account_index = 1;
                return arg.value() == expected;
              })));
  bitcoin_wallet_service_->DiscoverAccount(mojom::KeyringId::kBitcoin84, 1,
                                           callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Receive addresses 5 and 30 for account 0 are transacted.
  auto receive_address_5 =
      keyring_service_
          ->GetBitcoinAccountDiscoveryAddress(mojom::KeyringId::kBitcoin84, 0,
                                              mojom::BitcoinKeyId::New(0, 5))
          ->address_string;
  bitcoin_test_rpc_server_->AddTransactedAddress(receive_address_5);
  auto receive_address_30 =
      keyring_service_
          ->GetBitcoinAccountDiscoveryAddress(mojom::KeyringId::kBitcoin84, 0,
                                              mojom::BitcoinKeyId::New(0, 30))
          ->address_string;
  bitcoin_test_rpc_server_->AddTransactedAddress(receive_address_30);

  // For acc 0 next receive address is 6.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                expected.next_unused_receive_index = 6;
                return arg.value() == expected;
              })));
  bitcoin_wallet_service_->DiscoverAccount(mojom::KeyringId::kBitcoin84, 0,
                                           callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // For acc 1 nothing is still discovered.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                expected.account_index = 1;
                return arg.value() == expected;
              })));
  bitcoin_wallet_service_->DiscoverAccount(mojom::KeyringId::kBitcoin84, 1,
                                           callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Receive address 15 and change address 17 for account 0 are transacted.
  auto receive_address_15 =
      keyring_service_
          ->GetBitcoinAccountDiscoveryAddress(mojom::KeyringId::kBitcoin84, 0,
                                              mojom::BitcoinKeyId::New(0, 15))
          ->address_string;
  bitcoin_test_rpc_server_->AddTransactedAddress(receive_address_15);
  auto change_address_17 =
      keyring_service_
          ->GetBitcoinAccountDiscoveryAddress(mojom::KeyringId::kBitcoin84, 0,
                                              mojom::BitcoinKeyId::New(1, 17))
          ->address_string;
  bitcoin_test_rpc_server_->AddTransactedAddress(change_address_17);
  // Receive address 8 for account 1 is transacted.
  auto receive_address_8 =
      keyring_service_
          ->GetBitcoinAccountDiscoveryAddress(mojom::KeyringId::kBitcoin84, 1,
                                              mojom::BitcoinKeyId::New(0, 8))
          ->address_string;
  bitcoin_test_rpc_server_->AddTransactedAddress(receive_address_8);

  // Discovered are 30+1 and 17+1.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                expected.next_unused_receive_index = 31;
                expected.next_unused_change_index = 18;
                return arg.value() == expected;
              })));
  bitcoin_wallet_service_->DiscoverAccount(mojom::KeyringId::kBitcoin84, 0,
                                           callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Discovered 9 and 0.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                expected.account_index = 1;
                expected.next_unused_receive_index = 9;
                return arg.value() == expected;
              })));
  bitcoin_wallet_service_->DiscoverAccount(mojom::KeyringId::kBitcoin84, 1,
                                           callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

}  // namespace brave_wallet
