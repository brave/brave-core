/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_meta.h"

#include "base/values.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(FilTxMeta, ToTransactionInfo) {
  std::unique_ptr<FilTransaction> tx =
      std::make_unique<FilTransaction>(*FilTransaction::FromTxData(
          false, mojom::FilTxData::New(
                     "1", "2", "3", "4", "5",
                     "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6")));

  const char from_address[] = "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq";
  auto fil_account_id =
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoin,
                    mojom::AccountKind::kDerived, from_address);

  FilTxMeta meta(fil_account_id, std::move(tx));
  meta.set_chain_id("0x66");
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));

  mojom::TransactionInfoPtr ti = meta.ToTransactionInfo();
  EXPECT_EQ(ti->id, meta.id());
  EXPECT_EQ(ti->chain_id, meta.chain_id());
  EXPECT_EQ(ti->from_address, from_address);
  EXPECT_EQ(ti->from_account_id, fil_account_id);
  EXPECT_EQ(ti->tx_status, meta.status());
  EXPECT_TRUE(ti->tx_data_union->is_fil_tx_data());

  const auto& tx_data = ti->tx_data_union->get_fil_tx_data();
  EXPECT_EQ(tx_data->nonce, "1");
  EXPECT_EQ(tx_data->gas_premium, "2");
  EXPECT_EQ(tx_data->gas_fee_cap, "3");
  EXPECT_EQ(tx_data->gas_limit, "4");
  EXPECT_EQ(tx_data->max_fee, "5");
  EXPECT_EQ(tx_data->to, "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q");
  EXPECT_EQ(tx_data->value, "6");

  EXPECT_EQ(meta.created_time().ToJavaTime(),
            ti->created_time.InMilliseconds());
  EXPECT_EQ(meta.submitted_time().ToJavaTime(),
            ti->submitted_time.InMilliseconds());
  EXPECT_EQ(meta.confirmed_time().ToJavaTime(),
            ti->confirmed_time.InMilliseconds());
}

TEST(FilTxMeta, ToValue) {
  auto fil_account_id =
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoin,
                    mojom::AccountKind::kDerived,
                    "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq");

  auto transaction = FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "4", "5",
                            "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "6"));
  std::unique_ptr<FilTransaction> tx =
      std::make_unique<FilTransaction>(*transaction);
  FilTxMeta meta(fil_account_id, std::move(tx));
  auto root = meta.ToValue();
  auto* tx_node = root.FindDict("tx");
  ASSERT_TRUE(tx_node);
  EXPECT_EQ(transaction->ToValue(), *tx_node);
}

}  // namespace brave_wallet
