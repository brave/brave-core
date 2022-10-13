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
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
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
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/l10n_util.h"

using brave_wallet::mojom::CoinType;

namespace {

void UpdateCustomNetworks(PrefService* prefs,
                          CoinType coin,
                          std::vector<base::Value::Dict>* values) {
  DictionaryPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::Value::Dict& dict = update.Get()->GetDict();
  base::Value* list = dict.Find(brave_wallet::GetPrefKeyForCoinType(coin));
  if (!list) {
    list = dict.Set(brave_wallet::GetPrefKeyForCoinType(coin),
                    base::Value::List());
  }
  ASSERT_TRUE(list);
  auto& list_value = list->GetList();
  list_value.clear();
  for (auto& it : *values) {
    list_value.Append(std::move(it));
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
  void SetEthChainIdInterceptor(const GURL& network_url,
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
                network_url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" + chain_id +
                    "\"}");
          }
        }));
  }

  std::vector<brave_wallet::mojom::NetworkInfoPtr> GetAllEthCustomChains() {
    return brave_wallet::GetAllCustomChains(prefs(), CoinType::ETH);
  }

  std::vector<brave_wallet::mojom::NetworkInfoPtr> GetAllCustomChains(
      brave_wallet::mojom::CoinType coin) {
    return brave_wallet::GetAllCustomChains(prefs(), coin);
  }

  void RegisterMessages() override {}

  void RemoveChain(const base::Value::List& args) {
    BraveWalletHandler::RemoveChain(args);
  }
  void ResetChain(const base::Value::List& args) {
    BraveWalletHandler::ResetChain(args);
  }
  void GetNetworksList(const base::Value::List& args) {
    BraveWalletHandler::GetNetworksList(args);
  }
  void AddChain(const base::Value::List& args) {
    BraveWalletHandler::AddChain(args);
  }
  void SetActiveNetwork(const base::Value::List& args) {
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
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  content::TestWebUI test_web_ui_;
};

TEST(TestBraveWalletHandler, RemoveChain) {
  TestBraveWalletHandler handler;

  std::vector<base::Value::Dict> values;
  brave_wallet::mojom::NetworkInfo chain1 = brave_wallet::GetTestNetworkInfo1();
  values.push_back(brave_wallet::NetworkInfoToValue(chain1));

  brave_wallet::mojom::NetworkInfo chain2 = brave_wallet::GetTestNetworkInfo2();
  values.push_back(brave_wallet::NetworkInfoToValue(chain2));
  UpdateCustomNetworks(handler.prefs(), CoinType::ETH, &values);
  EXPECT_EQ(handler.GetAllEthCustomChains().size(), 2u);

  base::Value::List args;
  args.Append(base::Value("id"));
  args.Append(base::Value("chain_id"));
  args.Append(base::Value(static_cast<int>(CoinType::ETH)));

  handler.RemoveChain(args);
  const auto& data = *handler.web_ui()->call_data()[0];
  ASSERT_TRUE(data.arg3()->is_bool());
  EXPECT_EQ(data.arg3()->GetBool(), true);
  ASSERT_EQ(handler.GetAllEthCustomChains().size(), 1u);
  EXPECT_EQ(handler.GetAllEthCustomChains()[0]->chain_id, "chain_id2");
}

TEST(TestBraveWalletHandler, ResetChain) {
  TestBraveWalletHandler handler;

  std::vector<base::Value::Dict> values;
  brave_wallet::mojom::NetworkInfo chain1 = brave_wallet::GetTestNetworkInfo1(
      brave_wallet::mojom::kPolygonMainnetChainId);
  values.push_back(brave_wallet::NetworkInfoToValue(chain1));

  EXPECT_EQ(handler.GetAllEthCustomChains().size(), 0u);
  UpdateCustomNetworks(handler.prefs(), CoinType::ETH, &values);
  EXPECT_EQ(handler.GetAllEthCustomChains().size(), 1u);

  base::Value::List args;
  args.Append(base::Value("id"));
  args.Append(base::Value(brave_wallet::mojom::kPolygonMainnetChainId));
  args.Append(base::Value(static_cast<int>(CoinType::ETH)));

  handler.ResetChain(args);
  const auto& data = *handler.web_ui()->call_data()[0];
  ASSERT_TRUE(data.arg3()->is_bool());
  EXPECT_EQ(data.arg3()->GetBool(), true);
  EXPECT_EQ(handler.GetAllEthCustomChains().size(), 0u);
}

TEST(TestBraveWalletHandler, AddChain) {
  TestBraveWalletHandler handler;
  brave_wallet::mojom::NetworkInfo chain1 =
      brave_wallet::GetTestNetworkInfo1("0x999");
  EXPECT_EQ(handler.GetAllEthCustomChains().size(), 0u);

  base::Value::List args;
  args.Append(base::Value("id"));
  args.Append(brave_wallet::NetworkInfoToValue(chain1));
  handler.SetEthChainIdInterceptor(brave_wallet::GetActiveEndpointUrl(chain1),
                                   "0x999");
  base::RunLoop loop;
  handler.SetChainCallbackForTesting(loop.QuitClosure());
  handler.AddChain(args);
  loop.Run();
  EXPECT_EQ(handler.GetAllEthCustomChains().size(), 1u);
  EXPECT_EQ(handler.GetAllEthCustomChains()[0], chain1.Clone());

  const auto& assets_pref = handler.prefs()->GetDict(kBraveWalletUserAssets);
  const base::Value::List& list =
      *assets_pref.FindListByDottedPath("ethereum.0x999");
  ASSERT_EQ(list.size(), 1u);

  EXPECT_EQ(*list[0].GetDict().FindString("address"), "");
  EXPECT_EQ(*list[0].GetDict().FindString("name"), "symbol_name");
  EXPECT_EQ(*list[0].GetDict().FindString("symbol"), "symbol");
  EXPECT_EQ(*list[0].GetDict().FindBool("is_erc20"), false);
  EXPECT_EQ(*list[0].GetDict().FindBool("is_erc721"), false);
  EXPECT_EQ(*list[0].GetDict().FindInt("decimals"), 11);
  EXPECT_EQ(*list[0].GetDict().FindString("logo"), "https://url1.com");
  EXPECT_EQ(*list[0].GetDict().FindBool("visible"), true);

  base::Value::List args2;
  args2.Append(base::Value("id"));
  args2.Append(brave_wallet::NetworkInfoToValue(chain1));
  handler.AddChain(args2);
  const auto& data = *handler.web_ui()->call_data()[1];
  ASSERT_TRUE(data.arg1()->is_string());
  EXPECT_EQ(data.arg1()->GetString(), "id");

  const auto& arg3_list = data.arg3()->GetList();
  ASSERT_EQ(arg3_list.size(), 2UL);
  EXPECT_EQ(arg3_list[0].GetBool(), false);
  std::string error_message =
      l10n_util::GetStringUTF8(IDS_SETTINGS_WALLET_NETWORKS_EXISTS);
  EXPECT_EQ(arg3_list[1].GetString(), error_message);
  ASSERT_EQ(handler.GetAllEthCustomChains().size(), 1u);
  EXPECT_EQ(handler.GetAllEthCustomChains()[0], chain1.Clone());
}

TEST(TestBraveWalletHandler, AddChainWrongNetwork) {
  TestBraveWalletHandler handler;
  brave_wallet::mojom::NetworkInfo chain1 = brave_wallet::GetTestNetworkInfo1();

  EXPECT_EQ(handler.GetAllEthCustomChains().size(), 0u);

  base::Value::List args;
  args.Append(base::Value("id"));
  args.Append(brave_wallet::NetworkInfoToValue(chain1));
  handler.SetEthChainIdInterceptor(brave_wallet::GetActiveEndpointUrl(chain1),
                                   "0x11");
  base::RunLoop loop;
  handler.SetChainCallbackForTesting(loop.QuitClosure());
  handler.AddChain(args);
  loop.Run();
  const auto& data = *handler.web_ui()->call_data()[0];
  ASSERT_TRUE(data.arg1()->is_string());
  EXPECT_EQ(data.arg1()->GetString(), "id");

  const auto& arg3_list = data.arg3()->GetList();
  ASSERT_EQ(arg3_list.size(), 2UL);
  EXPECT_EQ(arg3_list[0].GetBool(), false);
  std::string error_message = l10n_util::GetStringFUTF8(
      IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
      base::ASCIIToUTF16(brave_wallet::GetActiveEndpointUrl(chain1).spec()));
  EXPECT_EQ(arg3_list[1].GetString(), error_message);
}

TEST(TestBraveWalletHandler, GetNetworkListEth) {
  TestBraveWalletHandler handler;
  std::vector<base::Value::Dict> values;
  brave_wallet::mojom::NetworkInfo chain1 = brave_wallet::GetTestNetworkInfo1();
  values.push_back(brave_wallet::NetworkInfoToValue(chain1));

  brave_wallet::mojom::NetworkInfo chain2 = brave_wallet::GetTestNetworkInfo2();
  values.push_back(brave_wallet::NetworkInfoToValue(chain2));
  UpdateCustomNetworks(handler.prefs(), CoinType::ETH, &values);
  EXPECT_EQ(handler.GetAllEthCustomChains().size(), 2u);

  base::Value::List args;
  args.Append(base::Value("id"));
  args.Append(base::Value(static_cast<int>(CoinType::ETH)));
  handler.GetNetworksList(args);
  const auto& data = *handler.web_ui()->call_data()[0];
  ASSERT_TRUE(data.arg1()->is_string());
  EXPECT_EQ(data.arg1()->GetString(), "id");
  ASSERT_TRUE(data.arg3()->is_dict());
  const auto& networks = *data.arg3()->GetDict().FindList("networks");

  size_t index = 0u;
  for (auto& known_chain :
       brave_wallet::GetAllKnownChains(handler.prefs(), CoinType::ETH)) {
    EXPECT_EQ(*brave_wallet::ValueToNetworkInfo(networks[index++]),
              *known_chain);
  }
  EXPECT_EQ(*brave_wallet::ValueToNetworkInfo(networks[index++]), chain1);
  EXPECT_EQ(*brave_wallet::ValueToNetworkInfo(networks[index++]), chain2);
}

TEST(TestBraveWalletHandler, GetNetworkListFilSol) {
  for (auto coin : {CoinType::FIL, CoinType::SOL}) {
    TestBraveWalletHandler handler;

    base::Value::List args;
    args.Append(base::Value("id"));
    args.Append(base::Value(static_cast<int>(coin)));

    handler.GetNetworksList(args);
    const auto& data = *handler.web_ui()->call_data()[0];
    ASSERT_TRUE(data.arg1()->is_string());
    EXPECT_EQ(data.arg1()->GetString(), "id");
    ASSERT_TRUE(data.arg3()->is_dict());
    const auto& networks = *data.arg3()->GetDict().FindList("networks");

    size_t index = 0u;
    for (auto& known_chain :
         brave_wallet::GetAllKnownChains(handler.prefs(), coin)) {
      EXPECT_EQ(*brave_wallet::ValueToNetworkInfo(networks[index++]),
                *known_chain);
    }
  }
}

TEST(TestBraveWalletHandler, SetActiveNetwork) {
  TestBraveWalletHandler handler;

  std::vector<base::Value::Dict> values;
  brave_wallet::mojom::NetworkInfo chain1 = brave_wallet::GetTestNetworkInfo1();
  values.push_back(brave_wallet::NetworkInfoToValue(chain1));

  brave_wallet::mojom::NetworkInfo chain2 = brave_wallet::GetTestNetworkInfo2();
  values.push_back(brave_wallet::NetworkInfoToValue(chain2));
  UpdateCustomNetworks(handler.prefs(), CoinType::ETH, &values);
  EXPECT_EQ(handler.GetAllEthCustomChains().size(), 2u);

  DictionaryPrefUpdate update(handler.prefs(), kBraveWalletSelectedNetworks);
  base::Value::Dict& dict = update.Get()->GetDict();
  dict.Set(brave_wallet::kEthereumPrefKey, "chain_id");
  {
    base::Value::List args;
    args.Append(base::Value("id"));
    args.Append(base::Value("chain_id2"));
    args.Append(base::Value(static_cast<int>(CoinType::ETH)));

    handler.SetActiveNetwork(args);
    const auto& data = *handler.web_ui()->call_data()[0];
    ASSERT_TRUE(data.arg3()->is_bool());
    EXPECT_EQ(data.arg3()->GetBool(), true);

    EXPECT_EQ(brave_wallet::GetCurrentChainId(handler.prefs(), CoinType::ETH),
              "chain_id2");
  }
  {
    base::Value::List args;
    args.Append(base::Value("id"));
    args.Append(base::Value("unknown_chain_id"));
    args.Append(base::Value(static_cast<int>(CoinType::ETH)));

    handler.SetActiveNetwork(args);
    const auto& data = *handler.web_ui()->call_data()[1];
    ASSERT_TRUE(data.arg3()->is_bool());
    EXPECT_EQ(data.arg3()->GetBool(), false);

    EXPECT_EQ(brave_wallet::GetCurrentChainId(handler.prefs(), CoinType::ETH),
              "chain_id2");
  }
}
