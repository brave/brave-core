/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/values.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/ui/webui/settings/brave_wallet_handler.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "components/prefs/testing_pref_store.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_ui.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/l10n_util.h"

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

}  // namespace

class TestBraveWalletHandler : public BraveWalletHandler {
 public:
  TestBraveWalletHandler()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    TestingProfile::Builder builder;

    profile_ = builder.Build();
    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_.get()));

    test_web_ui_.set_web_contents(web_contents_.get());
    set_web_ui(&test_web_ui_);
    auto* json_rpc_service =
        brave_wallet::JsonRpcServiceFactory::GetServiceForContext(
            profile_.get());

    json_rpc_service->SetAPIRequestHelperForTesting(shared_url_loader_factory_);
  }

  ~TestBraveWalletHandler() override {
    // The test handler unusually owns its own TestWebUI, so we make sure to
    // unbind it from the base class before the derived class is destroyed.
    set_web_ui(nullptr);
  }
  void SetEthChainIdInterceptor(const std::string& network_url,
                                const std::string& chain_id) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, network_url, chain_id](const network::ResourceRequest& request) {
          base::StringPiece request_string(request.request_body->elements()
                                               ->at(0)
                                               .As<network::DataElementBytes>()
                                               .AsStringPiece());
          url_loader_factory_.ClearResponses();
          if (request_string.find("eth_chainId") != std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url, "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" +
                                 chain_id + "\"}");
          }
        }));
  }

  void RegisterMessages() override {}

  void RemoveEthereumChain(base::Value::ConstListView args) {
    BraveWalletHandler::RemoveEthereumChain(args);
  }
  void GetCustomNetworksList(base::Value::ConstListView args) {
    BraveWalletHandler::GetCustomNetworksList(args);
  }
  void AddEthereumChain(base::Value::ConstListView args) {
    BraveWalletHandler::AddEthereumChain(args);
  }
  void SetActiveNetwork(base::Value::ConstListView args) {
    BraveWalletHandler::SetActiveNetwork(args);
  }
  content::TestWebUI* web_ui() { return &test_web_ui_; }
  PrefService* prefs() { return profile_->GetPrefs(); }

 protected:
  std::unique_ptr<brave_wallet::JsonRpcService> json_rpc_service_;

 private:
  content::BrowserTaskEnvironment browser_task_environment;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<content::WebContents> web_contents_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  content::TestWebUI test_web_ui_;
};

TEST(TestBraveWalletHandler, RemoveEthereumChain) {
  TestBraveWalletHandler handler;

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
  UpdateCustomNetworks(handler.prefs(), &values);
  {
    std::vector<brave_wallet::mojom::EthereumChainPtr> result;
    brave_wallet::GetAllCustomChains(handler.prefs(), &result);
    EXPECT_EQ(result.size(), 2u);
  }

  auto args = base::ListValue();
  args.Append(base::Value("id"));
  args.Append(base::Value("chain_id"));

  handler.RemoveEthereumChain(args.GetList());
  const auto& data = *handler.web_ui()->call_data()[0];
  ASSERT_TRUE(data.arg3()->is_bool());
  EXPECT_EQ(data.arg3()->GetBool(), true);
  {
    std::vector<brave_wallet::mojom::EthereumChainPtr> result;
    brave_wallet::GetAllCustomChains(handler.prefs(), &result);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]->chain_id, "chain_id2");
  }
}

TEST(TestBraveWalletHandler, AddEthereumChain) {
  TestBraveWalletHandler handler;
  brave_wallet::mojom::EthereumChain chain1(
      "0x999", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol", "symbol_name", 11, false);
  auto chain_ptr1 = chain1.Clone();

  {
    std::vector<brave_wallet::mojom::EthereumChainPtr> result;
    brave_wallet::GetAllCustomChains(handler.prefs(), &result);
    EXPECT_EQ(result.size(), 0u);
  }

  auto args = base::ListValue();
  args.Append(base::Value("id"));
  auto value = brave_wallet::EthereumChainToValue(chain_ptr1);
  std::string json_string;
  base::JSONWriter::Write(value, &json_string);
  args.Append(base::Value(json_string));
  handler.SetEthChainIdInterceptor(chain1.rpc_urls.front(), "0x999");
  base::RunLoop loop;
  handler.SetChainCallbackForTesting(loop.QuitClosure());
  handler.AddEthereumChain(args.GetList());
  loop.Run();
  {
    std::vector<brave_wallet::mojom::EthereumChainPtr> result;
    brave_wallet::GetAllCustomChains(handler.prefs(), &result);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], chain1.Clone());
  }

  const base::Value* assets_pref =
      handler.prefs()->GetDictionary(kBraveWalletUserAssets);
  const base::Value* list = assets_pref->FindKey("0x999");
  ASSERT_TRUE(list->is_list());
  base::Value::ConstListView asset_list = list->GetList();
  ASSERT_EQ(asset_list.size(), 1u);

  EXPECT_EQ(*asset_list[0].FindStringKey("contract_address"), "");
  EXPECT_EQ(*asset_list[0].FindStringKey("name"), "symbol_name");
  EXPECT_EQ(*asset_list[0].FindStringKey("symbol"), "symbol");
  EXPECT_EQ(*asset_list[0].FindBoolKey("is_erc20"), false);
  EXPECT_EQ(*asset_list[0].FindBoolKey("is_erc721"), false);
  EXPECT_EQ(*asset_list[0].FindIntKey("decimals"), 11);
  EXPECT_EQ(*asset_list[0].FindStringKey("logo"), "https://url1.com");
  EXPECT_EQ(*asset_list[0].FindBoolKey("visible"), true);

  auto args2 = base::ListValue();
  args2.Append(base::Value("id"));
  args2.Append(base::Value(json_string));
  handler.AddEthereumChain(args2.GetList());
  const auto& data = *handler.web_ui()->call_data()[1];
  ASSERT_TRUE(data.arg1()->is_string());
  EXPECT_EQ(data.arg1()->GetString(), "id");

  auto arg3_list = data.arg3()->GetList();
  ASSERT_EQ(arg3_list.size(), 2UL);
  EXPECT_EQ(arg3_list[0].GetBool(), false);
  std::string error_message =
      l10n_util::GetStringUTF8(IDS_SETTINGS_WALLET_NETWORKS_EXISTS);
  EXPECT_EQ(arg3_list[1].GetString(), error_message);
  {
    std::vector<brave_wallet::mojom::EthereumChainPtr> result;
    brave_wallet::GetAllCustomChains(handler.prefs(), &result);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], chain1.Clone());
  }
}

TEST(TestBraveWalletHandler, AddEthereumChainWrongNetwork) {
  TestBraveWalletHandler handler;
  brave_wallet::mojom::EthereumChain chain1(
      "0x999", "chain_name", {"https://url1.com"}, {"https://url2.com"},
      {"https://url3.com"}, "symbol", "symbol_name", 11, false);
  auto chain_ptr1 = chain1.Clone();

  {
    std::vector<brave_wallet::mojom::EthereumChainPtr> result;
    brave_wallet::GetAllCustomChains(handler.prefs(), &result);
    EXPECT_EQ(result.size(), 0u);
  }

  auto args = base::ListValue();
  args.Append(base::Value("id"));
  auto value = brave_wallet::EthereumChainToValue(chain_ptr1);
  std::string json_string;
  base::JSONWriter::Write(value, &json_string);
  args.Append(base::Value(json_string));
  handler.SetEthChainIdInterceptor(chain1.rpc_urls.front(), "0x11");
  base::RunLoop loop;
  handler.SetChainCallbackForTesting(loop.QuitClosure());
  handler.AddEthereumChain(args.GetList());
  loop.Run();
  const auto& data = *handler.web_ui()->call_data()[0];
  ASSERT_TRUE(data.arg1()->is_string());
  EXPECT_EQ(data.arg1()->GetString(), "id");

  auto arg3_list = data.arg3()->GetList();
  ASSERT_EQ(arg3_list.size(), 2UL);
  EXPECT_EQ(arg3_list[0].GetBool(), false);
  std::string error_message =
      l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                base::ASCIIToUTF16(chain1.rpc_urls.front()));
  EXPECT_EQ(arg3_list[1].GetString(), error_message);
}
TEST(TestBraveWalletHandler, AddEthereumChainFail) {
  TestBraveWalletHandler handler;

  {
    std::vector<brave_wallet::mojom::EthereumChainPtr> result;
    brave_wallet::GetAllCustomChains(handler.prefs(), &result);
    EXPECT_EQ(result.size(), 0u);
  }

  auto args = base::ListValue();
  args.Append(base::Value("id"));
  args.Append(base::Value(""));
  handler.AddEthereumChain(args.GetList());

  {
    std::vector<brave_wallet::mojom::EthereumChainPtr> result;
    brave_wallet::GetAllCustomChains(handler.prefs(), &result);
    EXPECT_EQ(result.size(), 0u);
  }

  auto args2 = base::ListValue();
  args2.Append(base::Value("id"));
  args2.Append(base::Value(R"({"chain_name\":"a","rpcUrl":["http://u.c"]})"));
  handler.AddEthereumChain(args2.GetList());
  const auto& data = *handler.web_ui()->call_data()[0];
  ASSERT_TRUE(data.arg1()->is_string());
  EXPECT_EQ(data.arg1()->GetString(), "id");

  auto arg3_list = data.arg3()->GetList();
  ASSERT_EQ(arg3_list.size(), 2UL);
  EXPECT_EQ(arg3_list[0].GetBool(), false);
  std::string error_message =
      l10n_util::GetStringUTF8(IDS_SETTINGS_WALLET_NETWORKS_SUMBISSION_FAILED);
  EXPECT_EQ(arg3_list[1].GetString(), error_message);

  {
    std::vector<brave_wallet::mojom::EthereumChainPtr> result;
    brave_wallet::GetAllCustomChains(handler.prefs(), &result);
    EXPECT_EQ(result.size(), 0u);
  }
}

TEST(TestBraveWalletHandler, GetNetworkList) {
  TestBraveWalletHandler handler;
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
  UpdateCustomNetworks(handler.prefs(), &values);
  {
    std::vector<brave_wallet::mojom::EthereumChainPtr> result;
    brave_wallet::GetAllCustomChains(handler.prefs(), &result);
    EXPECT_EQ(result.size(), 2u);
  }
  auto args = base::ListValue();
  args.Append(base::Value("id"));
  handler.GetCustomNetworksList(args.GetList());
  const auto& data = *handler.web_ui()->call_data()[0];
  ASSERT_TRUE(data.arg1()->is_string());
  EXPECT_EQ(data.arg1()->GetString(), "id");
  ASSERT_TRUE(data.arg3()->is_string());
  absl::optional<base::Value> expected_list =
      base::JSONReader::Read(data.arg3()->GetString());
  ASSERT_TRUE(expected_list);
  auto expected_chain1 =
      brave_wallet::ValueToEthereumChain(expected_list.value().GetList()[0]);
  ASSERT_TRUE(expected_chain1);
  EXPECT_EQ(expected_chain1.value(), chain1);

  auto expected_chain2 =
      brave_wallet::ValueToEthereumChain(expected_list.value().GetList()[1]);
  ASSERT_TRUE(expected_chain2);
  EXPECT_EQ(expected_chain2.value(), chain2);
}

TEST(TestBraveWalletHandler, SetActiveNetwork) {
  TestBraveWalletHandler handler;

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
  UpdateCustomNetworks(handler.prefs(), &values);
  {
    std::vector<brave_wallet::mojom::EthereumChainPtr> result;
    brave_wallet::GetAllCustomChains(handler.prefs(), &result);
    EXPECT_EQ(result.size(), 2u);
  }
  handler.prefs()->SetString(kBraveWalletCurrentChainId, "chain_id");
  {
    auto args = base::ListValue();
    args.Append(base::Value("id"));
    args.Append(base::Value("chain_id2"));

    handler.SetActiveNetwork(args.GetList());
    const auto& data = *handler.web_ui()->call_data()[0];
    ASSERT_TRUE(data.arg3()->is_bool());
    EXPECT_EQ(data.arg3()->GetBool(), true);

    EXPECT_EQ(handler.prefs()->GetString(kBraveWalletCurrentChainId),
              "chain_id2");
  }
  {
    auto args = base::ListValue();
    args.Append(base::Value("id"));
    args.Append(base::Value("unknown_chain_id"));

    handler.SetActiveNetwork(args.GetList());
    const auto& data = *handler.web_ui()->call_data()[1];
    ASSERT_TRUE(data.arg3()->is_bool());
    EXPECT_EQ(data.arg3()->GetBool(), false);

    EXPECT_EQ(handler.prefs()->GetString(kBraveWalletCurrentChainId),
              "chain_id2");
  }
}
