/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/value_conversion_utils.h"

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

TEST(ValueConversionUtilsUnitTest, ValueToEthereumChainTest) {
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"({
      "chainId": "0x5",
      "chainName": "Goerli",
      "rpcUrls": [
        "https://goerli.infura.io/v3/INSERT_API_KEY_HERE",
        "https://second.infura.io/"
      ],
      "iconUrls": [
        "https://xdaichain.com/fake/example/url/xdai.svg",
        "https://xdaichain.com/fake/example/url/xdai.png"
      ],
      "nativeCurrency": {
        "name": "Goerli ETH",
        "symbol": "gorETH",
        "decimals": 18
      },
      "blockExplorerUrls": ["https://goerli.etherscan.io"]
    })")
                                               .value());
    ASSERT_TRUE(chain);
    EXPECT_EQ("0x5", chain->chain_id);
    EXPECT_EQ("Goerli", chain->chain_name);
    EXPECT_EQ(size_t(2), chain->rpc_urls.size());
    EXPECT_EQ("https://goerli.infura.io/v3/INSERT_API_KEY_HERE",
              chain->rpc_urls.front());
    EXPECT_EQ("https://second.infura.io/", chain->rpc_urls.back());
    EXPECT_EQ("https://goerli.etherscan.io",
              chain->block_explorer_urls.front());
    EXPECT_EQ("Goerli ETH", chain->symbol_name);
    EXPECT_EQ("gorETH", chain->symbol);
    EXPECT_EQ(18, chain->decimals);
    EXPECT_EQ("https://xdaichain.com/fake/example/url/xdai.svg",
              chain->icon_urls.front());
    EXPECT_EQ("https://xdaichain.com/fake/example/url/xdai.png",
              chain->icon_urls.back());
  }
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"({
      "chainId": "0x5"
    })")
                                               .value());
    ASSERT_TRUE(chain);
    EXPECT_EQ("0x5", chain->chain_id);
    ASSERT_TRUE(chain->chain_name.empty());
    ASSERT_TRUE(chain->rpc_urls.empty());
    ASSERT_TRUE(chain->icon_urls.empty());
    ASSERT_TRUE(chain->block_explorer_urls.empty());
    ASSERT_TRUE(chain->symbol_name.empty());
    ASSERT_TRUE(chain->symbol.empty());
  }

  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"({
    })")
                                               .value());
    ASSERT_FALSE(chain);
  }
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"([
          ])")
                                               .value());
    ASSERT_FALSE(chain);
  }
}

TEST(ValueConversionUtilsUnitTest, EthereumChainToValueTest) {
  brave_wallet::mojom::EthereumChain chain(
      "chain_id", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11);
  base::Value value = brave_wallet::EthereumChainToValue(chain.Clone());
  EXPECT_EQ(*value.FindStringKey("chainId"), chain.chain_id);
  EXPECT_EQ(*value.FindStringKey("chainName"), chain.chain_name);
  EXPECT_EQ(*value.FindStringPath("nativeCurrency.name"), chain.symbol_name);
  EXPECT_EQ(*value.FindStringPath("nativeCurrency.symbol"), chain.symbol);
  EXPECT_EQ(*value.FindIntPath("nativeCurrency.decimals"), chain.decimals);
  for (const auto& entry : value.FindListKey("rpcUrls")->GetList()) {
    ASSERT_NE(std::find(chain.rpc_urls.begin(), chain.rpc_urls.end(),
                        entry.GetString()),
              chain.rpc_urls.end());
  }

  for (const auto& entry : value.FindListKey("iconUrls")->GetList()) {
    ASSERT_NE(std::find(chain.icon_urls.begin(), chain.icon_urls.end(),
                        entry.GetString()),
              chain.icon_urls.end());
  }
  auto* blocked_urls = value.FindListKey("blockExplorerUrls");
  for (const auto& entry : blocked_urls->GetList()) {
    ASSERT_NE(std::find(chain.block_explorer_urls.begin(),
                        chain.block_explorer_urls.end(), entry.GetString()),
              chain.block_explorer_urls.end());
  }

  auto result = brave_wallet::ValueToEthereumChain(value);
  ASSERT_TRUE(result->Equals(chain));
}
