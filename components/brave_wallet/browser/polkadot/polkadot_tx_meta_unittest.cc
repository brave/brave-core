// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_meta.h"

#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

// Use the BOB account here:
// https://westend.subscan.io/account/5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
inline constexpr const char kBob[] =
    "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48";

PolkadotTransaction MakePolkadotTx(bool transfer_all,
                                   std::optional<uint32_t> asset_id) {
  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  base::HexStringToSpan(kBob, pubkey);

  uint128_t send_amount = 1234;

  PolkadotTransaction tx;
  tx.set_amount(send_amount);
  tx.set_recipient(PolkadotAddress{pubkey, std::nullopt});
  tx.set_transfer_all(transfer_all);
  if (asset_id.has_value()) {
    tx.set_asset_id(*asset_id);
  }

  return tx;
}

std::unique_ptr<PolkadotTxMeta> MakePolkadotTxMeta(
    mojom::AccountIdPtr account_id,
    PolkadotTransaction tx) {
  auto meta = std::make_unique<PolkadotTxMeta>();
  meta->set_from(std::move(account_id));
  meta->set_tx(std::move(tx));
  meta->set_chain_id(mojom::kPolkadotTestnet);
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta->confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta->set_submitted_time(confirmed_time - base::Seconds(3));
  meta->set_created_time(confirmed_time - base::Minutes(1));

  return meta;
}

}  // namespace

TEST(PolkadotTxMeta, ToValue) {
  auto polkadot_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::DOT, mojom::KeyringId::kPolkadotTestnet,
      mojom::AccountKind::kDerived, 0);

  {
    std::string_view expected_value = R"(
    {
      "chain_id": "polkadot_testnet",
      "coin": 354,
      "confirmed_time": "0",
      "created_time": "11996733540000000",
      "from_account_id": "354_15_0_0",
      "id": "",
      "status": 0,
      "submitted_time": "11996733597000000",
      "tx_hash": "",
      "tx": {
        "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
        "amount": "d2040000000000000000000000000000",
        "fee": "00000000000000000000000000000000",
        "transfer_all": false
      }
    })";

    PolkadotTransaction tx =
        MakePolkadotTx(/*transfer_all=*/false, /*asset_id=*/std::nullopt);

    auto meta = MakePolkadotTxMeta(polkadot_account_id.Clone(), std::move(tx));
    EXPECT_EQ(meta->ToValue(), base::test::ParseJsonDict(expected_value));
  }

  {
    std::string_view expected_value = R"(
    {
      "chain_id": "polkadot_testnet",
      "coin": 354,
      "confirmed_time": "0",
      "created_time": "11996733540000000",
      "from_account_id": "354_15_0_0",
      "id": "",
      "status": 0,
      "submitted_time": "11996733597000000",
      "tx_hash": "",
      "tx": {
        "recipient": "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48",
        "amount": "d2040000000000000000000000000000",
        "fee": "00000000000000000000000000000000",
        "transfer_all": false,
        "asset_id": 1337
      }
    })";

    PolkadotTransaction tx =
        MakePolkadotTx(/*transfer_all=*/false, /*asset_id=*/1337);

    auto meta = MakePolkadotTxMeta(polkadot_account_id.Clone(), std::move(tx));
    EXPECT_EQ(meta->ToValue(), base::test::ParseJsonDict(expected_value));
  }
}

TEST(PolkadotTxMeta, ToTransactionPtr) {
  auto polkadot_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::DOT, mojom::KeyringId::kPolkadotTestnet,
      mojom::AccountKind::kDerived, 0);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  base::HexStringToSpan(kBob, pubkey);

  PolkadotTransaction tx =
      MakePolkadotTx(/*transfer_all=*/false, /*asset_id=*/std::nullopt);

  auto meta = MakePolkadotTxMeta(polkadot_account_id.Clone(), std::move(tx));

  {
    mojom::TransactionInfoPtr ti = meta->ToTransactionInfo();
    EXPECT_EQ(ti->id, meta->id());
    EXPECT_EQ(ti->chain_id, meta->chain_id());
    EXPECT_EQ(ti->from_account_id, polkadot_account_id);
    EXPECT_EQ(ti->tx_status, meta->status());
    EXPECT_TRUE(ti->tx_data_union->is_polkadot_tx_data());
    EXPECT_EQ(meta->created_time().InMillisecondsSinceUnixEpoch(),
              ti->created_time.InMilliseconds());
    EXPECT_EQ(meta->submitted_time().InMillisecondsSinceUnixEpoch(),
              ti->submitted_time.InMilliseconds());
    EXPECT_EQ(meta->confirmed_time().InMillisecondsSinceUnixEpoch(),
              ti->confirmed_time.InMilliseconds());

    const auto& tx_data = ti->tx_data_union->get_polkadot_tx_data();

    EXPECT_EQ(tx_data->to, "0x" + std::string(kBob));
    EXPECT_EQ(tx_data->amount, Uint128ToMojom(1234));
    EXPECT_EQ(tx_data->fee, Uint128ToMojom(0));
    EXPECT_FALSE(tx_data->sending_max_amount);
    EXPECT_FALSE(tx_data->asset_id.has_value());
  }

  meta = MakePolkadotTxMeta(polkadot_account_id->Clone(),
                            MakePolkadotTx(false, 1337));

  {
    mojom::TransactionInfoPtr ti = meta->ToTransactionInfo();
    const auto& tx_data = ti->tx_data_union->get_polkadot_tx_data();
    ASSERT_TRUE(tx_data->asset_id.has_value());
    EXPECT_EQ(tx_data->asset_id.value(), 1337u);
    EXPECT_FALSE(tx_data->sending_max_amount);
  }

  meta = MakePolkadotTxMeta(polkadot_account_id->Clone(),
                            MakePolkadotTx(true, 1337));

  {
    mojom::TransactionInfoPtr ti = meta->ToTransactionInfo();
    const auto& tx_data = ti->tx_data_union->get_polkadot_tx_data();
    ASSERT_TRUE(tx_data->asset_id.has_value());
    EXPECT_EQ(tx_data->asset_id.value(), 1337u);
    EXPECT_TRUE(tx_data->sending_max_amount);
  }
}

}  // namespace brave_wallet
