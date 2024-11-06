/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_max_send_solver.h"

#include <utility>

#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_hd_keyring.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "components/grit/brave_components_strings.h"
#include "crypto/sha2.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using testing::UnorderedElementsAreArray;

namespace brave_wallet {

class BitcoinMaxSendSolverUnitTest : public testing::Test {
 public:
  BitcoinMaxSendSolverUnitTest() = default;
  ~BitcoinMaxSendSolverUnitTest() override = default;

 protected:
  BitcoinTransaction MakeMockTransaction(uint32_t receive_index = 123) {
    BitcoinTransaction transaction;
    transaction.set_to(
        keyring_
            .GetAddress(
                1, mojom::BitcoinKeyId(kBitcoinReceiveIndex, receive_index))
            ->address_string);
    transaction.set_amount(0);
    transaction.sending_max_amount();
    transaction.set_locktime(12345);

    BitcoinTransaction::TxOutput target_output;
    target_output.type = BitcoinTransaction::TxOutputType::kTarget;
    target_output.amount = transaction.amount();
    target_output.address = transaction.to();
    target_output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
        target_output.address, testnet_);
    EXPECT_FALSE(target_output.script_pubkey.empty());
    transaction.AddOutput(std::move(target_output));

    return transaction;
  }

  BitcoinTransaction::TxInput MakeMockTxInput(uint64_t amount, uint32_t index) {
    auto address =
        keyring_
            .GetAddress(0, mojom::BitcoinKeyId(kBitcoinReceiveIndex, index))
            ->address_string;

    BitcoinTransaction::TxInput tx_input;
    tx_input.utxo_address = address;
    std::string txid_fake = address + base::NumberToString(amount);
    tx_input.utxo_outpoint.txid =
        crypto::SHA256Hash(base::as_bytes(base::make_span(txid_fake)));
    tx_input.utxo_outpoint.index = tx_input.utxo_outpoint.txid.back();
    tx_input.utxo_value = amount;

    return tx_input;
  }

  double fee_rate() const { return 11.1; }
  double longterm_fee_rate() const { return 3.0; }

  bool testnet_ = false;
  BitcoinHDKeyring keyring_{*MnemonicToSeed(kMnemonicAbandonAbandon), testnet_};
};

TEST_F(BitcoinMaxSendSolverUnitTest, NoInputs) {
  auto base_tx = MakeMockTransaction();

  BitcoinMaxSendSolver solver(base_tx, fee_rate(), {});

  // Can't send exactly what we have as we need to add some fee.
  EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
            solver.Solve().error());
}

TEST_F(BitcoinMaxSendSolverUnitTest, NotEnoughInputsForFee) {
  auto base_tx = MakeMockTransaction();

  // Fee for typical 1 input -> 1 output transaction.
  uint32_t min_fee = ApplyFeeRate(fee_rate(), std::ceil(109.25));

  {
    uint32_t total_input = min_fee;
    std::vector<BitcoinTransaction::TxInputGroup> input_groups;
    input_groups.emplace_back();
    input_groups.back().AddInput(MakeMockTxInput(total_input, 0));
    BitcoinMaxSendSolver solver(base_tx, fee_rate(), input_groups);

    // We have nothing left after fee is taken from inputs.
    EXPECT_EQ(l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE),
              solver.Solve().error());
  }

  {
    uint32_t total_input = min_fee + 1;
    std::vector<BitcoinTransaction::TxInputGroup> input_groups;
    input_groups.emplace_back();
    input_groups.back().AddInput(MakeMockTxInput(total_input, 0));
    BitcoinMaxSendSolver solver(base_tx, fee_rate(), input_groups);
    auto tx = solver.Solve();

    ASSERT_TRUE(tx.has_value());

    // We have 1 sat sent. Will not work when we support avoiding dust
    // outputs.
    EXPECT_EQ(tx->EffectiveFeeAmount(), min_fee);
    EXPECT_EQ(tx->TotalInputsAmount(), total_input);
    EXPECT_EQ(tx->TotalOutputsAmount(), 1u);
    EXPECT_EQ(tx->TargetOutput()->amount, 1u);
    EXPECT_FALSE(tx->ChangeOutput());
  }
}

TEST_F(BitcoinMaxSendSolverUnitTest, GroupIsSpentAsAWhole) {
  auto base_tx = MakeMockTransaction();

  std::vector<BitcoinTransaction::TxInputGroup> input_groups;
  input_groups.emplace_back();
  input_groups.back().AddInput(MakeMockTxInput(10000, 0));
  input_groups.back().AddInput(MakeMockTxInput(100000, 0));
  input_groups.back().AddInput(MakeMockTxInput(200000, 0));
  input_groups.back().AddInput(MakeMockTxInput(300000, 1));
  input_groups.back().AddInput(MakeMockTxInput(400000, 1));

  BitcoinMaxSendSolver solver(base_tx, fee_rate(), input_groups);

  // Large portion of inputs is spent to change as inputs group is not allowed
  // to be split.
  auto tx = solver.Solve();
  ASSERT_TRUE(tx.has_value());
  EXPECT_EQ(tx->amount(), 1005770u);

  EXPECT_EQ(tx->EffectiveFeeAmount(), 4230u);

  EXPECT_EQ(tx->TotalInputsAmount(), 1010000u);

  EXPECT_EQ(tx->TotalOutputsAmount(), 1005770u);
  EXPECT_EQ(tx->TargetOutput()->amount, 1005770u);
  EXPECT_FALSE(tx->ChangeOutput());

  EXPECT_THAT(tx.value().inputs(),
              UnorderedElementsAreArray(
                  {MakeMockTxInput(10000, 0), MakeMockTxInput(100000, 0),
                   MakeMockTxInput(200000, 0), MakeMockTxInput(300000, 1),
                   MakeMockTxInput(400000, 1)}));
}

TEST_F(BitcoinMaxSendSolverUnitTest, RandomTest) {
  std::vector<BitcoinTransaction::TxInputGroup> input_groups;

  uint64_t total_inputs = 0;

  for (int i = 0; i < 100; ++i) {
    if (base::RandInt(0, 10) == 0 || input_groups.empty()) {
      input_groups.emplace_back();
    }
    auto input =
        MakeMockTxInput(base::RandInt(0, 10000000), base::RandInt(0, 10));
    total_inputs += input.utxo_value;
    input_groups.back().AddInput(std::move(input));
  }

  auto base_tx = MakeMockTransaction(total_inputs / 2);

  BitcoinMaxSendSolver solver(base_tx, fee_rate(), input_groups);
  auto tx = solver.Solve();
  ASSERT_TRUE(tx.has_value());
}

}  // namespace brave_wallet
