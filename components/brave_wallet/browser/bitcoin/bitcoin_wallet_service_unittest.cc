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
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::ElementsAreArray;
using testing::Eq;
using testing::SaveArg;
using testing::Truly;
using testing::WithArg;

namespace brave_wallet {

namespace {
class AccountsChangedWaiter : public KeyringServiceObserverBase {
 public:
  explicit AccountsChangedWaiter(KeyringService& service) {
    service.AddObserver(observer_receiver_.BindNewPipeAndPassRemote());
  }
  ~AccountsChangedWaiter() override = default;

  void AccountsChanged() override { run_loop_.Quit(); }

  void Wait() { run_loop_.Run(); }

 private:
  mojo::Receiver<mojom::KeyringServiceObserver> observer_receiver_{this};
  base::RunLoop run_loop_;
};

bool EqualsNoBalance(const DiscoveredBitcoinAccount& left,
                     const DiscoveredBitcoinAccount& right) {
  return std::tie(left.next_unused_receive_index,
                  left.next_unused_change_index) ==
         std::tie(right.next_unused_receive_index,
                  right.next_unused_change_index);
}

}  // namespace

namespace bitcoin_rpc {
bool operator==(const UnspentOutput& l, const UnspentOutput& r) {
  return l.status.confirmed == r.status.confirmed && l.txid == r.txid &&
         l.value == r.value && l.vout == r.vout;
}

}  // namespace bitcoin_rpc

class BitcoinWalletServiceUnitTest : public testing::Test {
 public:
  BitcoinWalletServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~BitcoinWalletServiceUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    bitcoin_test_rpc_server_ = std::make_unique<BitcoinTestRpcServer>();
    bitcoin_wallet_service_ = std::make_unique<BitcoinWalletService>(
        *keyring_service_, *network_manager_,
        bitcoin_test_rpc_server_->GetURLLoaderFactory());
    bitcoin_wallet_service_->SetArrangeTransactionsForTesting(true);

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  void SetupBtcAccount(uint32_t next_receive_index = 0,
                       uint32_t next_change_index = 0) {
    bitcoin_test_rpc_server_->SetUpBitcoinRpc(kMnemonicDivideCruise, 0);
    btc_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kBitcoin84, 0);
    ASSERT_TRUE(btc_account_);
    task_environment_.RunUntilIdle();
    keyring_service_->UpdateNextUnusedAddressForBitcoinAccount(
        btc_account_->account_id, next_receive_index, next_change_index);
  }

  void SetupHwBtcAccount(uint32_t next_receive_index = 0,
                         uint32_t next_change_index = 0) {
    bitcoin_test_rpc_server_->SetUpBitcoinRpc(kMnemonicDivideCruise, 0);
    hw_btc_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kBitcoinHardware, 0);
    ASSERT_TRUE(hw_btc_account_);
    task_environment_.RunUntilIdle();
    keyring_service_->UpdateNextUnusedAddressForBitcoinAccount(
        hw_btc_account_->account_id, next_receive_index, next_change_index);
  }

  mojom::AccountIdPtr account_id() const {
    return btc_account_->account_id.Clone();
  }

  mojom::AccountIdPtr hw_account_id() const {
    return hw_btc_account_->account_id.Clone();
  }

 protected:
  base::test::ScopedFeatureList scoped_btc_feature_{
      features::kBraveWalletBitcoinFeature};
  base::test::ScopedFeatureList scoped_btc_ledger_feature_{
      features::kBraveWalletBitcoinLedgerFeature};

  mojom::AccountInfoPtr btc_account_;
  mojom::AccountInfoPtr hw_btc_account_;

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<BitcoinTestRpcServer> bitcoin_test_rpc_server_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<BitcoinWalletService> bitcoin_wallet_service_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(BitcoinWalletServiceUnitTest, GetBalance) {
  SetupBtcAccount();

  base::MockCallback<BitcoinWalletService::GetBalanceCallback> callback;

  // GetBalance works.
  auto expected_balance = mojom::BitcoinBalance::New();
  auto addresses = keyring_service_->GetBitcoinAddresses(account_id());

  auto address_0 = bitcoin_test_rpc_server_->Address0().Clone();
  auto address_6 = bitcoin_test_rpc_server_->Address6().Clone();

  expected_balance->balances[address_0->address_string] =
      10000 - 5000 + 8888 - 2222;
  expected_balance->balances[address_6->address_string] =
      100000 - 50000 + 88888 - 22222;
  expected_balance->total_balance =
      expected_balance->balances[address_0->address_string] +
      expected_balance->balances[address_6->address_string];
  expected_balance->available_balance =
      10000 + 100000 - 5000 - 50000 - 2222 - 22222;
  expected_balance->pending_balance = 8888 + 88888 - 2222 - 22222;
  EXPECT_CALL(callback, Run(Truly([&](const mojom::BitcoinBalancePtr& balance) {
                              return balance == expected_balance;
                            }),
                            std::optional<std::string>()));
  bitcoin_wallet_service_->GetBalance(account_id(), callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  bitcoin_test_rpc_server_->AddMempoolBalance(address_0, 1000, 10000000);
  bitcoin_test_rpc_server_->AddTransactedAddress(address_6);

  expected_balance->balances.clear();
  expected_balance->total_balance = 0;
  expected_balance->available_balance = 0;
  expected_balance->pending_balance = 1000 - 10000000;  // negative
  EXPECT_CALL(callback, Run(Truly([&](const mojom::BitcoinBalancePtr& balance) {
                              return balance == expected_balance;
                            }),
                            std::optional<std::string>()));
  bitcoin_wallet_service_->GetBalance(account_id(), callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, GetBitcoinAccountInfo) {
  SetupBtcAccount(5, 5);

  base::MockCallback<BitcoinWalletService::GetBitcoinAccountInfoCallback>
      callback;

  auto expected_bitcoin_account_info = mojom::BitcoinAccountInfo::New();
  expected_bitcoin_account_info->next_receive_address =
      mojom::BitcoinAddress::New("bc1qe68jzwhglrs9lm0zf8ddqvzrdcxeg8ej5nd0rc",
                                 mojom::BitcoinKeyId::New(0, 5));
  expected_bitcoin_account_info->next_change_address =
      mojom::BitcoinAddress::New("bc1q9khch2y932xktwxxzplvaxw6r7h0pw2yeelvj7",
                                 mojom::BitcoinKeyId::New(1, 5));

  EXPECT_CALL(callback, Run(Truly([&](const mojom::BitcoinAccountInfoPtr& arg) {
                EXPECT_EQ(expected_bitcoin_account_info, arg);
                return true;
              })));
  bitcoin_wallet_service_->GetBitcoinAccountInfo(account_id(), callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, RunDiscovery) {
  SetupBtcAccount(5, 5);

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
  task_environment_.RunUntilIdle();
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
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  bitcoin_account_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account_id());
  EXPECT_EQ(bitcoin_account_info->next_receive_address->key_id,
            mojom::BitcoinKeyId::New(0, 10));
  EXPECT_EQ(bitcoin_account_info->next_change_address->key_id,
            mojom::BitcoinKeyId::New(1, 8));
}

TEST_F(BitcoinWalletServiceUnitTest, GetUtxos) {
  SetupBtcAccount(5, 5);

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
      expected_utxos[bitcoin_test_rpc_server_->Address0()->address_string]
          .emplace_back();
  utxo_0.txid = kMockBtcTxid1;
  utxo_0.vout = "1";
  utxo_0.value = "5000";
  utxo_0.status.confirmed = true;
  auto& utxo_6 =
      expected_utxos[bitcoin_test_rpc_server_->Address6()->address_string]
          .emplace_back();
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
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, CreateTransaction_UpdatesChangeAddress) {
  SetupBtcAccount(5, 5);

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
      keyring_service_->GetBitcoinAddress(account_id(),
                                          mojom::BitcoinKeyId::New(1, 5)));

  bitcoin_wallet_service_->CreateTransaction(account_id(), kMockBtcAddress,
                                             48000, false, callback.Get());
  task_environment_.RunUntilIdle();
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
  SetupBtcAccount(5, 5);

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
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_EQ(actual_tx.locktime(), 12345u);
  EXPECT_EQ(actual_tx.amount(), 48000u);
  EXPECT_EQ(actual_tx.to(), kMockBtcAddress);

  EXPECT_EQ(actual_tx.EffectiveFeeAmount(), 4879u);  // ceil(23.456*208)
  EXPECT_EQ(actual_tx.TotalInputsAmount(), 50000u + 5000u);
  EXPECT_EQ(actual_tx.TotalOutputsAmount(), 50000u + 5000u - 4879u);

  EXPECT_EQ(actual_tx.inputs().size(), 2u);
  auto& input_0 = actual_tx.inputs().at(0);
  EXPECT_EQ(input_0.utxo_address,
            bitcoin_test_rpc_server_->Address0()->address_string);
  EXPECT_EQ(base::HexEncode(input_0.utxo_outpoint.txid),
            base::ToUpperASCII(kMockBtcTxid1));
  EXPECT_EQ(input_0.utxo_outpoint.index, 1u);
  EXPECT_EQ(input_0.utxo_value, 5000u);
  EXPECT_TRUE(input_0.script_sig.empty());
  EXPECT_TRUE(input_0.witness.empty());

  auto& input_1 = actual_tx.inputs().at(1);
  EXPECT_EQ(input_1.utxo_address,
            bitcoin_test_rpc_server_->Address6()->address_string);
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
  SetupBtcAccount(5, 5);

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
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  BitcoinTransaction signed_tx;
  EXPECT_CALL(sign_callback, Run(kMockBtcTxid3, _, ""))
      .WillOnce(
          WithArg<1>([&](const BitcoinTransaction& tx) { signed_tx = tx; }));
  bitcoin_wallet_service_->SignAndPostTransaction(
      account_id(), std::move(initial_tx), sign_callback.Get());
  task_environment_.RunUntilIdle();
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

TEST_F(BitcoinWalletServiceUnitTest, PostHwSignedTransaction) {
  SetupHwBtcAccount(5, 5);

  using CreateTransactionResult =
      base::expected<BitcoinTransaction, std::string>;
  base::MockCallback<BitcoinWalletService::CreateTransactionCallback> callback;

  BitcoinTransaction actual_tx;
  EXPECT_CALL(callback, Run(Truly([&](const CreateTransactionResult& arg) {
                EXPECT_TRUE(arg.has_value());
                actual_tx = arg.value();
                return true;
              })));
  bitcoin_wallet_service_->CreateTransaction(hw_account_id(), kMockBtcAddress,
                                             48000, false, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  base::MockCallback<BitcoinWalletService::PostHwSignedTransactionCallback>
      sign_callback;

  BitcoinTransaction signed_tx;
  EXPECT_CALL(sign_callback, Run(kMockBtcTxid3, _, ""))
      .WillOnce(
          WithArg<1>([&](const BitcoinTransaction& tx) { signed_tx = tx; }));
  auto bitcoin_signature = mojom::BitcoinSignature::New();
  bitcoin_signature->witness_array.push_back(
      std::vector<uint8_t>{0x00, 0x01, 0x02});
  bitcoin_signature->witness_array.push_back(
      std::vector<uint8_t>{0xa0, 0xb0, 0xc0});
  bitcoin_wallet_service_->PostHwSignedTransaction(
      hw_account_id(), std::move(actual_tx), *bitcoin_signature,
      sign_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&sign_callback);

  EXPECT_EQ(BitcoinSerializer::CalcTransactionWeight(signed_tx, false), 624u);
  EXPECT_EQ(BitcoinSerializer::CalcTransactionVBytes(signed_tx, false), 156u);
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
      "000102"

      // witness 1
      "A0B0C0"

      "39300000"  // locktime
  );
}

TEST_F(BitcoinWalletServiceUnitTest, DiscoverExtendedKeyAccount) {
  base::MockCallback<BitcoinWalletService::GetExtendedKeyAccountBalanceCallback>
      callback;
  bitcoin_test_rpc_server_->SetUpBitcoinRpc(std::nullopt, std::nullopt);

  // Xpub form kMnemonicDivideCruise m/84'/0'/0'.
  const auto* xpub_extended_key =
      "xpub6C9TRymDq1G8ueHrv4Etbvzv1ARp4fFAHezEuLQ7X3VcZM7ZKco3aBup3fyzSHhnbFfX"
      "tXF3m8EWTwk1TMvTVSciQ1BHxtvjMGcGLkCE2nz";

  // Some addresses for account 0 are transacted.
  auto receive_address_0_5 =
      keyring_service_->GetBitcoinAccountDiscoveryAddress(
          mojom::KeyringId::kBitcoin84, 0, mojom::BitcoinKeyId::New(0, 5));
  bitcoin_test_rpc_server_->AddBalanceAddress(receive_address_0_5, 10000);
  auto receive_address_1_10 =
      keyring_service_->GetBitcoinAccountDiscoveryAddress(
          mojom::KeyringId::kBitcoin84, 0, mojom::BitcoinKeyId::New(1, 10));
  bitcoin_test_rpc_server_->AddBalanceAddress(receive_address_1_10, 2000000);
  auto receive_address_0_20 =
      keyring_service_->GetBitcoinAccountDiscoveryAddress(
          mojom::KeyringId::kBitcoin84, 0, mojom::BitcoinKeyId::New(0, 20));
  bitcoin_test_rpc_server_->AddMempoolBalance(receive_address_0_20, 2000, 1000);
  // This wan't be discovered.
  auto receive_address_0_100 =
      keyring_service_->GetBitcoinAccountDiscoveryAddress(
          mojom::KeyringId::kBitcoin84, 0, mojom::BitcoinKeyId::New(0, 100));
  bitcoin_test_rpc_server_->AddMempoolBalance(receive_address_0_100, 2000,
                                              1000);

  EXPECT_CALL(
      callback,
      Run(Truly([&](const mojom::BitcoinBalancePtr& balance) {
            auto expected = mojom::BitcoinBalance::New();
            expected->total_balance = 2011000;
            expected->available_balance = 2010000;
            expected->pending_balance = 1000;
            expected->balances["bc1qe68jzwhglrs9lm0zf8ddqvzrdcxeg8ej5nd0rc"] =
                10000;
            expected->balances["bc1qajvjuwcexnnv7h58kqgdl435l9ap0cuyg260tp"] =
                2000000;
            expected->balances["bc1q4mfd6eghcyu36g7g9fauccjhnqwudfa9ecmvx0"] =
                1000;
            return balance == expected;
          }),
          std::optional<std::string>()));
  bitcoin_wallet_service_->GetExtendedKeyAccountBalance(
      mojom::kBitcoinMainnet, xpub_extended_key, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, DiscoverWalletAccount) {
  base::MockCallback<BitcoinWalletService::DiscoverWalletAccountCallback>
      callback;
  bitcoin_test_rpc_server_->SetUpBitcoinRpc(std::nullopt, std::nullopt);

  // By default discovered receive and change indexes are zero.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                return EqualsNoBalance(arg.value(), expected);
              })));
  bitcoin_wallet_service_->DiscoverWalletAccount(mojom::KeyringId::kBitcoin84,
                                                 0, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                return EqualsNoBalance(arg.value(), expected);
              })));
  bitcoin_wallet_service_->DiscoverWalletAccount(mojom::KeyringId::kBitcoin84,
                                                 1, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Receive addresses 5 and 30 for account 0 are transacted.
  auto receive_address_5 = keyring_service_->GetBitcoinAccountDiscoveryAddress(
      mojom::KeyringId::kBitcoin84, 0, mojom::BitcoinKeyId::New(0, 5));
  bitcoin_test_rpc_server_->AddTransactedAddress(receive_address_5);
  auto receive_address_30 = keyring_service_->GetBitcoinAccountDiscoveryAddress(
      mojom::KeyringId::kBitcoin84, 0, mojom::BitcoinKeyId::New(0, 30));
  bitcoin_test_rpc_server_->AddTransactedAddress(receive_address_30);

  // For acc 0 next receive address is 6.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                expected.next_unused_receive_index = 6;
                return EqualsNoBalance(arg.value(), expected);
              })));
  bitcoin_wallet_service_->DiscoverWalletAccount(mojom::KeyringId::kBitcoin84,
                                                 0, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // For acc 1 nothing is still discovered.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                return EqualsNoBalance(arg.value(), expected);
              })));
  bitcoin_wallet_service_->DiscoverWalletAccount(mojom::KeyringId::kBitcoin84,
                                                 1, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Receive address 15 and change address 17 for account 0 are transacted.
  auto receive_address_15 = keyring_service_->GetBitcoinAccountDiscoveryAddress(
      mojom::KeyringId::kBitcoin84, 0, mojom::BitcoinKeyId::New(0, 15));
  bitcoin_test_rpc_server_->AddTransactedAddress(receive_address_15);
  auto change_address_17 = keyring_service_->GetBitcoinAccountDiscoveryAddress(
      mojom::KeyringId::kBitcoin84, 0, mojom::BitcoinKeyId::New(1, 17));
  bitcoin_test_rpc_server_->AddTransactedAddress(change_address_17);
  // Receive address 8 for account 1 is transacted.
  auto receive_address_8 = keyring_service_->GetBitcoinAccountDiscoveryAddress(
      mojom::KeyringId::kBitcoin84, 1, mojom::BitcoinKeyId::New(0, 8));
  bitcoin_test_rpc_server_->AddTransactedAddress(receive_address_8);

  // Discovered are 30+1 and 17+1.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                expected.next_unused_receive_index = 31;
                expected.next_unused_change_index = 18;
                return EqualsNoBalance(arg.value(), expected);
              })));
  bitcoin_wallet_service_->DiscoverWalletAccount(mojom::KeyringId::kBitcoin84,
                                                 0, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Discovered 9 and 0.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                DiscoveredBitcoinAccount expected;
                expected.next_unused_receive_index = 9;
                return EqualsNoBalance(arg.value(), expected);
              })));
  bitcoin_wallet_service_->DiscoverWalletAccount(mojom::KeyringId::kBitcoin84,
                                                 1, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, GetBtcHardwareTransactionSignData) {
  SetupHwBtcAccount(5, 5);

  using CreateTransactionResult =
      base::expected<BitcoinTransaction, std::string>;
  base::MockCallback<BitcoinWalletService::CreateTransactionCallback> callback;

  BitcoinTransaction actual_tx;
  EXPECT_CALL(callback, Run(Truly([&](const CreateTransactionResult& arg) {
                EXPECT_TRUE(arg.has_value());
                actual_tx = arg.value();
                return true;
              })));
  bitcoin_wallet_service_->CreateTransaction(hw_account_id(), kMockBtcAddress,
                                             48000, false, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  auto sign_data = bitcoin_wallet_service_->GetBtcHardwareTransactionSignData(
      actual_tx, hw_account_id());

  EXPECT_EQ(sign_data->inputs.size(), 2u);
  EXPECT_EQ(sign_data->inputs[0],
            mojom::BtcHardwareTransactionSignInputData::New(
                std::vector<uint8_t>{0xAA, 0x38}, 1, "84'/0'/0'/0/0"));
  EXPECT_EQ(sign_data->inputs[1],
            mojom::BtcHardwareTransactionSignInputData::New(
                std::vector<uint8_t>{0xBD, 0x1C}, 7, "84'/0'/0'/1/0"));

  EXPECT_EQ(base::HexEncode(sign_data->output_script),
            "0280BB000000000000160014751E76E8199196D454941C45D1B3A323F1433BD649"
            "080000000000001600142DAF8BA8858A8D65B8C6107ECE99DA1FAEF0B944");
  EXPECT_EQ(sign_data->change_path, "84'/0'/0'/1/5");

  EXPECT_EQ(sign_data->lock_time, 12345u);
}

TEST_F(BitcoinWalletServiceUnitTest, BitcoinAddHDAccountRunsDiscovery) {
  bitcoin_test_rpc_server_->SetUpBitcoinRpc(kMnemonicAbandonAbandon, 0);

  auto account = keyring_service_->AddAccountSync(
      mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84, "acc");

  auto acc_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account->account_id);
  EXPECT_EQ(0u, acc_info->next_receive_address->key_id->index);
  EXPECT_EQ(0u, acc_info->next_change_address->key_id->index);

  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring_service_->GetBitcoinAddress(
          account->account_id,
          mojom::BitcoinKeyId::New(kBitcoinReceiveIndex, 4)));
  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring_service_->GetBitcoinAddress(
          account->account_id,
          mojom::BitcoinKeyId::New(kBitcoinChangeIndex, 7)));

  AccountsChangedWaiter(*keyring_service_).Wait();

  // Discovery finishes and account's address indexes get updated.
  acc_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account->account_id);
  EXPECT_EQ(5u, acc_info->next_receive_address->key_id->index);
  EXPECT_EQ(8u, acc_info->next_change_address->key_id->index);
}

TEST_F(BitcoinWalletServiceUnitTest, BitcoinAddHardwareAccountRunsDiscovery) {
  bitcoin_test_rpc_server_->SetUpBitcoinRpc(kMnemonicAbandonAbandon, 0);
  mojom::HardwareWalletAccountPtr hw_account =
      mojom::HardwareWalletAccount::New(
          kBtcMainnetHardwareAccount0, "m/84'/0'/0'", "HW Account",
          mojom::HardwareVendor::kLedger, "device1",
          mojom::KeyringId::kBitcoinHardware);
  auto account =
      keyring_service_->AddBitcoinHardwareAccountSync(std::move(hw_account));
  ASSERT_TRUE(account);

  auto acc_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account->account_id);
  EXPECT_EQ(0u, acc_info->next_receive_address->key_id->index);
  EXPECT_EQ(0u, acc_info->next_change_address->key_id->index);

  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring_service_->GetBitcoinAddress(
          account->account_id,
          mojom::BitcoinKeyId::New(kBitcoinReceiveIndex, 4)));
  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring_service_->GetBitcoinAddress(
          account->account_id,
          mojom::BitcoinKeyId::New(kBitcoinChangeIndex, 7)));

  AccountsChangedWaiter(*keyring_service_).Wait();

  // Discovery finishes and account's address indexes get updated.
  acc_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account->account_id);
  EXPECT_EQ(5u, acc_info->next_receive_address->key_id->index);
  EXPECT_EQ(8u, acc_info->next_change_address->key_id->index);
}

TEST_F(BitcoinWalletServiceUnitTest, BitcoinImportRunsDiscovery) {
  bitcoin_test_rpc_server_->SetUpBitcoinRpc(kMnemonicAbandonAbandon, 0);

  auto account = keyring_service_->ImportBitcoinAccountSync(
      "acc", kBtcMainnetImportAccount0, mojom::kBitcoinMainnet);

  auto acc_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account->account_id);
  EXPECT_EQ(0u, acc_info->next_receive_address->key_id->index);
  EXPECT_EQ(0u, acc_info->next_change_address->key_id->index);

  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring_service_->GetBitcoinAddress(
          account->account_id,
          mojom::BitcoinKeyId::New(kBitcoinReceiveIndex, 4)));
  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring_service_->GetBitcoinAddress(
          account->account_id,
          mojom::BitcoinKeyId::New(kBitcoinChangeIndex, 7)));

  AccountsChangedWaiter(*keyring_service_).Wait();

  // Discovery finishes and account's address indexes get updated.
  acc_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account->account_id);
  EXPECT_EQ(5u, acc_info->next_receive_address->key_id->index);
  EXPECT_EQ(8u, acc_info->next_change_address->key_id->index);
}

}  // namespace brave_wallet
