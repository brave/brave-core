/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "services/network/test/test_shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_wallet {

class EthJsonRpcControllerUnitTest : public testing::Test {
 public:
  EthJsonRpcControllerUnitTest()
      : browser_context_(new content::TestBrowserContext()) {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::TestSharedURLLoaderFactory>(
            nullptr /* network_service */, true /* is_trusted */);
  }
  ~EthJsonRpcControllerUnitTest() override = default;

  network::SharedURLLoaderFactory* shared_url_loader_factory() {
    return shared_url_loader_factory_.get();
  }

  content::TestBrowserContext* context() { return browser_context_.get(); }

 private:
  scoped_refptr<network::TestSharedURLLoaderFactory> shared_url_loader_factory_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
};

TEST_F(EthJsonRpcControllerUnitTest, SetNetwork) {
  EthJsonRpcController controller(Network::kRinkeby,
                                  shared_url_loader_factory());
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
  EthJsonRpcController controller(Network::kMainnet,
                                  shared_url_loader_factory());
  GURL custom_network("http://tesshared_url_loader_factoryt.com/");
  controller.SetCustomNetwork(custom_network);
  ASSERT_EQ(controller.GetNetwork(), Network::kCustom);
  ASSERT_EQ(controller.GetNetworkURL(), custom_network);
}

}  // namespace brave_wallet
