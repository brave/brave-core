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

TEST(CommonUtils, IsEthereumKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kDefault) {
      EXPECT_TRUE(IsEthereumKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsEthereumKeyring(keyring_id));
    }
  }
}
TEST(CommonUtils, IsEthereumAccount) {
  EXPECT_FALSE(IsEthereumAccount(nullptr));

  EXPECT_FALSE(IsEthereumAccount(MakeIndexBasedAccountId(
      mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
      mojom::AccountKind::kDerived, 4)));
  EXPECT_TRUE(IsEthereumAccount(
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived, "addr")));
  EXPECT_TRUE(IsEthereumAccount(
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kImported, "addr")));
}

TEST(CommonUtils, IsSolanaKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kSolana) {
      EXPECT_TRUE(IsSolanaKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsSolanaKeyring(keyring_id));
    }
  }
}
TEST(CommonUtils, IsSolanaAccount) {
  EXPECT_FALSE(IsSolanaAccount(nullptr));

  EXPECT_FALSE(IsSolanaAccount(MakeIndexBasedAccountId(
      mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
      mojom::AccountKind::kDerived, 4)));
  EXPECT_TRUE(IsSolanaAccount(
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived, "addr")));
  EXPECT_TRUE(IsSolanaAccount(
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kImported, "addr")));
}

TEST(CommonUtils, IsFilecoinKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kFilecoin ||
        keyring_id == mojom::KeyringId::kFilecoinTestnet) {
      EXPECT_TRUE(IsFilecoinKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsFilecoinKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsFilecoinAccount) {
  EXPECT_FALSE(IsFilecoinAccount(nullptr));

  EXPECT_FALSE(IsFilecoinAccount(MakeIndexBasedAccountId(
      mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
      mojom::AccountKind::kDerived, 4)));

  EXPECT_TRUE(IsFilecoinAccount(
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoin,
                    mojom::AccountKind::kDerived, "addr")));
  EXPECT_TRUE(IsFilecoinAccount(
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoinTestnet,
                    mojom::AccountKind::kDerived, "addr")));
  EXPECT_TRUE(IsFilecoinAccount(
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoin,
                    mojom::AccountKind::kImported, "addr")));
  EXPECT_TRUE(IsFilecoinAccount(
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoinTestnet,
                    mojom::AccountKind::kDerived, "addr")));
  EXPECT_TRUE(IsFilecoinAccount(
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoinTestnet,
                    mojom::AccountKind::kImported, "addr")));
}

TEST(CommonUtils, IsBitcoinKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kBitcoin84 ||
        keyring_id == mojom::KeyringId::kBitcoin84Testnet ||
        keyring_id == mojom::KeyringId::kBitcoinImport ||
        keyring_id == mojom::KeyringId::kBitcoinImportTestnet ||
        keyring_id == mojom::KeyringId::kBitcoinHardware ||
        keyring_id == mojom::KeyringId::kBitcoinHardwareTestnet) {
      EXPECT_TRUE(IsBitcoinKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsBitcoinKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsBitcoinHDKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kBitcoin84 ||
        keyring_id == mojom::KeyringId::kBitcoin84Testnet) {
      EXPECT_TRUE(IsBitcoinHDKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsBitcoinHDKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsBitcoinImportKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kBitcoinImport ||
        keyring_id == mojom::KeyringId::kBitcoinImportTestnet) {
      EXPECT_TRUE(IsBitcoinImportKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsBitcoinImportKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsBitcoinHardwareKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kBitcoinHardware ||
        keyring_id == mojom::KeyringId::kBitcoinHardwareTestnet) {
      EXPECT_TRUE(IsBitcoinHardwareKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsBitcoinHardwareKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsBitcoinMainnetKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kBitcoin84 ||
        keyring_id == mojom::KeyringId::kBitcoinImport ||
        keyring_id == mojom::KeyringId::kBitcoinHardware) {
      EXPECT_TRUE(IsBitcoinMainnetKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsBitcoinMainnetKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsBitcoinTestnetKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kBitcoin84Testnet ||
        keyring_id == mojom::KeyringId::kBitcoinImportTestnet ||
        keyring_id == mojom::KeyringId::kBitcoinHardwareTestnet) {
      EXPECT_TRUE(IsBitcoinTestnetKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsBitcoinTestnetKeyring(keyring_id));
    }
  }
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
  EXPECT_FALSE(IsBitcoinAccount(nullptr));

  EXPECT_TRUE(IsBitcoinAccount(MakeIndexBasedAccountId(
      mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
      mojom::AccountKind::kDerived, 4)));
  EXPECT_TRUE(IsBitcoinAccount(MakeIndexBasedAccountId(
      mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84Testnet,
      mojom::AccountKind::kDerived, 7)));

  EXPECT_FALSE(IsBitcoinAccount(
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived, "0xasdf")));
  EXPECT_FALSE(IsBitcoinAccount(
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived, "0xasdf")));
  EXPECT_FALSE(IsBitcoinAccount(
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoin,
                    mojom::AccountKind::kImported, "0xasdf")));
}

TEST(CommonUtils, IsZCashKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kZCashMainnet ||
        keyring_id == mojom::KeyringId::kZCashTestnet) {
      EXPECT_TRUE(IsZCashKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsZCashKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsZCashMainnetKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kZCashMainnet) {
      EXPECT_TRUE(IsZCashMainnetKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsZCashMainnetKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsZCashTestnetKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kZCashTestnet) {
      EXPECT_TRUE(IsZCashTestnetKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsZCashTestnetKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsZCashNetwork) {
  EXPECT_TRUE(IsZCashNetwork(mojom::kZCashMainnet));
  EXPECT_TRUE(IsZCashNetwork(mojom::kZCashTestnet));

  EXPECT_FALSE(IsZCashNetwork(mojom::kMainnetChainId));
  EXPECT_FALSE(IsZCashNetwork(mojom::kFilecoinMainnet));
  EXPECT_FALSE(IsZCashNetwork(mojom::kSolanaMainnet));
  EXPECT_FALSE(IsZCashNetwork(""));
  EXPECT_FALSE(IsZCashNetwork("abc"));
}

TEST(CommonUtils, IsZCashAccount) {
  EXPECT_FALSE(IsZCashAccount(nullptr));

  EXPECT_TRUE(IsZCashAccount(MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet,
      mojom::AccountKind::kDerived, 4)));
  EXPECT_TRUE(IsZCashAccount(MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashTestnet,
      mojom::AccountKind::kDerived, 7)));

  EXPECT_FALSE(IsZCashAccount(
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived, "0xasdf")));
  EXPECT_FALSE(IsZCashAccount(
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived, "0xasdf")));
  EXPECT_FALSE(IsZCashAccount(
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoin,
                    mojom::AccountKind::kImported, "0xasdf")));
}

TEST(CommonUtils, IsCardanoKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kCardanoMainnet ||
        keyring_id == mojom::KeyringId::kCardanoTestnet) {
      EXPECT_TRUE(IsCardanoKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsCardanoKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsCardanoHDKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kCardanoMainnet ||
        keyring_id == mojom::KeyringId::kCardanoTestnet) {
      EXPECT_TRUE(IsCardanoHDKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsCardanoHDKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsCardanoImportKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    EXPECT_FALSE(IsCardanoImportKeyring(keyring_id));
  }
}

TEST(CommonUtils, IsCardanoHardwareKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    EXPECT_FALSE(IsCardanoHardwareKeyring(keyring_id));
  }
}

TEST(CommonUtils, IsCardanoMainnetKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kCardanoMainnet) {
      EXPECT_TRUE(IsCardanoMainnetKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsCardanoMainnetKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsCardanoTestnetKeyring) {
  for (const auto& keyring_id : kAllKeyrings) {
    if (keyring_id == mojom::KeyringId::kCardanoTestnet) {
      EXPECT_TRUE(IsCardanoTestnetKeyring(keyring_id));
    } else {
      EXPECT_FALSE(IsCardanoTestnetKeyring(keyring_id));
    }
  }
}

TEST(CommonUtils, IsCardanoNetwork) {
  EXPECT_TRUE(IsCardanoNetwork(mojom::kCardanoMainnet));
  EXPECT_TRUE(IsCardanoNetwork(mojom::kCardanoTestnet));

  EXPECT_FALSE(IsCardanoNetwork(mojom::kMainnetChainId));
  EXPECT_FALSE(IsCardanoNetwork(mojom::kFilecoinMainnet));
  EXPECT_FALSE(IsCardanoNetwork(mojom::kSolanaMainnet));
  EXPECT_FALSE(IsCardanoNetwork(mojom::kBitcoinMainnet));
  EXPECT_FALSE(IsCardanoNetwork(""));
  EXPECT_FALSE(IsCardanoNetwork("abc"));
}

TEST(CommonUtils, IsCardanoAccount) {
  EXPECT_TRUE(IsCardanoAccount(MakeIndexBasedAccountId(
      mojom::CoinType::ADA, mojom::KeyringId::kCardanoMainnet,
      mojom::AccountKind::kDerived, 4)));
  EXPECT_TRUE(IsCardanoAccount(MakeIndexBasedAccountId(
      mojom::CoinType::ADA, mojom::KeyringId::kCardanoTestnet,
      mojom::AccountKind::kDerived, 7)));

  EXPECT_FALSE(IsCardanoAccount(
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived, "0xasdf")));
  EXPECT_FALSE(IsCardanoAccount(
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived, "0xasdf")));
  EXPECT_FALSE(IsCardanoAccount(
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoin,
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

TEST(CommonUtils, GetEnabledCoins) {
  base::test::ScopedFeatureList disabled_feature_list;
  const std::vector<base::test::FeatureRef> coin_features = {
      features::kBraveWalletBitcoinFeature, features::kBraveWalletZCashFeature,
      features::kBraveWalletCardanoFeature};
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

    auto coins = GetEnabledCoins();
    size_t last_pos = 0;

    EXPECT_EQ(coins[last_pos++], mojom::CoinType::ETH);
    EXPECT_EQ(coins[last_pos++], mojom::CoinType::SOL);
    EXPECT_EQ(coins[last_pos++], mojom::CoinType::FIL);

    if (IsBitcoinEnabled()) {
      EXPECT_EQ(coins[last_pos++], mojom::CoinType::BTC);
    }

    if (IsZCashEnabled()) {
      EXPECT_EQ(coins[last_pos++], mojom::CoinType::ZEC);
    }

    if (IsCardanoEnabled()) {
      EXPECT_EQ(coins[last_pos++], mojom::CoinType::ADA);
    }

    EXPECT_EQ(last_pos, coins.size());
  }

  static_assert(AllCoinsTested<6>());
}

TEST(CommonUtils, GetEnabledKeyrings) {
  base::test::ScopedFeatureList disabled_feature_list;
  const std::vector<base::test::FeatureRef> coin_features = {
      features::kBraveWalletBitcoinFeature,
      features::kBraveWalletZCashFeature,
      features::kBraveWalletBitcoinImportFeature,
      features::kBraveWalletBitcoinLedgerFeature,
      features::kBraveWalletCardanoFeature,
  };
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

    auto keyrings = GetEnabledKeyrings();
    size_t last_pos = 0;

    EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kDefault);

    EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kFilecoin);
    EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kFilecoinTestnet);

    EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kSolana);

    if (IsBitcoinEnabled()) {
      EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kBitcoin84);
      EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kBitcoin84Testnet);
      if (IsBitcoinImportEnabled()) {
        EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kBitcoinImport);
        EXPECT_EQ(keyrings[last_pos++],
                  mojom::KeyringId::kBitcoinImportTestnet);
      }
      if (IsBitcoinLedgerEnabled()) {
        EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kBitcoinHardware);
        EXPECT_EQ(keyrings[last_pos++],
                  mojom::KeyringId::kBitcoinHardwareTestnet);
      }
    }

    if (IsZCashEnabled()) {
      EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kZCashMainnet);
      EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kZCashTestnet);
    }

    if (IsCardanoEnabled()) {
      EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kCardanoMainnet);
      EXPECT_EQ(keyrings[last_pos++], mojom::KeyringId::kCardanoTestnet);
    }

    EXPECT_EQ(last_pos, keyrings.size());
  }

  static_assert(AllKeyringsTested<14>());
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
              ElementsAreArray({mojom::KeyringId::kBitcoin84,
                                mojom::KeyringId::kBitcoinImport,
                                mojom::KeyringId::kBitcoinHardware}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::BTC,
                                             mojom::kBitcoinTestnet),
              ElementsAreArray({mojom::KeyringId::kBitcoin84Testnet,
                                mojom::KeyringId::kBitcoinImportTestnet,
                                mojom::KeyringId::kBitcoinHardwareTestnet}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::BTC,
                                             "any non mainnet chain"),
              ElementsAreArray({mojom::KeyringId::kBitcoin84Testnet,
                                mojom::KeyringId::kBitcoinImportTestnet,
                                mojom::KeyringId::kBitcoinHardwareTestnet}));

  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::ZEC,
                                             mojom::kZCashMainnet),
              ElementsAreArray({mojom::KeyringId::kZCashMainnet}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::ZEC,
                                             mojom::kZCashTestnet),
              ElementsAreArray({mojom::KeyringId::kZCashTestnet}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::ZEC,
                                             "any non mainnet chain"),
              ElementsAreArray({mojom::KeyringId::kZCashTestnet}));

  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::ADA,
                                             mojom::kCardanoMainnet),
              ElementsAreArray({mojom::KeyringId::kCardanoMainnet}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::ADA,
                                             mojom::kCardanoTestnet),
              ElementsAreArray({mojom::KeyringId::kCardanoTestnet}));
  EXPECT_THAT(GetSupportedKeyringsForNetwork(mojom::CoinType::ADA,
                                             "any non mainnet chain"),
              ElementsAreArray({mojom::KeyringId::kCardanoTestnet}));

  static_assert(AllCoinsTested<6>());

  static_assert(AllKeyringsTested<14>());
}

TEST(CommonUtils, MakeAccountId) {
  auto id = *MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                           mojom::AccountKind::kDerived,
                           "0xA0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");
  EXPECT_EQ(id.coin, mojom::CoinType::ETH);
  EXPECT_EQ(id.keyring_id, mojom::KeyringId::kDefault);
  EXPECT_EQ(id.kind, mojom::AccountKind::kDerived);
  EXPECT_EQ(id.address, "0xA0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");
  EXPECT_EQ(id.account_index, 0u);

  EXPECT_EQ(id.unique_key, "60_0_0_0xA0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");

  // Same AccountId
  EXPECT_EQ(
      id.unique_key,
      MakeAccountId(id.coin, id.keyring_id, id.kind, id.address)->unique_key);

  // Coin differs
  for (const auto& coin : kAllCoins) {
    if (coin == mojom::CoinType::ETH) {
      continue;
    } else if (coin == mojom::CoinType::BTC || coin == mojom::CoinType::ZEC ||
               coin == mojom::CoinType::ADA) {
      EXPECT_DCHECK_DEATH(
          MakeAccountId(coin, id.keyring_id, id.kind, id.address));
    } else {
      EXPECT_NE(
          id.unique_key,
          MakeAccountId(coin, id.keyring_id, id.kind, id.address)->unique_key);
    }
  }

  // Keyring differs
  for (const auto& keyring : kAllKeyrings) {
    if (keyring == mojom::KeyringId::kDefault) {
      continue;
    } else if (IsBitcoinKeyring(keyring) || IsZCashKeyring(keyring) ||
               IsCardanoKeyring(keyring)) {
      EXPECT_DCHECK_DEATH(MakeAccountId(id.coin, keyring, id.kind, id.address));
    } else {
      EXPECT_NE(
          id.unique_key,
          MakeAccountId(id.coin, keyring, id.kind, id.address)->unique_key);
    }
  }

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
                                    mojom::KeyringId::kBitcoin84,
                                    mojom::AccountKind::kDerived, "0xabc"));
  EXPECT_DCHECK_DEATH(MakeAccountId(mojom::CoinType::ZEC,
                                    mojom::KeyringId::kZCashMainnet,
                                    mojom::AccountKind::kDerived, "0xabc"));
  EXPECT_DCHECK_DEATH(MakeAccountId(mojom::CoinType::ADA,
                                    mojom::KeyringId::kCardanoMainnet,
                                    mojom::AccountKind::kDerived, "0xabc"));
  static_assert(AllCoinsTested<6>());

  static_assert(AllKeyringsTested<14>());
}

TEST(CommonUtils, MakeIndexBasedAccountId) {
  // Coin differs
  for (const auto& coin : kAllCoins) {
    if (coin == mojom::CoinType::BTC || coin == mojom::CoinType::ZEC ||
        coin == mojom::CoinType::ADA) {
      continue;
    }
    EXPECT_DCHECK_DEATH(MakeIndexBasedAccountId(
        coin, mojom::KeyringId::kBitcoin84, mojom::AccountKind::kDerived, 123));
  }

  // Keyring differs
  for (const auto& keyring : kAllKeyrings) {
    if (IsBitcoinKeyring(keyring) || IsZCashKeyring(keyring) ||
        IsCardanoKeyring(keyring)) {
      continue;
    }
    EXPECT_DCHECK_DEATH(MakeIndexBasedAccountId(
        mojom::CoinType::BTC, keyring, mojom::AccountKind::kDerived, 123));
  }
}

TEST(CommonUtils, MakeIndexBasedAccountId_BTC) {
  auto id = *MakeIndexBasedAccountId(mojom::CoinType::BTC,
                                     mojom::KeyringId::kBitcoin84,
                                     mojom::AccountKind::kDerived, 123);
  EXPECT_EQ(id.coin, mojom::CoinType::BTC);
  EXPECT_EQ(id.keyring_id, mojom::KeyringId::kBitcoin84);
  EXPECT_EQ(id.kind, mojom::AccountKind::kDerived);
  EXPECT_EQ(id.address, "");
  EXPECT_EQ(id.account_index, 123u);
  EXPECT_EQ(id.unique_key, "0_4_0_123");

  // Same AccountId
  EXPECT_EQ(id.unique_key, MakeIndexBasedAccountId(id.coin, id.keyring_id,
                                                   id.kind, id.account_index)
                               ->unique_key);

  // Keyring differs
  EXPECT_NE(id.unique_key, MakeIndexBasedAccountId(
                               id.coin, mojom::KeyringId::kBitcoin84Testnet,
                               id.kind, id.account_index)
                               ->unique_key);

  // Index differs
  EXPECT_NE(id.unique_key,
            MakeIndexBasedAccountId(id.coin, id.keyring_id, id.kind, 321)
                ->unique_key);

  EXPECT_TRUE(MakeIndexBasedAccountId(mojom::CoinType::BTC,
                                      mojom::KeyringId::kBitcoin84,
                                      mojom::AccountKind::kDerived, 123));

  // kImported is a valid kind.
  EXPECT_TRUE(MakeIndexBasedAccountId(mojom::CoinType::BTC,
                                      mojom::KeyringId::kBitcoinImport,
                                      mojom::AccountKind::kImported, 123));
  EXPECT_TRUE(MakeIndexBasedAccountId(mojom::CoinType::BTC,
                                      mojom::KeyringId::kBitcoinImportTestnet,
                                      mojom::AccountKind::kImported, 123));
  // kHardware is a valid kind.
  EXPECT_TRUE(MakeIndexBasedAccountId(mojom::CoinType::BTC,
                                      mojom::KeyringId::kBitcoinHardware,
                                      mojom::AccountKind::kHardware, 123));
  EXPECT_TRUE(MakeIndexBasedAccountId(mojom::CoinType::BTC,
                                      mojom::KeyringId::kBitcoinHardwareTestnet,
                                      mojom::AccountKind::kHardware, 123));
  static_assert(AllCoinsTested<6>());

  static_assert(AllKeyringsTested<14>());
}

TEST(CommonUtils, MakeIndexBasedAccountId_ZEC) {
  auto id = *MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                     mojom::KeyringId::kZCashMainnet,
                                     mojom::AccountKind::kDerived, 123);
  EXPECT_EQ(id.coin, mojom::CoinType::ZEC);
  EXPECT_EQ(id.keyring_id, mojom::KeyringId::kZCashMainnet);
  EXPECT_EQ(id.kind, mojom::AccountKind::kDerived);
  EXPECT_EQ(id.address, "");
  EXPECT_EQ(id.account_index, 123u);
  EXPECT_EQ(id.unique_key, "133_6_0_123");

  // Keyring differs
  EXPECT_EQ("133_7_0_123",
            MakeIndexBasedAccountId(id.coin, mojom::KeyringId::kZCashTestnet,
                                    id.kind, id.account_index)
                ->unique_key);

  // Index differs
  EXPECT_EQ("133_6_0_321",
            MakeIndexBasedAccountId(id.coin, id.keyring_id, id.kind, 321)
                ->unique_key);

  // Imported not supported.
  EXPECT_DCHECK_DEATH(MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet,
      mojom::AccountKind::kImported, 123));

  // Hardware not supported.
  EXPECT_DCHECK_DEATH(MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet,
      mojom::AccountKind::kHardware, 123));
}

TEST(CommonUtils, MakeIndexBasedAccountId_ADA) {
  auto id = *MakeIndexBasedAccountId(mojom::CoinType::ADA,
                                     mojom::KeyringId::kCardanoMainnet,
                                     mojom::AccountKind::kDerived, 123);
  EXPECT_EQ(id.coin, mojom::CoinType::ADA);
  EXPECT_EQ(id.keyring_id, mojom::KeyringId::kCardanoMainnet);
  EXPECT_EQ(id.kind, mojom::AccountKind::kDerived);
  EXPECT_EQ(id.address, "");
  EXPECT_EQ(id.account_index, 123u);
  EXPECT_EQ(id.unique_key, "1815_12_0_123");

  // Keyring differs
  EXPECT_EQ("1815_13_0_123",
            MakeIndexBasedAccountId(id.coin, mojom::KeyringId::kCardanoTestnet,
                                    id.kind, id.account_index)
                ->unique_key);

  // Index differs
  EXPECT_EQ("1815_12_0_321",
            MakeIndexBasedAccountId(id.coin, id.keyring_id, id.kind, 321)
                ->unique_key);

  // Imported not supported.
  EXPECT_DCHECK_DEATH(MakeIndexBasedAccountId(
      mojom::CoinType::ADA, mojom::KeyringId::kCardanoMainnet,
      mojom::AccountKind::kImported, 123));

  // Hardware not supported.
  EXPECT_DCHECK_DEATH(MakeIndexBasedAccountId(
      mojom::CoinType::ADA, mojom::KeyringId::kCardanoMainnet,
      mojom::AccountKind::kHardware, 123));
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

TEST(CommonUtils, GetNetworkForZCashKeyring) {
  EXPECT_EQ(mojom::kZCashMainnet,
            GetNetworkForZCashKeyring(mojom::KeyringId::kZCashMainnet));
  EXPECT_EQ(mojom::kZCashTestnet,
            GetNetworkForZCashKeyring(mojom::KeyringId::kZCashTestnet));
}

TEST(CommonUtils, GetNetworkForBitcoinKeyring) {
  EXPECT_EQ(mojom::kBitcoinMainnet,
            GetNetworkForBitcoinKeyring(mojom::KeyringId::kBitcoin84));
  EXPECT_EQ(mojom::kBitcoinTestnet,
            GetNetworkForBitcoinKeyring(mojom::KeyringId::kBitcoin84Testnet));
}

TEST(CommonUtils, GetNetworkForBitcoinAccount) {
  EXPECT_EQ(mojom::kBitcoinMainnet,
            GetNetworkForBitcoinAccount(MakeIndexBasedAccountId(
                mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
                mojom::AccountKind::kDerived, 123)));
  EXPECT_EQ(mojom::kBitcoinTestnet,
            GetNetworkForBitcoinAccount(MakeIndexBasedAccountId(
                mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84Testnet,
                mojom::AccountKind::kDerived, 123)));
}

}  // namespace brave_wallet
