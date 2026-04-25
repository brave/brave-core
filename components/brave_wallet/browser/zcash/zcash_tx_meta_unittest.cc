/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_tx_meta.h"

#include <optional>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(ZCashTxMeta, ToTransactionInfo_TransparentInputs) {
  auto zec_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet,
      mojom::AccountKind::kDerived, 0);

  std::unique_ptr<ZCashTransaction> tx = std::make_unique<ZCashTransaction>();

  tx->set_amount(15000u);
  tx->set_fee(10000u);
  tx->set_to("to");

  ZCashTransaction::TxInput input;
  input.utxo_address = "t1Hsc1LR8yKnbbe3twRp88p6vFfC5t7DLbs";
  input.utxo_value = 25000u;
  tx->transparent_part().inputs.push_back(std::move(input));

  ZCashTransaction::TxOutput output;
  output.address = "t1MmQ8PGfRygwhSK6qyianhMtb5tixuK8ZS";
  output.amount = 15000u;
  tx->transparent_part().outputs.push_back(std::move(output));

  ZCashTransaction::OrchardOutput orchard_output;
  orchard_output.addr = {0xce, 0xcb, 0xe5, 0xe6, 0x89, 0xa4, 0x53, 0xa3, 0xfe,
                         0x10, 0xcc, 0xf7, 0x61, 0x7e, 0x6c, 0x1f, 0xb3, 0x82,
                         0x81, 0x9d, 0x7f, 0xc9, 0x20, 0x0a, 0x1f, 0x42, 0x09,
                         0x2a, 0xc8, 0x4a, 0x30, 0x37, 0x8f, 0x8c, 0x1f, 0xb9,
                         0x0d, 0xff, 0x71, 0xa6, 0xd5, 0x04, 0x2d};
  orchard_output.value = 10000u;
  tx->orchard_part().outputs.push_back(std::move(orchard_output));

  ZCashTxMeta meta(zec_account_id, std::move(tx));
  meta.set_chain_id(mojom::kZCashMainnet);
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));

  mojom::TransactionInfoPtr ti = meta.ToTransactionInfo();
  EXPECT_EQ(ti->id, meta.id());
  EXPECT_EQ(ti->chain_id, meta.chain_id());
  EXPECT_EQ(ti->from_account_id, zec_account_id);
  EXPECT_EQ(ti->tx_status, meta.status());
  EXPECT_TRUE(ti->tx_data_union->is_zec_tx_data());
  EXPECT_EQ(meta.created_time().InMillisecondsSinceUnixEpoch(),
            ti->created_time.InMilliseconds());
  EXPECT_EQ(meta.submitted_time().InMillisecondsSinceUnixEpoch(),
            ti->submitted_time.InMilliseconds());
  EXPECT_EQ(meta.confirmed_time().InMillisecondsSinceUnixEpoch(),
            ti->confirmed_time.InMilliseconds());

  const auto& tx_data = ti->tx_data_union->get_zec_tx_data();

  EXPECT_EQ(tx_data->to, "to");

  EXPECT_EQ(tx_data->amount, 15000u);
  EXPECT_EQ(tx_data->fee, 10000u);

  EXPECT_EQ(tx_data->inputs.size(), 1u);
  EXPECT_EQ(tx_data->inputs[0]->address, "t1Hsc1LR8yKnbbe3twRp88p6vFfC5t7DLbs");
  EXPECT_EQ(tx_data->inputs[0]->value, 25000u);

  EXPECT_EQ(tx_data->outputs.size(), 2u);

  EXPECT_EQ(tx_data->outputs[0]->address,
            "t1MmQ8PGfRygwhSK6qyianhMtb5tixuK8ZS");
  EXPECT_EQ(tx_data->outputs[0]->value, 15000u);

  EXPECT_EQ(
      tx_data->outputs[1]->address,
      GetOrchardUnifiedAddress(
          {0xce, 0xcb, 0xe5, 0xe6, 0x89, 0xa4, 0x53, 0xa3, 0xfe, 0x10, 0xcc,
           0xf7, 0x61, 0x7e, 0x6c, 0x1f, 0xb3, 0x82, 0x81, 0x9d, 0x7f, 0xc9,
           0x20, 0x0a, 0x1f, 0x42, 0x09, 0x2a, 0xc8, 0x4a, 0x30, 0x37, 0x8f,
           0x8c, 0x1f, 0xb9, 0x0d, 0xff, 0x71, 0xa6, 0xd5, 0x04, 0x2d},
          false)
          .value());
  EXPECT_EQ(tx_data->outputs[1]->value, 10000u);
}

TEST(ZCashTxMeta, ToTransactionInfo_ShieldedInputs) {
  auto zec_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet,
      mojom::AccountKind::kDerived, 0);

  std::unique_ptr<ZCashTransaction> tx = std::make_unique<ZCashTransaction>();

  tx->set_amount(15000u);
  tx->set_fee(10000u);
  tx->set_to("to");

  ZCashTransaction::OrchardInput orchard_input;
  orchard_input.note.addr = {
      0xe3, 0x40, 0x63, 0x65, 0x42, 0xec, 0xe1, 0xc8, 0x12, 0x85, 0xed,
      0x4e, 0xab, 0x44, 0x8a, 0xdb, 0xb5, 0xa8, 0xc0, 0xf4, 0xd3, 0x86,
      0xee, 0xff, 0x33, 0x7e, 0x88, 0xe6, 0x91, 0x5f, 0x6c, 0x3e, 0xc1,
      0xb6, 0xea, 0x83, 0x5a, 0x88, 0xd5, 0x66, 0x12, 0xd2, 0xbd};
  orchard_input.note.amount = 25000u;
  tx->orchard_part().inputs.push_back(std::move(orchard_input));

  ZCashTransaction::TxOutput output;
  output.address = "t1MmQ8PGfRygwhSK6qyianhMtb5tixuK8ZS";
  output.amount = 15000u;
  tx->transparent_part().outputs.push_back(std::move(output));

  ZCashTransaction::OrchardOutput orchard_output;
  orchard_output.addr = {0xce, 0xcb, 0xe5, 0xe6, 0x89, 0xa4, 0x53, 0xa3, 0xfe,
                         0x10, 0xcc, 0xf7, 0x61, 0x7e, 0x6c, 0x1f, 0xb3, 0x82,
                         0x81, 0x9d, 0x7f, 0xc9, 0x20, 0x0a, 0x1f, 0x42, 0x09,
                         0x2a, 0xc8, 0x4a, 0x30, 0x37, 0x8f, 0x8c, 0x1f, 0xb9,
                         0x0d, 0xff, 0x71, 0xa6, 0xd5, 0x04, 0x2d};
  orchard_output.value = 10000u;
  tx->orchard_part().outputs.push_back(std::move(orchard_output));

  ZCashTxMeta meta(zec_account_id, std::move(tx));
  meta.set_chain_id(mojom::kZCashMainnet);
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));

  mojom::TransactionInfoPtr ti = meta.ToTransactionInfo();
  EXPECT_EQ(ti->id, meta.id());
  EXPECT_EQ(ti->chain_id, meta.chain_id());
  EXPECT_EQ(ti->from_account_id, zec_account_id);
  EXPECT_EQ(ti->tx_status, meta.status());
  EXPECT_TRUE(ti->tx_data_union->is_zec_tx_data());
  EXPECT_EQ(meta.created_time().InMillisecondsSinceUnixEpoch(),
            ti->created_time.InMilliseconds());
  EXPECT_EQ(meta.submitted_time().InMillisecondsSinceUnixEpoch(),
            ti->submitted_time.InMilliseconds());
  EXPECT_EQ(meta.confirmed_time().InMillisecondsSinceUnixEpoch(),
            ti->confirmed_time.InMilliseconds());

  const auto& tx_data = ti->tx_data_union->get_zec_tx_data();

  EXPECT_EQ(tx_data->to, "to");

  EXPECT_EQ(tx_data->amount, 15000u);
  EXPECT_EQ(tx_data->fee, 10000u);

  EXPECT_EQ(tx_data->inputs.size(), 1u);
  EXPECT_EQ(
      tx_data->inputs[0]->address,
      GetOrchardUnifiedAddress(
          {0xe3, 0x40, 0x63, 0x65, 0x42, 0xec, 0xe1, 0xc8, 0x12, 0x85, 0xed,
           0x4e, 0xab, 0x44, 0x8a, 0xdb, 0xb5, 0xa8, 0xc0, 0xf4, 0xd3, 0x86,
           0xee, 0xff, 0x33, 0x7e, 0x88, 0xe6, 0x91, 0x5f, 0x6c, 0x3e, 0xc1,
           0xb6, 0xea, 0x83, 0x5a, 0x88, 0xd5, 0x66, 0x12, 0xd2, 0xbd},
          false));
  EXPECT_EQ(tx_data->inputs[0]->value, 25000u);

  EXPECT_EQ(tx_data->outputs.size(), 2u);
  EXPECT_EQ(tx_data->outputs[0]->address,
            "t1MmQ8PGfRygwhSK6qyianhMtb5tixuK8ZS");
  EXPECT_EQ(tx_data->outputs[0]->value, 15000u);

  EXPECT_EQ(
      tx_data->outputs[1]->address,
      GetOrchardUnifiedAddress(
          {0xce, 0xcb, 0xe5, 0xe6, 0x89, 0xa4, 0x53, 0xa3, 0xfe, 0x10, 0xcc,
           0xf7, 0x61, 0x7e, 0x6c, 0x1f, 0xb3, 0x82, 0x81, 0x9d, 0x7f, 0xc9,
           0x20, 0x0a, 0x1f, 0x42, 0x09, 0x2a, 0xc8, 0x4a, 0x30, 0x37, 0x8f,
           0x8c, 0x1f, 0xb9, 0x0d, 0xff, 0x71, 0xa6, 0xd5, 0x04, 0x2d},
          false));
  EXPECT_EQ(tx_data->outputs[1]->value, 10000u);
}

}  // namespace brave_wallet
