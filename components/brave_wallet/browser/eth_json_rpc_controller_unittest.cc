/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_wallet {

class EthJsonRpcControllerUnitTest : public testing::Test {
 public:
  EthJsonRpcControllerUnitTest()
      : browser_context_(new content::TestBrowserContext()) {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    auto resource_request = base::BindRepeating(
        &EthJsonRpcControllerUnitTest::ResourceRequest, this);
    url_loader_factory_.SetInterceptor(std::move(resource_request));
  }

  ~EthJsonRpcControllerUnitTest() override = default;

  network::SharedURLLoaderFactory* shared_url_loader_factory() {
    return url_loader_factory_.GetSafeWeakWrapper().get();
  }
  void SwitchToNextResponse() {
    url_loader_factory_.ClearResponses();
    url_loader_factory_.AddResponse(
        "https://mainnet-infura.brave.com/f7106c838853428280fa0c585acc9485",
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0000000000000000000000"
        "0000000000000000000000000000000000000000200000000000000000000000000"
        "000000000000000000000000000000000000026e3010170122008ab7bf21b738283"
        "64305ef6b7c676c1f5a73e18ab4f93beec7e21e0bc84010e0000000000000000000"
        "000000000000000000000000000000000\"}");
  }
  static void ResourceRequest(EthJsonRpcControllerUnitTest* controller,
                              const network::ResourceRequest& request) {
    if (controller)
      controller->SwitchToNextResponse();
  }

  void SetRegistrarResponse() {
    url_loader_factory_.AddResponse(
        "https://mainnet-infura.brave.com/f7106c838853428280fa0c585acc9485",
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x00000"
        "0000000000000000000226159d592e2b063810a10ebf6dcbada94ed68b8\"}");
  }
  content::TestBrowserContext* context() { return browser_context_.get(); }

 private:
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
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

TEST_F(EthJsonRpcControllerUnitTest, ResolveENSDomain) {
  EthJsonRpcController controller(Network::kMainnet,
                                  shared_url_loader_factory());
  SetRegistrarResponse();
  base::RunLoop run;
  controller.EnsProxyReaderGetResolverAddress(
      "0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e", "blocktimer.dappstar.eth",
      base::BindOnce(
          [](base::OnceClosure done, bool status, const std::string& result) {
            ASSERT_TRUE(status);
            EXPECT_EQ(result,
                      "0x000000000000000000000000000000000000000000000000000000"
                      "00000000200000000000000000000000000000000000000000000000"
                      "000000000000000026e3010170122008ab7bf21b73828364305ef6b7"
                      "c676c1f5a73e18ab4f93beec7e21e0bc84010e000000000000000000"
                      "0000000000000000000000000000000000");
            std::move(done).Run();
          },
          run.QuitClosure()));
  run.Run();
}

}  // namespace brave_wallet
