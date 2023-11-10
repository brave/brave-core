/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/common_utils.h"

#include "base/test/gtest_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAreArray;

namespace brave_wallet {

TEST(CommonUtils, IsFilecoinKeyringId) {
  EXPECT_TRUE(IsFilecoinKeyringId(mojom::KeyringId::kFilecoin));
  EXPECT_TRUE(IsFilecoinKeyringId(mojom::KeyringId::kFilecoinTestnet));

  EXPECT_FALSE(IsFilecoinKeyringId(mojom::KeyringId::kDefault));
  EXPECT_FALSE(IsFilecoinKeyringId(mojom::KeyringId::kSolana));
  EXPECT_FALSE(IsFilecoinKeyringId(mojom::KeyringId::kBitcoin84));
}

TEST(CommonUtils, IsBitcoinKeyring) {
  EXPECT_TRUE(IsBitcoinKeyring(mojom::KeyringId::kBitcoin84));
  EXPECT_TRUE(IsBitcoinKeyring(mojom::KeyringId::kBitcoin84Testnet));

  EXPECT_FALSE(IsBitcoinKeyring(mojom::KeyringId::kDefault));
  EXPECT_FALSE(IsBitcoinKeyring(mojom::KeyringId::kSolana));
  EXPECT_FALSE(IsBitcoinKeyring(mojom::KeyringId::kFilecoin));
}

TEST(CommonUtils, IsBitcoinMainnetKeyring) {
  EXPECT_TRUE(IsBitcoinMainnetKeyring(mojom::KeyringId::kBitcoin84));

  EXPECT_FALSE(IsBitcoinMainnetKeyring(mojom::KeyringId::kBitcoin84Testnet));

  EXPECT_FALSE(IsBitcoinMainnetKeyring(mojom::KeyringId::kDefault));
  EXPECT_FALSE(IsBitcoinMainnetKeyring(mojom::KeyringId::kSolana));
  EXPECT_FALSE(IsBitcoinMainnetKeyring(mojom::KeyringId::kFilecoin));
}

TEST(CommonUtils, IsBitcoinTestnetKeyring) {
  EXPECT_TRUE(IsBitcoinTestnetKeyring(mojom::KeyringId::kBitcoin84Testnet));

  EXPECT_FALSE(IsBitcoinTestnetKeyring(mojom::KeyringId::kBitcoin84));

  EXPECT_FALSE(IsBitcoinTestnetKeyring(mojom::KeyringId::kDefault));
  EXPECT_FALSE(IsBitcoinTestnetKeyring(mojom::KeyringId::kSolana));
  EXPECT_FALSE(IsBitcoinTestnetKeyring(mojom::KeyringId::kFilecoin));
}

TEST(CommonUtils, IsBitcoinNetwork) {
  EXPECT_TRUE(IsBitcoinNetwork(mojom::kBitcoinMainnet));
  EXPECT_TRUE(IsBitcoinNetwork(mojom::kBitcoinTestnet));

  EXPECT_FALSE(IsBitcoinNetwork(mojom::kMainnetChainId));
  EXPECT_FALSE(IsBitcoinNetwork(mojom::kFilecoinMainnet));
  EXPECT_FALSE(IsBitcoinNetwork(mojom::kSolanaMainnet));
  EXPECT_FALSE(IsBitcoinNetwork(""));
  EXPECT_FALSE(IsBitcoinNetwork("abc"));
}

TEST(CommonUtils, IsBitcoinAccount) {
  EXPECT_TRUE(IsBitcoinAccount(
      *MakeBitcoinAccountId(mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
                            mojom::AccountKind::kDerived, 4)));
  EXPECT_TRUE(IsBitcoinAccount(*MakeBitcoinAccountId(
      mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84Testnet,
      mojom::AccountKind::kDerived, 7)));

  EXPECT_FALSE(IsBitcoinAccount(
      *MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                     mojom::AccountKind::kDerived, "0xasdf")));
  EXPECT_FALSE(IsBitcoinAccount(
      *MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                     mojom::AccountKind::kDerived, "0xasdf")));
  EXPECT_FALSE(IsBitcoinAccount(
      *MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoin,
                     mojom::AccountKind::kImported, "0xasdf")));
}

TEST(CommonUtils, GetActiveEndpointUrl) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1();
  EXPECT_EQ(GURL("https://url1.com"), GetActiveEndpointUrl(chain));
  chain.active_rpc_endpoint_index = -1;
  EXPECT_EQ(GURL(), GetActiveEndpointUrl(chain));
  chain.active_rpc_endpoint_index = 1;
  EXPECT_EQ(GURL(), GetActiveEndpointUrl(chain));

  chain.active_rpc_endpoint_index = 2;
  chain.rpc_endpoints.emplace_back("https://brave.com");
  chain.rpc_endpoints.emplace_back("https://test.com");
  EXPECT_EQ(GURL("https://test.com"), GetActiveEndpointUrl(chain));

  chain.active_rpc_endpoint_index = 0;
  chain.rpc_endpoints.clear();
  EXPECT_EQ(GURL(), GetActiveEndpointUrl(chain));
}

TEST(CommonUtils, GetSupportedKeyrings) {
  base::test::ScopedFeatureList disabled_feature_list;
  const std::vector<base::test::FeatureRef> coin_features = {
      features::kBraveWalletBitcoinFeature};
  disabled_feature_list.InitWithFeatures({}, coin_features);

  uint32_t test_cases_count = (1 << coin_features.size());
  for (uint32_t test_case = 0; test_case < test_cases_count; ++test_case) {
    std::vector<base::test::FeatureRef> enabled_features;
    for (uint32_t feature = 0; feature < coin_features.size(); ++feature) {
      if ((1 << feature) & test_case) {
        enabled_features.emplace_back(coin_features[feature]);
      }
    }

    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeatures(enabled_features, {});

    auto keyrings = GetSupportedKeyrings();
    size_t last_pos = 0;

    EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kDefault);

    EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kFilecoin);
    EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kFilecoinTestnet);

    EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kSolana);

    if (IsBitcoinEnabled()) {
      EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kBitcoin84);
      EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kBitcoin84Testnet);
    }

    EXPECT_EQ(last_pos, keyrings.size());
  }
}

TEST(CommonUtils, GetSupportedKeyringsForNetwork) {
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::ETH,
                                             mojom::kMainnetChainId),
              ElementsAreArray({mojom::KeyringId::kDefault}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::ETH,
                                             mojom::kPolygonMainnetChainId),
              ElementsAreArray({mojom::KeyringId::kDefault}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::ETH,
                                             "any chain id allowed"),
              ElementsAreArray({mojom::KeyringId::kDefault}));

  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::SOL,
                                             mojom::kSolanaMainnet),
              ElementsAreArray({mojom::KeyringId::kSolana}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::SOL,
                                             mojom::kSolanaTestnet),
              ElementsAreArray({mojom::KeyringId::kSolana}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::SOL,
                                             "any chain id allowed"),
              ElementsAreArray({mojom::KeyringId::kSolana}));

  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::FIL,
                                             mojom::kFilecoinMainnet),
              ElementsAreArray({mojom::KeyringId::kFilecoin}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::FIL,
                                             mojom::kFilecoinTestnet),
              ElementsAreArray({mojom::KeyringId::kFilecoinTestnet}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::FIL,
                                             "any non mainnet chain"),
              ElementsAreArray({mojom::KeyringId::kFilecoinTestnet}));

  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::BTC,
                                             mojom::kBitcoinMainnet),
              ElementsAreArray({mojom::KeyringId::kBitcoin84}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::BTC,
                                             mojom::kBitcoinTestnet),
              ElementsAreArray({mojom::KeyringId::kBitcoin84Testnet}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::BTC,
                                             "any non mainnet chain"),
              ElementsAreArray({mojom::KeyringId::kBitcoin84Testnet}));

  EXPECT_TRUE(AllCoinsTested());
}

TEST(CommonUtils, MakeAccountId) {
  auto id = *MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                           mojom::AccountKind::kDerived,
                           "0xA0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");
  EXPECT_EQ(id.coin, mojom::CoinType::ETH);
  EXPECT_EQ(id.keyring_id, mojom::KeyringId::kDefault);
  EXPECT_EQ(id.kind, mojom::AccountKind::kDerived);
  EXPECT_EQ(id.address, "0xA0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");
  EXPECT_EQ(id.bitcoin_account_index, 0u);

  EXPECT_EQ(id.unique_key, "60_0_0_0xA0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");

  // Same AccountId
  EXPECT_EQ(
      id.unique_key,
      MakeAccountId(id.coin, id.keyring_id, id.kind, id.address)->unique_key);

  // Coin differs.
  EXPECT_NE(id.unique_key, MakeAccountId(mojom::CoinType::SOL, id.keyring_id,
                                         id.kind, id.address)
                               ->unique_key);

  // Keyring differs
  EXPECT_NE(id.unique_key, MakeAccountId(id.coin, mojom::KeyringId::kSolana,
                                         id.kind, id.address)
                               ->unique_key);

  // Kind differs
  EXPECT_NE(id.unique_key,
            MakeAccountId(id.coin, id.keyring_id, mojom::AccountKind::kImported,
                          id.address)
                ->unique_key);

  // Address differs
  EXPECT_NE(
      id.unique_key,
      MakeAccountId(id.coin, id.keyring_id, id.kind, "0x123")->unique_key);

  EXPECT_TRUE(MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                            mojom::AccountKind::kDerived, "0xabc"));
  EXPECT_TRUE(MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kDefault,
                            mojom::AccountKind::kDerived, "0xabc"));
  EXPECT_TRUE(MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kDefault,
                            mojom::AccountKind::kDerived, "0xabc"));
  EXPECT_DCHECK_DEATH(MakeAccountId(mojom::CoinType::BTC,
                                    mojom::KeyringId::kDefault,
                                    mojom::AccountKind::kDerived, "0xabc"));
  EXPECT_TRUE(AllCoinsTested());
}

TEST(CommonUtils, MakeBitcoinAccountId) {
  auto id =
      *MakeBitcoinAccountId(mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
                            mojom::AccountKind::kDerived, 123);
  EXPECT_EQ(id.coin, mojom::CoinType::BTC);
  EXPECT_EQ(id.keyring_id, mojom::KeyringId::kBitcoin84);
  EXPECT_EQ(id.kind, mojom::AccountKind::kDerived);
  EXPECT_EQ(id.address, "");
  EXPECT_EQ(id.bitcoin_account_index, 123u);
  EXPECT_EQ(id.unique_key, "0_4_0_123");

  // Same AccountId
  EXPECT_EQ(id.unique_key, MakeBitcoinAccountId(id.coin, id.keyring_id, id.kind,
                                                id.bitcoin_account_index)
                               ->unique_key);

  // Keyring differs
  EXPECT_NE(id.unique_key,
            MakeBitcoinAccountId(id.coin, mojom::KeyringId::kBitcoin84Testnet,
                                 id.kind, id.bitcoin_account_index)
                ->unique_key);

  // Index differs
  EXPECT_NE(
      id.unique_key,
      MakeBitcoinAccountId(id.coin, id.keyring_id, id.kind, 321)->unique_key);

  EXPECT_TRUE(MakeBitcoinAccountId(mojom::CoinType::BTC,
                                   mojom::KeyringId::kBitcoin84,
                                   mojom::AccountKind::kDerived, 123));
  // ETH is not a valid coin.
  EXPECT_DCHECK_DEATH(MakeBitcoinAccountId(mojom::CoinType::ETH,
                                           mojom::KeyringId::kBitcoin84,
                                           mojom::AccountKind::kDerived, 123));
  // SOL is not a valid coin.
  EXPECT_DCHECK_DEATH(MakeBitcoinAccountId(mojom::CoinType::SOL,
                                           mojom::KeyringId::kBitcoin84,
                                           mojom::AccountKind::kDerived, 123));
  // FIL is not a valid coin.
  EXPECT_DCHECK_DEATH(MakeBitcoinAccountId(mojom::CoinType::FIL,
                                           mojom::KeyringId::kBitcoin84,
                                           mojom::AccountKind::kDerived, 123));
  // kSolana is not a valid keyring.
  EXPECT_DCHECK_DEATH(MakeBitcoinAccountId(mojom::CoinType::BTC,
                                           mojom::KeyringId::kSolana,
                                           mojom::AccountKind::kDerived, 123));
  // kImported is not a valid kind.
  EXPECT_DCHECK_DEATH(MakeBitcoinAccountId(mojom::CoinType::BTC,
                                           mojom::KeyringId::kBitcoin84,
                                           mojom::AccountKind::kImported, 123));
  EXPECT_TRUE(AllCoinsTested());
}

TEST(CommonUtils, CoinSupportsDapps) {
  for (auto coin : kAllCoins) {
    if (coin == mojom::CoinType::ETH || coin == mojom::CoinType::SOL) {
      EXPECT_TRUE(CoinSupportsDapps(coin));
    } else {
      EXPECT_FALSE(CoinSupportsDapps(coin));
    }
  }
}

TEST(CommonUtils, GetNetworkForBitcoinKeyring) {
  EXPECT_EQ(mojom::kBitcoinMainnet,
            GetNetworkForBitcoinKeyring(mojom::KeyringId::kBitcoin84));
  EXPECT_EQ(mojom::kBitcoinTestnet,
            GetNetworkForBitcoinKeyring(mojom::KeyringId::kBitcoin84Testnet));
}

TEST(CommonUtils, GetNetworkForBitcoinAccount) {
  EXPECT_EQ(mojom::kBitcoinMainnet,
            GetNetworkForBitcoinAccount(MakeBitcoinAccountId(
                mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
                mojom::AccountKind::kDerived, 123)));
  EXPECT_EQ(mojom::kBitcoinTestnet,
            GetNetworkForBitcoinAccount(MakeBitcoinAccountId(
                mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84Testnet,
                mojom::AccountKind::kDerived, 123)));
}

}  // namespace brave_wallet
