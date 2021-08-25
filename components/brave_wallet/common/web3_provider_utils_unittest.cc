/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/web3_provider_utils.h"

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

TEST(Web3ProviderUtilsUnitTest, ParameterValueToEthereumChainTest) {
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ParameterValueToEthereumChain(base::JSONReader::Read(R"({
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
    EXPECT_EQ("Goerli ETH", chain->name);
    EXPECT_EQ("gorETH", chain->symbol);
    EXPECT_EQ(18, chain->decimals);
    EXPECT_EQ("https://xdaichain.com/fake/example/url/xdai.svg",
              chain->icon_urls.front());
    EXPECT_EQ("https://xdaichain.com/fake/example/url/xdai.png",
              chain->icon_urls.back());
  }
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ParameterValueToEthereumChain(base::JSONReader::Read(R"({
      "chainId": "0x5"
    })")
                                                        .value());
    ASSERT_TRUE(chain);
    EXPECT_EQ("0x5", chain->chain_id);
    ASSERT_TRUE(chain->chain_name.empty());
    ASSERT_TRUE(chain->rpc_urls.empty());
    ASSERT_TRUE(chain->icon_urls.empty());
    ASSERT_TRUE(chain->block_explorer_urls.empty());
    ASSERT_TRUE(chain->name.empty());
    ASSERT_TRUE(chain->symbol.empty());
  }

  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ParameterValueToEthereumChain(base::JSONReader::Read(R"({
    })")
                                                        .value());
    ASSERT_FALSE(chain);
  }
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ParameterValueToEthereumChain(base::JSONReader::Read(R"([
          ])")
                                                        .value());
    ASSERT_FALSE(chain);
  }
}
