/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/browser/storage_partition.h"
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
  EthJsonRpcControllerUnitTest() {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    auto resource_request = base::BindRepeating(
        &EthJsonRpcControllerUnitTest::ResourceRequest, this);
    url_loader_factory_.SetInterceptor(std::move(resource_request));
  }

  ~EthJsonRpcControllerUnitTest() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }
  void SwitchToNextResponse() {
    url_loader_factory_.ClearResponses();
    url_loader_factory_.AddResponse(
        "http://localhost:8545/",
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
        "http://localhost:8545/",
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x00000"
        "0000000000000000000226159d592e2b063810a10ebf6dcbada94ed68b8\"}");
  }

 private:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(EthJsonRpcControllerUnitTest, SetNetwork) {
  EthJsonRpcController controller(brave_wallet::mojom::Network::Mainnet,
                                  shared_url_loader_factory());

  controller.SetNetwork(brave_wallet::mojom::Network::Mainnet);
  controller.GetNetwork(
      base::BindOnce([](brave_wallet::mojom::Network network) {
        EXPECT_EQ(network, brave_wallet::mojom::Network::Mainnet);
      }));
  controller.GetNetworkUrl(base::BindOnce([](const std::string& spec) {
    EXPECT_EQ(GURL(spec).GetOrigin(), "https://mainnet-infura.brave.com/");
  }));

  controller.SetNetwork(brave_wallet::mojom::Network::Rinkeby);
  controller.GetNetwork(
      base::BindOnce([](brave_wallet::mojom::Network network) {
        EXPECT_EQ(network, brave_wallet::mojom::Network::Rinkeby);
      }));
  controller.GetNetworkUrl(base::BindOnce([](const std::string& spec) {
    EXPECT_EQ(GURL(spec).GetOrigin(), "https://rinkeby-infura.brave.com/");
  }));

  controller.SetNetwork(brave_wallet::mojom::Network::Ropsten);
  controller.GetNetwork(
      base::BindOnce([](brave_wallet::mojom::Network network) {
        EXPECT_EQ(network, brave_wallet::mojom::Network::Ropsten);
      }));
  controller.GetNetworkUrl(base::BindOnce([](const std::string& spec) {
    EXPECT_EQ(GURL(spec).GetOrigin(), "https://ropsten-infura.brave.com/");
  }));

  controller.SetNetwork(brave_wallet::mojom::Network::Goerli);
  controller.GetNetwork(
      base::BindOnce([](brave_wallet::mojom::Network network) {
        EXPECT_EQ(network, brave_wallet::mojom::Network::Goerli);
      }));
  controller.GetNetworkUrl(base::BindOnce([](const std::string& spec) {
    EXPECT_EQ(GURL(spec).GetOrigin(), "https://goerli-infura.brave.com/");
  }));

  controller.SetNetwork(brave_wallet::mojom::Network::Kovan);
  controller.GetNetwork(
      base::BindOnce([](brave_wallet::mojom::Network network) {
        EXPECT_EQ(network, brave_wallet::mojom::Network::Kovan);
      }));
  controller.GetNetworkUrl(base::BindOnce([](const std::string& spec) {
    EXPECT_EQ(GURL(spec).GetOrigin(), "https://kovan-infura.brave.com/");
  }));

  controller.SetNetwork(brave_wallet::mojom::Network::Localhost);
  controller.GetNetwork(
      base::BindOnce([](brave_wallet::mojom::Network network) {
        EXPECT_EQ(network, brave_wallet::mojom::Network::Localhost);
      }));
  controller.GetNetworkUrl(base::BindOnce([](const std::string& spec) {
    EXPECT_EQ(GURL(spec).GetOrigin(), "http://localhost:8545/");
  }));
  base::RunLoop().RunUntilIdle();
}

TEST_F(EthJsonRpcControllerUnitTest, SetCustomNetwork) {
  EthJsonRpcController controller(brave_wallet::mojom::Network::Mainnet,
                                  shared_url_loader_factory());
  std::string custom_network("http://tesshared_url_loader_factoryt.com/");
  controller.SetCustomNetwork(GURL(custom_network));

  controller.GetNetwork(
      base::BindOnce([](brave_wallet::mojom::Network network) {
        EXPECT_EQ(network, brave_wallet::mojom::Network::Custom);
      }));
  controller.GetNetworkUrl(base::BindOnce(
      [](std::string custom_network, const std::string& spec) {
        EXPECT_EQ(GURL(spec).GetOrigin(), custom_network);
      },
      custom_network));

  base::RunLoop().RunUntilIdle();
}

TEST_F(EthJsonRpcControllerUnitTest, ResolveENSDomain) {
  EthJsonRpcController controller(brave_wallet::mojom::Network::Localhost,
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
