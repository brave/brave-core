/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"

#include <string>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_blockfrost_api.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Truly;

namespace brave_wallet::cardano_rpc {

namespace {
const constexpr char* kInvalidUint32Values[] = {"", "1.1", "-1", "a",
                                                "5000000000"};
const constexpr char* kInvalidUint64Values[] = {"", "1.1", "-1", "a"};
}  // namespace

TEST(CardanoRpcSchema, EpochParameters) {
  EXPECT_FALSE(EpochParameters::FromBlockfrostApiValue(std::nullopt));

  blockfrost_api::EpochParameters valid;
  valid.min_fee_a = "10";
  valid.min_fee_b = "20";

  EXPECT_EQ(
      *EpochParameters::FromBlockfrostApiValue(valid.Clone()),
      EpochParameters({.min_fee_coefficient = 10, .min_fee_constant = 20}));

  for (auto* value : kInvalidUint64Values) {
    auto invalid = valid.Clone();
    invalid.min_fee_a = value;
    EXPECT_FALSE(EpochParameters::FromBlockfrostApiValue(invalid.Clone()));
  }

  for (auto* value : kInvalidUint64Values) {
    auto invalid = valid.Clone();
    invalid.min_fee_b = value;
    EXPECT_FALSE(EpochParameters::FromBlockfrostApiValue(invalid.Clone()));
  }
}

TEST(CardanoRpcSchema, Block) {
  EXPECT_FALSE(Block::FromBlockfrostApiValue(std::nullopt));

  blockfrost_api::Block valid;
  valid.height = "10";
  valid.slot = "20";
  valid.epoch = "30";

  EXPECT_EQ(*Block::FromBlockfrostApiValue(valid.Clone()),
            Block({.height = 10, .slot = 20, .epoch = 30}));

  for (auto* value : kInvalidUint32Values) {
    auto invalid = valid.Clone();
    invalid.height = value;
    EXPECT_FALSE(Block::FromBlockfrostApiValue(invalid.Clone()));
  }

  for (auto* value : kInvalidUint64Values) {
    auto invalid = valid.Clone();
    invalid.slot = value;
    EXPECT_FALSE(Block::FromBlockfrostApiValue(invalid.Clone()));
  }

  for (auto* value : kInvalidUint32Values) {
    auto invalid = valid.Clone();
    invalid.epoch = value;
    EXPECT_FALSE(Block::FromBlockfrostApiValue(invalid.Clone()));
  }
}

TEST(CardanoRpcSchema, UnspentOutput) {
  EXPECT_FALSE(UnspentOutput::FromBlockfrostApiValue(std::nullopt));

  blockfrost_api::UnspentOutput valid;
  valid.tx_hash =
      "000102030405060708090a0b0c0d0f0e000102030405060708090a0b0c0d0f0e";
  valid.output_index = "123";
  valid.amount.emplace_back();
  valid.amount.back().quantity = "555";
  valid.amount.back().unit = "lovelace";

  auto converted = UnspentOutput::FromBlockfrostApiValue(valid.Clone());
  EXPECT_EQ(base::HexEncode(converted->tx_hash),
            "000102030405060708090A0B0C0D0F0E000102030405060708090A0B0C0D0F0E");
  EXPECT_EQ(converted->output_index, 123u);
  EXPECT_EQ(converted->lovelace_amount, 555u);

  valid.amount.emplace_back();
  valid.amount.back().quantity = "10000";
  valid.amount.back().unit = "lovelace";
  converted = UnspentOutput::FromBlockfrostApiValue(valid.Clone());
  // Still using first one.
  EXPECT_EQ(converted->lovelace_amount, 555u);

  valid.amount.front().unit = "some_token";
  converted = UnspentOutput::FromBlockfrostApiValue(valid.Clone());
  // Other token is ignored.
  EXPECT_EQ(converted->lovelace_amount, 10000u);

  valid.amount.clear();
  valid.amount.emplace_back();
  valid.amount.back().quantity = "10000";
  valid.amount.back().unit = "lovelace";

  for (auto* value :
       {"", "xx0102030405060708090a0b0c0d0f0e000102030405060708090a0b0c0d0f0e",
        "5000102030405060708090a0b0c0d0f0e000102030405060708090a0b0c0d0f0e"}) {
    auto invalid = valid.Clone();
    invalid.output_index = value;
    EXPECT_FALSE(UnspentOutput::FromBlockfrostApiValue(invalid.Clone()))
        << value;
  }

  for (auto* value : kInvalidUint32Values) {
    auto invalid = valid.Clone();
    invalid.output_index = value;
    EXPECT_FALSE(UnspentOutput::FromBlockfrostApiValue(invalid.Clone()))
        << value;
  }

  for (auto* value : kInvalidUint64Values) {
    auto invalid = valid.Clone();
    invalid.amount.front().quantity = value;
    EXPECT_FALSE(UnspentOutput::FromBlockfrostApiValue(invalid.Clone()))
        << value;
  }

  for (auto* value : {"", "some_token"}) {
    auto invalid = valid.Clone();
    invalid.amount.front().unit = value;
    EXPECT_FALSE(UnspentOutput::FromBlockfrostApiValue(invalid.Clone()))
        << value;
  }
}

}  // namespace brave_wallet::cardano_rpc
