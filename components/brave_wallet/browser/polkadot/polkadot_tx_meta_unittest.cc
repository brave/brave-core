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

inline constexpr const char kWestendChainType[] = "Westend";

}  // namespace

TEST(PolkadotTxMeta, ToValue) {
  auto polkadot_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::DOT, mojom::KeyringId::kPolkadotTestnet,
      mojom::AccountKind::kDerived, 0);

  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendChainType).value();

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  base::HexStringToSpan(kBob, pubkey);

  uint128_t send_amount = 1234;

  PolkadotUnsignedTransfer transfer_extrinsic(pubkey, send_amount);

  PolkadotTxMeta meta(polkadot_account_id, testnet_metadata,
                      transfer_extrinsic);

  meta.set_chain_id(mojom::kPolkadotTestnet);
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));

  std::string_view expected_value = R"(
    {
      "chain_id": "polkadot_testnet",
      "coin": 354,
      "confirmed_time": "0",
      "created_time": "11996733540000000",
      "extrinsic": "98040400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913",
      "from_account_id": "354_15_0_0",
      "id": "",
      "status": 0,
      "submitted_time": "11996733597000000",
      "tx_hash": ""
    })";

  EXPECT_EQ(meta.ToValue(), base::test::ParseJsonDict(expected_value));
}

TEST(PolkadotTxMeta, ToTransactionPtr) {
  auto polkadot_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::DOT, mojom::KeyringId::kPolkadotTestnet,
      mojom::AccountKind::kDerived, 0);

  auto testnet_metadata =
      PolkadotChainMetadata::FromChainName(kWestendChainType).value();

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  base::HexStringToSpan(kBob, pubkey);

  uint128_t send_amount = 1234;

  PolkadotUnsignedTransfer transfer_extrinsic(pubkey, send_amount);

  PolkadotTxMeta meta(polkadot_account_id, testnet_metadata,
                      transfer_extrinsic);

  meta.set_chain_id(mojom::kPolkadotTestnet);
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));

  mojom::TransactionInfoPtr ti = meta.ToTransactionInfo();
  EXPECT_EQ(ti->id, meta.id());
  EXPECT_EQ(ti->chain_id, meta.chain_id());
  EXPECT_EQ(ti->from_account_id, polkadot_account_id);
  EXPECT_EQ(ti->tx_status, meta.status());
  EXPECT_TRUE(ti->tx_data_union->is_polkadot_tx_data());
  EXPECT_EQ(meta.created_time().InMillisecondsSinceUnixEpoch(),
            ti->created_time.InMilliseconds());
  EXPECT_EQ(meta.submitted_time().InMillisecondsSinceUnixEpoch(),
            ti->submitted_time.InMilliseconds());
  EXPECT_EQ(meta.confirmed_time().InMillisecondsSinceUnixEpoch(),
            ti->confirmed_time.InMilliseconds());

  const auto& tx_data = ti->tx_data_union->get_polkadot_tx_data();

  EXPECT_EQ(tx_data->to, kBob);
  EXPECT_EQ(tx_data->amount, 1234u);
  EXPECT_EQ(tx_data->fee, 0u);
}

}  // namespace brave_wallet
