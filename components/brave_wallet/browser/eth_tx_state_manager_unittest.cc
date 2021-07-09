/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"

#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

TEST(EthTxStateManagerUnitTest, GenerateMetaID) {
  EXPECT_NE(EthTxStateManager::GenerateMetaID(),
            EthTxStateManager::GenerateMetaID());
}

TEST(EthTxStateManagerUnitTest, TxMetaAndValye) {
  EthTransaction tx(EthTransaction::TxData(
      0x09, 0x4a817c800, 0x5208,
      EthAddress::FromHex("0x3535353535353535353535353535353535353535"),
      0x0de0b6b3a7640000, std::vector<uint8_t>()));

  EthTxStateManager::TxMeta meta(tx);
  meta.status = EthTxStateManager::TransactionStatus::SUBMITTED;
  meta.from = EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  meta.last_gas_price = 0x1234;
  meta.created_time = base::Time::Now();
  meta.submitted_time = base::Time::Now();
  meta.confirmed_time = base::Time::Now();

  TransactionReceipt tx_receipt;
  tx_receipt.transaction_hash =
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238";
  tx_receipt.transaction_index = 0x1;
  tx_receipt.block_number = 0xb;
  tx_receipt.block_hash =
      "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b";
  tx_receipt.cumulative_gas_used = 0x33bc;
  tx_receipt.gas_used = 0x4dc;
  tx_receipt.contract_address = "0xb60e8dd61c5d32be8058bb8eb970870f07233155";
  tx_receipt.status = true;

  meta.tx_receipt = tx_receipt;
  meta.tx_hash =
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238";

  base::Value meta_value = EthTxStateManager::TxMetaToValue(meta);
  auto meta_from_value = EthTxStateManager::ValueToTxMeta(meta_value);
  ASSERT_NE(meta_from_value, absl::nullopt);
  EXPECT_EQ(meta_from_value->status, meta.status);
  EXPECT_EQ(meta_from_value->from, meta.from);
  EXPECT_EQ(meta_from_value->last_gas_price, meta.last_gas_price);
  EXPECT_EQ(meta_from_value->created_time, meta.created_time);
  EXPECT_EQ(meta_from_value->submitted_time, meta.submitted_time);
  EXPECT_EQ(meta_from_value->confirmed_time, meta.confirmed_time);
  EXPECT_EQ(meta_from_value->tx_receipt, meta.tx_receipt);
  EXPECT_EQ(meta_from_value->tx_hash, meta.tx_hash);
  EXPECT_EQ(meta_from_value->tx, meta.tx);
}

}  // namespace brave_wallet
