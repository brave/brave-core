/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_meta.h"

#include <optional>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(BitcoinTxMeta, ToTransactionInfo) {
  auto btc_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
      mojom::AccountKind::kDerived, 1);

  std::unique_ptr<BitcoinTransaction> tx =
      std::make_unique<BitcoinTransaction>();
  tx->set_amount(200000);
  tx->set_to("tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm");
  BitcoinTransaction::TxInput input;
  input.utxo_address = "tb1q56kslnp386v43wpp6wkpx072ryud5gu865efx8";
  input.utxo_value = 200000;
  tx->AddInput(std::move(input));

  BitcoinTransaction::TxOutput output;
  output.address = "tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm";
  output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
      "tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm", true);
  output.amount = 200000 - 1000;
  tx->AddOutput(std::move(output));

  BitcoinTxMeta meta(btc_account_id, std::move(tx));
  meta.set_chain_id(mojom::kBitcoinTestnet);
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));

  mojom::TransactionInfoPtr ti = meta.ToTransactionInfo();
  EXPECT_EQ(ti->id, meta.id());
  EXPECT_EQ(ti->chain_id, meta.chain_id());
  EXPECT_EQ(ti->from_address, std::nullopt);
  EXPECT_EQ(ti->from_account_id, btc_account_id);
  EXPECT_EQ(ti->tx_status, meta.status());
  EXPECT_TRUE(ti->tx_data_union->is_btc_tx_data());
  EXPECT_EQ(meta.created_time().InMillisecondsSinceUnixEpoch(),
            ti->created_time.InMilliseconds());
  EXPECT_EQ(meta.submitted_time().InMillisecondsSinceUnixEpoch(),
            ti->submitted_time.InMilliseconds());
  EXPECT_EQ(meta.confirmed_time().InMillisecondsSinceUnixEpoch(),
            ti->confirmed_time.InMilliseconds());

  const auto& tx_data = ti->tx_data_union->get_btc_tx_data();

  EXPECT_EQ(tx_data->to, "tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm");
  EXPECT_EQ(tx_data->amount, 200000ULL);
  EXPECT_EQ(tx_data->fee, 1000ULL);
  EXPECT_EQ(tx_data->inputs.size(), 1u);
  EXPECT_EQ(tx_data->inputs[0]->address,
            "tb1q56kslnp386v43wpp6wkpx072ryud5gu865efx8");
  EXPECT_EQ(tx_data->inputs[0]->value, 200000ULL);
  EXPECT_EQ(tx_data->outputs.size(), 1u);
  EXPECT_EQ(tx_data->outputs[0]->address,
            "tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm");
  EXPECT_EQ(tx_data->outputs[0]->value, 200000ULL - 1000);
}

TEST(BitcoinTxMeta, ToValue) {
  auto btc_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
      mojom::AccountKind::kDerived, 1);

  std::unique_ptr<BitcoinTransaction> tx =
      std::make_unique<BitcoinTransaction>();
  tx->set_amount(200000);
  tx->set_to("tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm");

  BitcoinTransaction::TxInput input;
  input.utxo_address = "tb1q56kslnp386v43wpp6wkpx072ryud5gu865efx8";
  input.utxo_value = 200000;
  tx->AddInput(std::move(input));

  BitcoinTransaction::TxOutput output;
  output.address = "tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm";
  output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
      "tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm", true);
  output.amount = 200000 - 1000;
  tx->AddOutput(std::move(output));
  auto tx_value = tx->ToValue();

  BitcoinTxMeta meta(btc_account_id, std::move(tx));
  auto root = meta.ToValue();
  auto* tx_node = root.FindDict("tx");
  ASSERT_TRUE(tx_node);
  EXPECT_EQ(tx_value, *tx_node);
}

}  // namespace brave_wallet
