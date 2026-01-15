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
  // Test serializing a default-constructed transaction.

  PolkadotTransaction polkadot_tx;

  EXPECT_EQ(
      polkadot_tx.recipient().ToString(),
      "0x0000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(polkadot_tx.amount(), uint128_t{0});
  EXPECT_EQ(polkadot_tx.fee(), uint128_t{0});
  EXPECT_EQ(polkadot_tx.transfer_all(), false);
  EXPECT_EQ(polkadot_tx.extrinsic_metadata(), std::nullopt);

  const char empty_tx_json[] = R"({
    "amount": "00000000000000000000000000000000",
    "fee": "00000000000000000000000000000000",
    "recipient": "0000000000000000000000000000000000000000000000000000000000000000",
    "transfer_all": false
  })";

  EXPECT_EQ(base::test::ParseJsonDict(empty_tx_json), polkadot_tx.ToValue());

  // Test (de)serializing a transaction with non-empty data. This would
  // represent a saved unapproved transaction.

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(
      "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
      pubkey));

  polkadot_tx.set_amount(uint128_t{12341234123412341234u});
  polkadot_tx.set_fee(uint128_t{15937408476u});
  polkadot_tx.set_recipient(PolkadotAddress{pubkey, 0});
  polkadot_tx.set_transfer_all(true);

  const char tx_json_no_metadata[] = R"({
    "amount": "f201ec6f0cdf44ab0000000000000000",
    "fee": "dc8df1b5030000000000000000000000",
    "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
    "ss58_prefix": 0,
    "transfer_all": true
  })";

  EXPECT_EQ(base::test::ParseJsonDict(tx_json_no_metadata),
            polkadot_tx.ToValue());

  auto tx = PolkadotTransaction::FromValue(
      base::test::ParseJsonDict(tx_json_no_metadata));
  ASSERT_TRUE(tx);

  EXPECT_EQ(tx->amount(), uint128_t{12341234123412341234u});
  EXPECT_EQ(tx->fee(), uint128_t{15937408476u});
  EXPECT_EQ(tx->recipient().ToString(),
            "14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3");
  EXPECT_EQ(tx->transfer_all(), true);
  EXPECT_EQ(tx->extrinsic_metadata(), std::nullopt);

  // Test (de)serializing a transaction that has extrinsic metadata attached to
  // it. This would represent a signed transaction that's been submitted and
  // accepted by the block chain.

  const char tx_json[] = R"({
    "amount": "f201ec6f0cdf44ab0000000000000000",
    "fee": "dc8df1b5030000000000000000000000",
    "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
    "ss58_prefix": 0,
    "transfer_all": true,
    "extrinsic_metadata": {
      "block_hash": "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd",
      "block_num": "015cbc00",
      "extrinsic": "3502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01acbcd3a50ad752305ce13def0c09c40a77a5dc5c9ba8e0443b8327560d1745380b45bb1b7fc68727de7c72865a6cef2d065b4d8572e572348c249854668d838ef5036c00000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a488d72",
      "mortality_period": "20000000"
    }
  })";

  std::array<uint8_t, kPolkadotBlockHashSize> block_hash = {};
  block_hash.fill(uint8_t{0xcd});

  std::vector<uint8_t> extrinsic;
  EXPECT_TRUE(base::HexStringToBytes(
      R"(3502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01acbcd3a50ad752305ce13def0c09c40a77a5dc5c9ba8e0443b8327560d1745380b45bb1b7fc68727de7c72865a6cef2d065b4d8572e572348c249854668d838ef5036c00000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a488d72)",
      &extrinsic));

  PolkadotExtrinsicMetadata metadata;
  metadata.set_block_hash(block_hash);
  metadata.set_block_num(12344321);
  metadata.set_extrinsic(extrinsic);
  metadata.set_mortality_period(32);
  polkadot_tx.set_extrinsic_metadata(std::move(metadata));

  EXPECT_EQ(base::test::ParseJsonDict(tx_json), polkadot_tx.ToValue());

  auto tx2 = PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json));
  ASSERT_TRUE(tx2);

  EXPECT_EQ(tx2->amount(), uint128_t{12341234123412341234u});
  EXPECT_EQ(tx2->fee(), uint128_t{15937408476u});
  EXPECT_EQ(tx2->recipient().ToString(),
            "14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3");
  EXPECT_EQ(tx2->transfer_all(), true);

  EXPECT_EQ(tx2->extrinsic_metadata()->block_hash(), block_hash);
  EXPECT_EQ(tx2->extrinsic_metadata()->block_num(), 12344321u);
  EXPECT_EQ(tx2->extrinsic_metadata()->extrinsic(), extrinsic);
  EXPECT_EQ(tx2->extrinsic_metadata()->mortality_period(), 32u);
}

TEST(PolkadotTransaction, FromValue) {
  {
    // Working/default sane case.

    const char tx_json[] = R"({
      "amount": "f201ec6f0cdf44ab0000000000000000",
      "fee": "dc8df1b5030000000000000000000000",
      "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
      "ss58_prefix": 0,
      "transfer_all": true
    })";

    EXPECT_TRUE(
        PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json)));
  }

  {
    // Amount too small

    const char tx_json[] = R"({
      "amount": "f201ec6f0cdf44ab000000000000",
      "fee": "dc8df1b5030000000000000000000000",
      "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
      "ss58_prefix": 0,
      "transfer_all": true
    })";

    EXPECT_FALSE(
        PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json)));
  }

  {
    // Amount too large.

    const char tx_json[] = R"({
      "amount": "f201ec6f0cdf44ab00000000000000001234",
      "fee": "dc8df1b5030000000000000000000000",
      "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
      "ss58_prefix": 0,
      "transfer_all": true
    })";

    EXPECT_FALSE(
        PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json)));
  }

  {
    // Fee too small.

    const char tx_json[] = R"({
      "amount": "f201ec6f0cdf44ab0000000000000000",
      "fee": "dc8df1b5030000000000000000000",
      "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
      "ss58_prefix": 0,
      "transfer_all": true
    })";

    EXPECT_FALSE(
        PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json)));
  }

  {
    // Fee too large

    const char tx_json[] = R"({
      "amount": "f201ec6f0cdf44ab0000000000000000",
      "fee": "dc8df1b50300000000000000000000001234",
      "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
      "ss58_prefix": 0,
      "transfer_all": true
    })";

    EXPECT_FALSE(
        PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json)));
  }

  {
    // Recipient contains non-hex

    const char tx_json[] = R"({
      "amount": "f201ec6f0cdf44ab0000000000000000",
      "fee": "dc8df1b5030000000000000000000000",
      "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4z",
      "ss58_prefix": 0,
      "transfer_all": true
    })";

    EXPECT_FALSE(
        PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json)));
  }

  {
    // Recipient too short.

    const char tx_json[] = R"({
      "amount": "f201ec6f0cdf44ab0000000000000000",
      "fee": "dc8df1b5030000000000000000000000",
      "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4",
      "ss58_prefix": 0,
      "transfer_all": true
    })";

    EXPECT_FALSE(
        PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json)));
  }

  {
    // Recipient too long.

    const char tx_json[] = R"({
      "amount": "f201ec6f0cdf44ab0000000000000000",
      "fee": "dc8df1b5030000000000000000000000",
      "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a481",
      "ss58_prefix": 0,
      "transfer_all": true
    })";

    EXPECT_FALSE(
        PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json)));
  }

  {
    // ss58_prefix exceeds uint16_t limits.

    const char tx_json[] = R"({
      "amount": "f201ec6f0cdf44ab0000000000000000",
      "fee": "dc8df1b5030000000000000000000000",
      "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
      "ss58_prefix": 123412341234123412341234,
      "transfer_all": true
    })";

    EXPECT_FALSE(
        PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json)));
  }

  {
    // transfer_all is non-boolean.

    const char tx_json[] = R"({
      "amount": "f201ec6f0cdf44ab0000000000000000",
      "fee": "dc8df1b5030000000000000000000000",
      "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
      "ss58_prefix": 0,
      "transfer_all": "hello, world"
    })";

    EXPECT_FALSE(
        PolkadotTransaction::FromValue(base::test::ParseJsonDict(tx_json)));
  }
}

}  // namespace brave_wallet
