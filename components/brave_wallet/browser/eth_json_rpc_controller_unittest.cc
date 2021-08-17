/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/testing_pref_service.h"
#include "components/user_prefs/user_prefs.h"
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
    user_prefs::UserPrefs::Set(browser_context_.get(), &prefs_);
  }

  ~EthJsonRpcControllerUnitTest() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }
  PrefService* prefs() { return &prefs_; }
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
  content::BrowserTaskEnvironment browser_task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  TestingPrefServiceSimple prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(EthJsonRpcControllerUnitTest, SetNetwork) {
  EthJsonRpcController controller(shared_url_loader_factory(), prefs());

  auto networks = brave_wallet::GetAllKnownChains();
  for (const auto& network : networks) {
    controller.SetNetwork(network.chain_id);
    controller.GetChainId(base::BindOnce(
        [](const std::string& expected_id, const std::string& chain_id) {
          EXPECT_EQ(chain_id, expected_id);
        },
        network.chain_id));
    controller.GetNetworkUrl(base::BindOnce(
        [](const std::string& expected_url, const std::string& spec) {
          EXPECT_EQ(GURL(spec).GetOrigin(), GURL(expected_url).GetOrigin());
        },
        network.rpc_urls.front()));
  }
  base::RunLoop().RunUntilIdle();
}

TEST_F(EthJsonRpcControllerUnitTest, ResolveENSDomain) {
  EthJsonRpcController controller(shared_url_loader_factory(), prefs());
  controller.SetNetwork("localhost");
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
