/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

void UpdateCustomNetworks(PrefService* prefs,
                          std::vector<base::Value>* values) {
  ListPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::ListValue* list = update.Get();
  list->ClearList();
  for (auto& it : *values) {
    list->Append(std::move(it));
  }
}

class FakeEthereumChainObserver
    : public brave_wallet::mojom::EthJsonRpcControllerObserver {
 public:
  FakeEthereumChainObserver(base::OnceClosure callback,
                            const std::string& expected_chain_id,
                            const bool expected_error_empty) {
    callback_ = std::move(callback);
    expected_chain_id_ = expected_chain_id;
    expected_error_empty_ = expected_error_empty;
  }

  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override {
    EXPECT_EQ(chain_id, expected_chain_id_);
    EXPECT_EQ(error.empty(), expected_error_empty_);
    std::move(callback_).Run();
  }
  void ChainChangedEvent(const std::string& chain_id) override { NOTREACHED(); }
  ::mojo::PendingRemote<brave_wallet::mojom::EthJsonRpcControllerObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }
  base::OnceClosure callback_;
  std::string expected_chain_id_;
  bool expected_error_empty_;
  mojo::Receiver<brave_wallet::mojom::EthJsonRpcControllerObserver>
      observer_receiver_{this};
};

}  // namespace

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
    KeyringController::RegisterProfilePrefs(prefs_.registry());
    EthJsonRpcController::RegisterProfilePrefs(prefs_.registry());
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

  void ValidateStartWithNetwork(const std::string& chain_id,
                                const std::string& expected_id) {
    prefs()->SetString(kBraveWalletCurrentChainId, chain_id);
    EthJsonRpcController controller(shared_url_loader_factory(), prefs());
    bool callback_is_called = false;
    controller.GetChainId(base::BindLambdaForTesting(
        [&callback_is_called, &expected_id](const std::string& chain_id) {
          EXPECT_EQ(chain_id, expected_id);
          callback_is_called = true;
        }));
    ASSERT_TRUE(callback_is_called);
  }

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(EthJsonRpcControllerUnitTest, SetNetwork) {
  EthJsonRpcController controller(shared_url_loader_factory(), prefs());

  std::vector<mojom::EthereumChainPtr> networks;
  brave_wallet::GetAllKnownChains(&networks);
  for (const auto& network : networks) {
    bool callback_is_called = false;
    controller.SetNetwork(network->chain_id);
    EXPECT_EQ(network->chain_id,
              prefs()->GetString(kBraveWalletCurrentChainId));
    const std::string& expected_id = network->chain_id;
    controller.GetChainId(base::BindLambdaForTesting(
        [&callback_is_called, &expected_id](const std::string& chain_id) {
          EXPECT_EQ(chain_id, expected_id);
          callback_is_called = true;
        }));
    ASSERT_TRUE(callback_is_called);

    callback_is_called = false;
    const std::string& expected_url = network->rpc_urls.front();
    controller.GetNetworkUrl(base::BindLambdaForTesting(
        [&callback_is_called, &expected_url](const std::string& spec) {
          EXPECT_EQ(GURL(spec).GetOrigin(), GURL(expected_url).GetOrigin());
          callback_is_called = true;
        }));
    ASSERT_TRUE(callback_is_called);
  }
  base::RunLoop().RunUntilIdle();
}

TEST_F(EthJsonRpcControllerUnitTest, SetCustomNetwork) {
  std::vector<base::Value> values;
  brave_wallet::mojom::EthereumChain chain1(
      "chain_id", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11);
  auto chain_ptr1 = chain1.Clone();
  values.push_back(brave_wallet::EthereumChainToValue(chain_ptr1));

  brave_wallet::mojom::EthereumChain chain2(
      "chain_id2", "chain_name2", {"https://url2.com"}, {"https://url2.com"},
      {"https://url2.com"}, "symbol_name2", "symbol2", 22);
  auto chain_ptr2 = chain2.Clone();
  values.push_back(brave_wallet::EthereumChainToValue(chain_ptr2));
  UpdateCustomNetworks(prefs(), &values);

  EthJsonRpcController controller(shared_url_loader_factory(), prefs());

  bool callback_is_called = false;
  controller.SetNetwork(chain1.chain_id);
  const std::string& expected_id = chain1.chain_id;
  controller.GetChainId(base::BindLambdaForTesting(
      [&callback_is_called, &expected_id](const std::string& chain_id) {
        EXPECT_EQ(chain_id, expected_id);
        callback_is_called = true;
      }));
  ASSERT_TRUE(callback_is_called);
  callback_is_called = false;
  const std::string& expected_url = chain1.rpc_urls.front();
  controller.GetNetworkUrl(base::BindLambdaForTesting(
      [&callback_is_called, &expected_url](const std::string& spec) {
        EXPECT_EQ(GURL(spec).GetOrigin(), GURL(expected_url).GetOrigin());
        callback_is_called = true;
      }));
  ASSERT_TRUE(callback_is_called);
  base::RunLoop().RunUntilIdle();
}

TEST_F(EthJsonRpcControllerUnitTest, GetAllNetworks) {
  std::vector<base::Value> values;
  brave_wallet::mojom::EthereumChain chain1(
      "chain_id", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11);
  auto chain_ptr1 = chain1.Clone();
  values.push_back(brave_wallet::EthereumChainToValue(chain_ptr1));

  brave_wallet::mojom::EthereumChain chain2(
      "chain_id2", "chain_name2", {"https://url2.com"}, {"https://url2.com"},
      {"https://url2.com"}, "symbol_name2", "symbol2", 22);
  auto chain_ptr2 = chain2.Clone();
  values.push_back(brave_wallet::EthereumChainToValue(chain_ptr2));
  UpdateCustomNetworks(prefs(), &values);

  EthJsonRpcController controller(shared_url_loader_factory(), prefs());
  std::vector<mojom::EthereumChainPtr> expected_chains;
  brave_wallet::GetAllChains(prefs(), &expected_chains);
  bool callback_is_called = false;
  controller.GetAllNetworks(base::BindLambdaForTesting(
      [&callback_is_called,
       &expected_chains](std::vector<mojom::EthereumChainPtr> chains) {
        EXPECT_EQ(expected_chains.size(), chains.size());

        for (size_t i = 0; i < chains.size(); i++) {
          ASSERT_TRUE(chains.at(i).Equals(expected_chains.at(i)));
        }
        callback_is_called = true;
      }));
  ASSERT_TRUE(callback_is_called);
  base::RunLoop().RunUntilIdle();
}

TEST_F(EthJsonRpcControllerUnitTest, ResolveENSDomain) {
  EthJsonRpcController controller(shared_url_loader_factory(), prefs());
  controller.SetNetwork(brave_wallet::mojom::kLocalhostChainId);
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

TEST_F(EthJsonRpcControllerUnitTest, ResetCustomChains) {
  std::vector<base::Value> values;
  brave_wallet::mojom::EthereumChain chain(
      "0x1", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11);
  auto chain_ptr = chain.Clone();
  values.push_back(brave_wallet::EthereumChainToValue(chain_ptr));
  UpdateCustomNetworks(prefs(), &values);

  std::vector<brave_wallet::mojom::EthereumChainPtr> custom_chains;
  GetAllCustomChains(prefs(), &custom_chains);
  ASSERT_FALSE(custom_chains.empty());
  custom_chains.clear();
  ASSERT_TRUE(custom_chains.empty());

  KeyringController controller(prefs());
  controller.Reset();
  GetAllCustomChains(prefs(), &custom_chains);
  ASSERT_TRUE(custom_chains.empty());
}

TEST_F(EthJsonRpcControllerUnitTest, AddEthereumChainApproved) {
  brave_wallet::mojom::EthereumChain chain(
      "0x111", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11);
  EthJsonRpcController controller(shared_url_loader_factory(), prefs());

  base::RunLoop loop;
  std::unique_ptr<FakeEthereumChainObserver> observer(
      new FakeEthereumChainObserver(loop.QuitClosure(), "0x111", true));

  controller.AddObserver(observer->GetReceiver());

  mojo::PendingRemote<brave_wallet::mojom::EthJsonRpcControllerObserver>
      receiver;
  mojo::MakeSelfOwnedReceiver(std::move(observer),
                              receiver.InitWithNewPipeAndPassReceiver());

  bool callback_is_called = false;
  bool expected = true;
  ASSERT_FALSE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
  controller.AddEthereumChain(
      chain.Clone(), GURL("https://brave.com"),
      base::BindLambdaForTesting([&callback_is_called, &expected](
                                     const std::string& chain_id, bool added) {
        ASSERT_FALSE(chain_id.empty());
        EXPECT_EQ(added, expected);
        callback_is_called = true;
      }));
  controller.AddEthereumChainRequestCompleted("0x111", true);
  loop.Run();
  ASSERT_TRUE(callback_is_called);
  ASSERT_TRUE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
  callback_is_called = false;
  controller.AddEthereumChainRequestCompleted("0x111", true);
  ASSERT_FALSE(callback_is_called);
}

TEST_F(EthJsonRpcControllerUnitTest, AddEthereumChainRejected) {
  brave_wallet::mojom::EthereumChain chain(
      "0x111", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11);
  EthJsonRpcController controller(shared_url_loader_factory(), prefs());

  base::RunLoop loop;
  std::unique_ptr<FakeEthereumChainObserver> observer(
      new FakeEthereumChainObserver(loop.QuitClosure(), "0x111", false));

  controller.AddObserver(observer->GetReceiver());

  mojo::PendingRemote<brave_wallet::mojom::EthJsonRpcControllerObserver>
      receiver;
  mojo::MakeSelfOwnedReceiver(std::move(observer),
                              receiver.InitWithNewPipeAndPassReceiver());

  bool callback_is_called = false;
  bool expected = true;
  ASSERT_FALSE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
  controller.AddEthereumChain(
      chain.Clone(), GURL("https://brave.com"),
      base::BindLambdaForTesting([&callback_is_called, &expected](
                                     const std::string& chain_id, bool added) {
        ASSERT_FALSE(chain_id.empty());
        EXPECT_EQ(added, expected);
        callback_is_called = true;
      }));
  controller.AddEthereumChainRequestCompleted("0x111", false);
  loop.Run();
  ASSERT_TRUE(callback_is_called);
  ASSERT_FALSE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
  callback_is_called = false;
  controller.AddEthereumChainRequestCompleted("0x111", true);
  ASSERT_FALSE(callback_is_called);
  ASSERT_FALSE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
}

TEST_F(EthJsonRpcControllerUnitTest, AddEthereumChainError) {
  brave_wallet::mojom::EthereumChain chain(
      "0x111", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11);
  EthJsonRpcController controller(shared_url_loader_factory(), prefs());

  bool callback_is_called = false;
  bool expected = true;
  ASSERT_FALSE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
  controller.AddEthereumChain(
      chain.Clone(), GURL("https://brave.com"),
      base::BindLambdaForTesting([&callback_is_called, &expected](
                                     const std::string& chain_id, bool added) {
        ASSERT_FALSE(chain_id.empty());
        EXPECT_EQ(added, expected);
        callback_is_called = true;
      }));
  ASSERT_TRUE(callback_is_called);
  callback_is_called = false;

  // other chain, same origin
  brave_wallet::mojom::EthereumChain chain2(
      "0x222", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11);

  bool second_callback_is_called = false;
  bool second_expected = false;
  controller.AddEthereumChain(
      chain2.Clone(), GURL("https://brave.com"),
      base::BindLambdaForTesting([&second_callback_is_called, &second_expected](
                                     const std::string& chain_id, bool added) {
        ASSERT_FALSE(chain_id.empty());
        EXPECT_EQ(added, second_expected);
        second_callback_is_called = true;
      }));
  ASSERT_FALSE(callback_is_called);
  ASSERT_TRUE(second_callback_is_called);
  second_callback_is_called = false;

  // same chain, other origin
  bool third_callback_is_called = false;
  bool third_expected = false;
  controller.AddEthereumChain(
      chain.Clone(), GURL("https://others.com"),
      base::BindLambdaForTesting([&third_callback_is_called, &third_expected](
                                     const std::string& chain_id, bool added) {
        ASSERT_FALSE(chain_id.empty());
        EXPECT_EQ(added, third_expected);
        third_callback_is_called = true;
      }));
  ASSERT_FALSE(callback_is_called);
  ASSERT_FALSE(second_callback_is_called);
  ASSERT_TRUE(third_callback_is_called);
}

TEST_F(EthJsonRpcControllerUnitTest, StartWithNetwork) {
  ValidateStartWithNetwork(std::string(), std::string());
  ValidateStartWithNetwork("SomeBadChainId", std::string());
  ValidateStartWithNetwork(brave_wallet::mojom::kRopstenChainId,
                           brave_wallet::mojom::kRopstenChainId);
}

}  // namespace brave_wallet
