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
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
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

void OnRequestResponse(
    bool* callback_called,
    bool expected_success,
    const std::string& expected_response,
    const int status,
    const std::string& response,
    const base::flat_map<std::string, std::string>& headers) {
  *callback_called = true;
  bool success = status == 200;
  EXPECT_EQ(expected_response, response);
  EXPECT_EQ(expected_success, success);
}

void OnStringResponse(bool* callback_called,
                      bool expected_success,
                      const std::string& expected_response,
                      bool success,
                      const std::string& response) {
  *callback_called = true;
  EXPECT_EQ(expected_response, response);
  EXPECT_EQ(expected_success, success);
}

void OnBoolResponse(bool* callback_called,
                    bool expected_success,
                    bool expected_response,
                    bool success,
                    bool response) {
  *callback_called = true;
  EXPECT_EQ(expected_response, response);
  EXPECT_EQ(expected_success, success);
}

class TestEthJsonRpcControllerObserver
    : public brave_wallet::mojom::EthJsonRpcControllerObserver {
 public:
  TestEthJsonRpcControllerObserver(base::OnceClosure callback,
                                   const std::string& expected_chain_id,
                                   bool expected_error_empty) {
    callback_ = std::move(callback);
    expected_chain_id_ = expected_chain_id;
    expected_error_empty_ = expected_error_empty;
  }

  TestEthJsonRpcControllerObserver(const std::string& expected_chain_id,
                                   bool expected_is_eip1559) {
    expected_chain_id_ = expected_chain_id;
    expected_is_eip1559_ = expected_is_eip1559;
  }

  void Reset(const std::string& expected_chain_id, bool expected_is_eip1559) {
    expected_chain_id_ = expected_chain_id;
    expected_is_eip1559_ = expected_is_eip1559;
    chain_changed_called_ = false;
    is_eip1559_changed_called_ = false;
  }

  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override {
    EXPECT_EQ(chain_id, expected_chain_id_);
    EXPECT_EQ(error.empty(), expected_error_empty_);
    std::move(callback_).Run();
  }

  void ChainChangedEvent(const std::string& chain_id) override {
    chain_changed_called_ = true;
    EXPECT_EQ(chain_id, expected_chain_id_);
  }

  void OnIsEip1559Changed(const std::string& chain_id,
                          bool is_eip1559) override {
    is_eip1559_changed_called_ = true;
    EXPECT_EQ(chain_id, expected_chain_id_);
    EXPECT_EQ(is_eip1559, expected_is_eip1559_);
  }

  bool is_eip1559_changed_called() { return is_eip1559_changed_called_; }
  bool chain_changed_called() { return chain_changed_called_; }

  ::mojo::PendingRemote<brave_wallet::mojom::EthJsonRpcControllerObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  base::OnceClosure callback_;
  std::string expected_chain_id_;
  bool expected_error_empty_;
  bool expected_is_eip1559_;
  bool chain_changed_called_ = false;
  bool is_eip1559_changed_called_ = false;
  mojo::Receiver<brave_wallet::mojom::EthJsonRpcControllerObserver>
      observer_receiver_{this};
};

}  // namespace

namespace brave_wallet {

class EthJsonRpcControllerUnitTest : public testing::Test {
 public:
  EthJsonRpcControllerUnitTest()
      : browser_context_(new content::TestBrowserContext()),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(
              brave_wallet::GetNetworkURL(prefs(), mojom::kLocalhostChainId)
                  .spec(),
              "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
              "\"0x000000000000000000000000000000000000000000000000000000000000"
              "0020000000000000000000000000000000000000000000000000000000000000"
              "0026e3010170122008ab7bf21b73828364305ef6b7c676c1f5a73e18ab4f93be"
              "ec7e21e0bc84010e000000000000000000000000000000000000000000000000"
              "0000\"}");
        }));

    user_prefs::UserPrefs::Set(browser_context_.get(), &prefs_);
    brave_wallet::RegisterProfilePrefs(prefs_.registry());

    rpc_controller_.reset(
        new EthJsonRpcController(shared_url_loader_factory_, &prefs_));
    rpc_controller_->SetNetwork(brave_wallet::mojom::kLocalhostChainId);
  }

  ~EthJsonRpcControllerUnitTest() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }

  PrefService* prefs() { return &prefs_; }

  bool GetIsEip1559FromPrefs(const std::string& chain_id) {
    if (chain_id == mojom::kLocalhostChainId)
      return prefs()->GetBoolean(kSupportEip1559OnLocalhostChain);
    const base::ListValue* custom_networks =
        prefs()->GetList(kBraveWalletCustomNetworks);
    if (!custom_networks)
      return false;

    for (const auto& chain : custom_networks->GetList()) {
      if (!chain.is_dict())
        continue;

      const std::string* id = chain.FindStringKey("chainId");
      if (!id || *id != chain_id)
        continue;

      return chain.FindBoolKey("is_eip1559").value_or(false);
    }

    return false;
  }

  void SetRegistrarResponse() {
    auto localhost_url_spec =
        brave_wallet::GetNetworkURL(prefs(), mojom::kLocalhostChainId).spec();

    url_loader_factory_.AddResponse(
        localhost_url_spec,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x00000"
        "0000000000000000000226159d592e2b063810a10ebf6dcbada94ed68b8\"}");
  }

  void SetInterceptor(const std::string& expected_method,
                      const std::string& expected_cache_header,
                      const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, expected_method, expected_cache_header,
         content](const network::ResourceRequest& request) {
          std::string header_value(100, '\0');
          EXPECT_TRUE(request.headers.GetHeader("x-brave-key", &header_value));
          EXPECT_EQ(BRAVE_SERVICES_KEY, header_value);
          EXPECT_TRUE(request.headers.GetHeader("X-Eth-Method", &header_value));
          EXPECT_EQ(expected_method, header_value);
          if (expected_method == "eth_blockNumber") {
            EXPECT_TRUE(
                request.headers.GetHeader("X-Eth-Block", &header_value));
            EXPECT_EQ(expected_cache_header, header_value);
          } else if (expected_method == "eth_getBlockByNumber") {
            EXPECT_TRUE(
                request.headers.GetHeader("X-eth-get-block", &header_value));
            EXPECT_EQ(expected_cache_header, header_value);
          }
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetErrorInterceptor() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "",
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
  }

  void SetIsEip1559Interceptor(bool is_eip1559) {
    if (is_eip1559)
      SetInterceptor(
          "eth_getBlockByNumber", "latest,false",
          "{\"jsonrpc\":\"2.0\",\"id\": \"0\",\"result\": "
          "{\"baseFeePerGas\":\"0x181f22e7a9\", \"gasLimit\":\"0x6691b8\"}}");
    else
      SetInterceptor("eth_getBlockByNumber", "latest,false",
                     "{\"jsonrpc\":\"2.0\",\"id\": \"0\",\"result\": "
                     "{\"gasLimit\":\"0x6691b8\"}}");
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

 protected:
  std::unique_ptr<EthJsonRpcController> rpc_controller_;

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(EthJsonRpcControllerUnitTest, SetNetwork) {
  std::vector<mojom::EthereumChainPtr> networks;
  brave_wallet::GetAllKnownChains(prefs(), &networks);
  for (const auto& network : networks) {
    bool callback_is_called = false;
    rpc_controller_->SetNetwork(network->chain_id);
    EXPECT_EQ(network->chain_id,
              prefs()->GetString(kBraveWalletCurrentChainId));
    const std::string& expected_id = network->chain_id;
    rpc_controller_->GetChainId(base::BindLambdaForTesting(
        [&callback_is_called, &expected_id](const std::string& chain_id) {
          EXPECT_EQ(chain_id, expected_id);
          callback_is_called = true;
        }));
    ASSERT_TRUE(callback_is_called);

    callback_is_called = false;
    const std::string& expected_url = network->rpc_urls.front();
    rpc_controller_->GetNetworkUrl(base::BindLambdaForTesting(
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
      {"https://url1.com"}, "symbol_name", "symbol", 11, false);
  auto chain_ptr1 = chain1.Clone();
  values.push_back(brave_wallet::EthereumChainToValue(chain_ptr1));

  brave_wallet::mojom::EthereumChain chain2(
      "chain_id2", "chain_name2", {"https://url2.com"}, {"https://url2.com"},
      {"https://url2.com"}, "symbol_name2", "symbol2", 22, true);
  auto chain_ptr2 = chain2.Clone();
  values.push_back(brave_wallet::EthereumChainToValue(chain_ptr2));
  UpdateCustomNetworks(prefs(), &values);

  bool callback_is_called = false;
  rpc_controller_->SetNetwork(chain1.chain_id);
  const std::string& expected_id = chain1.chain_id;
  rpc_controller_->GetChainId(base::BindLambdaForTesting(
      [&callback_is_called, &expected_id](const std::string& chain_id) {
        EXPECT_EQ(chain_id, expected_id);
        callback_is_called = true;
      }));
  ASSERT_TRUE(callback_is_called);
  callback_is_called = false;
  const std::string& expected_url = chain1.rpc_urls.front();
  rpc_controller_->GetNetworkUrl(base::BindLambdaForTesting(
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
      {"https://url1.com"}, "symbol_name", "symbol", 11, false);
  auto chain_ptr1 = chain1.Clone();
  values.push_back(brave_wallet::EthereumChainToValue(chain_ptr1));

  brave_wallet::mojom::EthereumChain chain2(
      "chain_id2", "chain_name2", {"https://url2.com"}, {"https://url2.com"},
      {"https://url2.com"}, "symbol_name2", "symbol2", 22, true);
  auto chain_ptr2 = chain2.Clone();
  values.push_back(brave_wallet::EthereumChainToValue(chain_ptr2));
  UpdateCustomNetworks(prefs(), &values);

  std::vector<mojom::EthereumChainPtr> expected_chains;
  brave_wallet::GetAllChains(prefs(), &expected_chains);
  bool callback_is_called = false;
  rpc_controller_->GetAllNetworks(base::BindLambdaForTesting(
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
  rpc_controller_->SetNetwork(brave_wallet::mojom::kLocalhostChainId);
  SetRegistrarResponse();
  base::RunLoop run;
  rpc_controller_->EnsProxyReaderGetResolverAddress(
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
      {"https://url1.com"}, "symbol_name", "symbol", 11, false);
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
      {"https://url1.com"}, "symbol_name", "symbol", 11, false);

  base::RunLoop loop;
  std::unique_ptr<TestEthJsonRpcControllerObserver> observer(
      new TestEthJsonRpcControllerObserver(loop.QuitClosure(), "0x111", true));

  rpc_controller_->AddObserver(observer->GetReceiver());

  mojo::PendingRemote<brave_wallet::mojom::EthJsonRpcControllerObserver>
      receiver;
  mojo::MakeSelfOwnedReceiver(std::move(observer),
                              receiver.InitWithNewPipeAndPassReceiver());

  bool callback_is_called = false;
  bool expected = true;
  ASSERT_FALSE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
  rpc_controller_->AddEthereumChain(
      chain.Clone(), GURL("https://brave.com"),
      base::BindLambdaForTesting([&callback_is_called, &expected](
                                     const std::string& chain_id, bool added) {
        ASSERT_FALSE(chain_id.empty());
        EXPECT_EQ(added, expected);
        callback_is_called = true;
      }));
  rpc_controller_->AddEthereumChainRequestCompleted("0x111", true);
  loop.Run();
  ASSERT_TRUE(callback_is_called);
  ASSERT_TRUE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
  callback_is_called = false;
  rpc_controller_->AddEthereumChainRequestCompleted("0x111", true);
  ASSERT_FALSE(callback_is_called);
}

TEST_F(EthJsonRpcControllerUnitTest, AddEthereumChainRejected) {
  brave_wallet::mojom::EthereumChain chain(
      "0x111", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11, false);

  base::RunLoop loop;
  std::unique_ptr<TestEthJsonRpcControllerObserver> observer(
      new TestEthJsonRpcControllerObserver(loop.QuitClosure(), "0x111", false));

  rpc_controller_->AddObserver(observer->GetReceiver());

  mojo::PendingRemote<brave_wallet::mojom::EthJsonRpcControllerObserver>
      receiver;
  mojo::MakeSelfOwnedReceiver(std::move(observer),
                              receiver.InitWithNewPipeAndPassReceiver());

  bool callback_is_called = false;
  bool expected = true;
  ASSERT_FALSE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
  rpc_controller_->AddEthereumChain(
      chain.Clone(), GURL("https://brave.com"),
      base::BindLambdaForTesting([&callback_is_called, &expected](
                                     const std::string& chain_id, bool added) {
        ASSERT_FALSE(chain_id.empty());
        EXPECT_EQ(added, expected);
        callback_is_called = true;
      }));
  rpc_controller_->AddEthereumChainRequestCompleted("0x111", false);
  loop.Run();
  ASSERT_TRUE(callback_is_called);
  ASSERT_FALSE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
  callback_is_called = false;
  rpc_controller_->AddEthereumChainRequestCompleted("0x111", true);
  ASSERT_FALSE(callback_is_called);
  ASSERT_FALSE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
}

TEST_F(EthJsonRpcControllerUnitTest, AddEthereumChainError) {
  brave_wallet::mojom::EthereumChain chain(
      "0x111", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11, false);

  bool callback_is_called = false;
  bool expected = true;
  ASSERT_FALSE(brave_wallet::GetNetworkURL(prefs(), "0x111").is_valid());
  rpc_controller_->AddEthereumChain(
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
      {"https://url1.com"}, "symbol_name", "symbol", 11, false);

  bool second_callback_is_called = false;
  bool second_expected = false;
  rpc_controller_->AddEthereumChain(
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
  rpc_controller_->AddEthereumChain(
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

TEST_F(EthJsonRpcControllerUnitTest, Request) {
  bool callback_called = false;
  std::string request =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"eth_blockNumber\",\"params\":"
      "[]}";
  std::string expected_response =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0xb539d5\"}";
  SetInterceptor("eth_blockNumber", "true", expected_response);
  rpc_controller_->Request(
      request, true,
      base::BindOnce(&OnRequestResponse, &callback_called, true /* success */,
                     expected_response));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  request =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"eth_getBlockByNumber\","
      "\"params\":"
      "[\"0x5BAD55\",true]}";
  expected_response = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0xb539d5\"}";
  SetInterceptor("eth_getBlockByNumber", "0x5BAD55,true", expected_response);
  rpc_controller_->Request(
      request, true,
      base::BindOnce(&OnRequestResponse, &callback_called, true /* success */,
                     expected_response));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetErrorInterceptor();
  rpc_controller_->Request(request, true,
                           base::BindOnce(&OnRequestResponse, &callback_called,
                                          false /* success */, ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthJsonRpcControllerUnitTest, GetBalance) {
  bool callback_called = false;
  SetInterceptor("eth_getBalance", "",
                 "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0xb539d5\"}");
  rpc_controller_->GetBalance(
      "0x4e02f254184E904300e0775E4b8eeCB1",
      base::BindOnce(&OnStringResponse, &callback_called, true, "0xb539d5"));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetErrorInterceptor();
  rpc_controller_->GetBalance(
      "0x4e02f254184E904300e0775E4b8eeCB1",
      base::BindOnce(&OnStringResponse, &callback_called, false, ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthJsonRpcControllerUnitTest, GetERC20TokenBalance) {
  bool callback_called = false;
  SetInterceptor(
      "eth_call", "",
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x00000000000000000000000000000000000000000000000166e12cfce39a0000\""
      "}");

  rpc_controller_->GetERC20TokenBalance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0x4e02f254184E904300e0775E4b8eeCB1",
      base::BindOnce(&OnStringResponse, &callback_called, true,
                     "0x00000000000000000000000000000000000000000000000166e12cf"
                     "ce39a0000"));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetErrorInterceptor();
  rpc_controller_->GetERC20TokenBalance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0x4e02f254184E904300e0775E4b8eeCB1",
      base::BindOnce(&OnStringResponse, &callback_called, false, ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Invalid input should fail.
  callback_called = false;
  rpc_controller_->GetERC20TokenBalance(
      "", "", base::BindOnce(&OnStringResponse, &callback_called, false, ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthJsonRpcControllerUnitTest, GetERC20TokenAllowance) {
  bool callback_called = false;
  SetInterceptor(
      "eth_call", "",
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x00000000000000000000000000000000000000000000000166e12cfce39a0000\""
      "}");

  rpc_controller_->GetERC20TokenAllowance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a",
      base::BindOnce(&OnStringResponse, &callback_called, true,
                     "0x00000000000000000000000000000000000000000000000166e12cf"
                     "ce39a0000"));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetErrorInterceptor();
  rpc_controller_->GetERC20TokenAllowance(
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a",
      base::BindOnce(&OnStringResponse, &callback_called, false, ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Invalid input should fail.
  callback_called = false;
  rpc_controller_->GetERC20TokenAllowance(
      "", "", "",
      base::BindOnce(&OnStringResponse, &callback_called, false, ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthJsonRpcControllerUnitTest, UnstoppableDomainsProxyReaderGetMany) {
  bool callback_called = false;
  SetInterceptor(
      "eth_call", "",
      "{\"jsonrpc\":\"2.0\",\"id\": \"0\",\"result\": "
      "\"0x00000000000000000000000000000000000000000000000000000000000000200000"
      "000000000000000000000000000000000000000000000000000000000004000000000000"
      "000000000000000000000000000000000000000000000000008000000000000000000000"
      "000000000000000000000000000000000000000000a00000000000000000000000000000"
      "000000000000000000000000000000000100000000000000000000000000000000000000"
      "000000000000000000000000012000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "00000000002e516d5772644e4a574d62765278787a4c686f6a564b614244737753344b4e"
      "564d374c766a734e3751624472766b610000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000\"}");

  rpc_controller_->UnstoppableDomainsProxyReaderGetMany(
      "0xa6E7cEf2EDDEA66352Fd68E5915b60BDbb7309f5" /* contract_address */,
      "brave.crypto" /* domain */,
      {"dweb.ipfs.hash", "ipfs.html.value", "browser.redirect_url",
       "ipfs.redirect_domain.value"} /* keys */,
      base::BindOnce(
          &OnStringResponse, &callback_called, true,
          "0x0000000000000000000000000000000000000000000000000000000000000020"
          "0000000000000000000000000000000000000000000000000000000000000004"
          "0000000000000000000000000000000000000000000000000000000000000080"
          "00000000000000000000000000000000000000000000000000000000000000a0"
          "0000000000000000000000000000000000000000000000000000000000000100"
          "0000000000000000000000000000000000000000000000000000000000000120"
          "0000000000000000000000000000000000000000000000000000000000000000"
          "000000000000000000000000000000000000000000000000000000000000002e"
          "516d5772644e4a574d62765278787a4c686f6a564b614244737753344b4e564d"
          "374c766a734e3751624472766b61000000000000000000000000000000000000"
          "0000000000000000000000000000000000000000000000000000000000000000"
          "0000000000000000000000000000000000000000000000000000000000000000"));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetErrorInterceptor();
  rpc_controller_->UnstoppableDomainsProxyReaderGetMany(
      "0xa6E7cEf2EDDEA66352Fd68E5915b60BDbb7309f5" /* contract_address */,
      "brave.crypto" /* domain */,
      {"dweb.ipfs.hash", "ipfs.html.value", "browser.redirect_url",
       "ipfs.redirect_domain.value"} /* keys */,
      base::BindOnce(&OnStringResponse, &callback_called, false, ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  rpc_controller_->UnstoppableDomainsProxyReaderGetMany(
      "", "", std::vector<std::string>(),
      base::BindOnce(&OnStringResponse, &callback_called, false, ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthJsonRpcControllerUnitTest, GetIsEip1559) {
  bool callback_called = false;

  SetIsEip1559Interceptor(true);
  rpc_controller_->GetIsEip1559(
      base::BindOnce(&OnBoolResponse, &callback_called, true, true));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetIsEip1559Interceptor(false);
  rpc_controller_->GetIsEip1559(
      base::BindOnce(&OnBoolResponse, &callback_called, true, false));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  callback_called = false;
  SetErrorInterceptor();
  rpc_controller_->GetIsEip1559(
      base::BindOnce(&OnBoolResponse, &callback_called, false, false));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthJsonRpcControllerUnitTest, UpdateIsEip1559NotCalledForKnownChains) {
  TestEthJsonRpcControllerObserver observer(mojom::kMainnetChainId, false);
  rpc_controller_->AddObserver(observer.GetReceiver());

  rpc_controller_->SetNetwork(brave_wallet::mojom::kMainnetChainId);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.is_eip1559_changed_called());
}

TEST_F(EthJsonRpcControllerUnitTest, UpdateIsEip1559LocalhostChain) {
  TestEthJsonRpcControllerObserver observer(mojom::kLocalhostChainId, true);
  rpc_controller_->AddObserver(observer.GetReceiver());

  // Switching to localhost should update is_eip1559 to true when is_eip1559 is
  // true in the RPC response.
  EXPECT_FALSE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));
  SetIsEip1559Interceptor(true);

  rpc_controller_->SetNetwork(mojom::kLocalhostChainId);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.chain_changed_called());
  EXPECT_TRUE(observer.is_eip1559_changed_called());
  EXPECT_TRUE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));

  // Switching to localhost should update is_eip1559 to false when is_eip1559
  // is false in the RPC response.
  observer.Reset(mojom::kLocalhostChainId, false);
  SetIsEip1559Interceptor(false);

  rpc_controller_->SetNetwork(mojom::kLocalhostChainId);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.chain_changed_called());
  EXPECT_TRUE(observer.is_eip1559_changed_called());
  EXPECT_FALSE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));

  // Switch to localhost again without changing is_eip1559 should not trigger
  // event.
  observer.Reset(mojom::kLocalhostChainId, false);
  EXPECT_FALSE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));
  SetIsEip1559Interceptor(false);

  rpc_controller_->SetNetwork(mojom::kLocalhostChainId);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.chain_changed_called());
  EXPECT_FALSE(observer.is_eip1559_changed_called());
  EXPECT_FALSE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));

  // OnEip1559Changed will not be called if RPC fails.
  observer.Reset(mojom::kLocalhostChainId, false);
  SetErrorInterceptor();

  rpc_controller_->SetNetwork(mojom::kLocalhostChainId);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.chain_changed_called());
  EXPECT_FALSE(observer.is_eip1559_changed_called());
  EXPECT_FALSE(GetIsEip1559FromPrefs(mojom::kLocalhostChainId));
}

TEST_F(EthJsonRpcControllerUnitTest, UpdateIsEip1559CustomChain) {
  std::vector<base::Value> values;
  brave_wallet::mojom::EthereumChain chain1(
      "chain_id", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11, false);
  auto chain_ptr1 = chain1.Clone();
  values.push_back(brave_wallet::EthereumChainToValue(chain_ptr1));

  brave_wallet::mojom::EthereumChain chain2(
      "chain_id2", "chain_name2", {"https://url2.com"}, {"https://url2.com"},
      {"https://url2.com"}, "symbol_name2", "symbol2", 22, true);
  auto chain_ptr2 = chain2.Clone();
  values.push_back(brave_wallet::EthereumChainToValue(chain_ptr2));
  UpdateCustomNetworks(prefs(), &values);

  // Switch to chain1 should trigger is_eip1559 being updated to true when
  // is_eip1559 is true in the RPC response.
  TestEthJsonRpcControllerObserver observer(chain1.chain_id, true);
  rpc_controller_->AddObserver(observer.GetReceiver());

  EXPECT_FALSE(GetIsEip1559FromPrefs(chain1.chain_id));
  SetIsEip1559Interceptor(true);

  rpc_controller_->SetNetwork(chain1.chain_id);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.chain_changed_called());
  EXPECT_TRUE(observer.is_eip1559_changed_called());
  EXPECT_TRUE(GetIsEip1559FromPrefs(chain1.chain_id));

  // Switch to chain2 should trigger is_eip1559 being updated to false when
  // is_eip1559 is false in the RPC response.
  observer.Reset(chain2.chain_id, false);
  EXPECT_TRUE(GetIsEip1559FromPrefs(chain2.chain_id));
  SetIsEip1559Interceptor(false);

  rpc_controller_->SetNetwork(chain2.chain_id);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.chain_changed_called());
  EXPECT_TRUE(observer.is_eip1559_changed_called());
  EXPECT_FALSE(GetIsEip1559FromPrefs(chain2.chain_id));

  // Switch to chain2 again without changing is_eip1559 should not trigger
  // event.
  observer.Reset(chain2.chain_id, false);
  EXPECT_FALSE(GetIsEip1559FromPrefs(chain2.chain_id));
  SetIsEip1559Interceptor(false);

  rpc_controller_->SetNetwork(chain2.chain_id);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.chain_changed_called());
  EXPECT_FALSE(observer.is_eip1559_changed_called());
  EXPECT_FALSE(GetIsEip1559FromPrefs(chain2.chain_id));

  // OnEip1559Changed will not be called if RPC fails.
  observer.Reset(chain2.chain_id, false);
  SetErrorInterceptor();

  rpc_controller_->SetNetwork(chain2.chain_id);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.chain_changed_called());
  EXPECT_FALSE(observer.is_eip1559_changed_called());
  EXPECT_FALSE(GetIsEip1559FromPrefs(chain2.chain_id));
}

}  // namespace brave_wallet
