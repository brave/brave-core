/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <vector>

#include "brave/components/brave_wallet/brave_wallet_constants.h"
#include "brave/components/brave_wallet/eth_json_rpc_controller.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_wallet {

class EthJsonRpcControllerUnitTest : public testing::Test {
 public:
  EthJsonRpcControllerUnitTest()
      : browser_context_(new content::TestBrowserContext()) {}
  ~EthJsonRpcControllerUnitTest() override = default;

  content::TestBrowserContext* context() { return browser_context_.get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
};

TEST_F(EthJsonRpcControllerUnitTest, SetNetwork) {
  EthJsonRpcController controller(context(), Network::kRinkeby);
  ASSERT_EQ(controller.GetNetwork(), Network::kRinkeby);
  ASSERT_EQ(controller.GetNetworkURL().GetOrigin(),
            "https://rinkeby-infura.brave.com/");

  controller.SetNetwork(Network::kMainnet);
  ASSERT_EQ(controller.GetNetwork(), Network::kMainnet);
  ASSERT_EQ(controller.GetNetworkURL().GetOrigin(),
            "https://mainnet-infura.brave.com/");

  controller.SetNetwork(Network::kRinkeby);
  ASSERT_EQ(controller.GetNetwork(), Network::kRinkeby);
  ASSERT_EQ(controller.GetNetworkURL().GetOrigin(),
            "https://rinkeby-infura.brave.com/");

  controller.SetNetwork(Network::kRopsten);
  ASSERT_EQ(controller.GetNetwork(), Network::kRopsten);
  ASSERT_EQ(controller.GetNetworkURL().GetOrigin(),
            "https://ropsten-infura.brave.com/");

  controller.SetNetwork(Network::kGoerli);
  ASSERT_EQ(controller.GetNetwork(), Network::kGoerli);
  ASSERT_EQ(controller.GetNetworkURL().GetOrigin(),
            "https://goerli-infura.brave.com/");

  controller.SetNetwork(Network::kKovan);
  ASSERT_EQ(controller.GetNetwork(), Network::kKovan);
  ASSERT_EQ(controller.GetNetworkURL().GetOrigin(),
            "https://kovan-infura.brave.com/");

  controller.SetNetwork(Network::kLocalhost);
  ASSERT_EQ(controller.GetNetwork(), Network::kLocalhost);
  ASSERT_EQ(controller.GetNetworkURL().GetOrigin(), "http://localhost:8545/");
}

TEST_F(EthJsonRpcControllerUnitTest, SetCustomNetwork) {
  EthJsonRpcController controller(context(), Network::kMainnet);
  GURL custom_network("http://test.com/");
  controller.SetCustomNetwork(custom_network);
  ASSERT_EQ(controller.GetNetwork(), Network::kCustom);
  ASSERT_EQ(controller.GetNetworkURL(), custom_network);
}

}  // namespace brave_wallet
