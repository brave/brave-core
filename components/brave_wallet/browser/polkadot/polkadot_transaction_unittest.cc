/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_transaction.h"

#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(PolkadotTransaction, JsonSerde) {
  PolkadotTransaction polkadot_tx;

  EXPECT_EQ(
      polkadot_tx.recipient().ToString(),
      "0x0000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(polkadot_tx.amount(), uint128_t{0});
  EXPECT_EQ(polkadot_tx.fee(), uint128_t{0});
  EXPECT_EQ(polkadot_tx.transfer_all(), false);

  const char empty_tx_json[] = R"({
    "amount": "00000000000000000000000000000000",
    "fee": "00000000000000000000000000000000",
    "recipient": "0000000000000000000000000000000000000000000000000000000000000000",
    "transfer_all": false
  })";

  EXPECT_EQ(base::test::ParseJsonDict(empty_tx_json), polkadot_tx.ToValue());

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(
      "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
      pubkey));

  polkadot_tx.set_amount(uint128_t{12341234123412341234u});
  polkadot_tx.set_fee(uint128_t{15937408476u});
  polkadot_tx.set_recipient(PolkadotAddress{pubkey, 0});
  polkadot_tx.set_transfer_all(true);

  const char tx_json[] = R"({
    "amount": "f201ec6f0cdf44ab0000000000000000",
    "fee": "dc8df1b5030000000000000000000000",
    "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
    "ss58_prefix": 0,
    "transfer_all": true
  })";

  EXPECT_EQ(base::test::ParseJsonDict(tx_json), polkadot_tx.ToValue());

  auto tx = PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json));
  EXPECT_TRUE(tx);

  EXPECT_EQ(tx->amount(), uint128_t{12341234123412341234u});
  EXPECT_EQ(tx->fee(), uint128_t{15937408476u});
  EXPECT_EQ(tx->recipient().ToString(),
            "14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3");
  EXPECT_EQ(tx->transfer_all(), true);
}

}  // namespace brave_wallet
