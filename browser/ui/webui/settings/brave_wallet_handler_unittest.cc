/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_wallet_handler.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/values.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_ui.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using brave_wallet::mojom::CoinType;
using testing::Contains;
using testing::Eq;
using testing::Not;

namespace {

void UpdateCustomNetworks(PrefService* prefs,
                          CoinType coin,
                          std::vector<base::Value::Dict>* values) {
  ScopedDictPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::Value::List* list =
      update->EnsureList(brave_wallet::GetPrefKeyForCoinType(coin));
  list->clear();
  for (auto& it : *values) {
    list->Append(std::move(it));
  }
}

}  // namespace

class TestBraveWalletHandler : public BraveWalletHandler {
 public:
  TestBraveWalletHandler()
      : local_state_(TestingBrowserProcess::GetGlobal()),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    TestingProfile::Builder builder;

    profile_ = builder.Build();
    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_.get()));

    test_web_ui_.set_web_contents(web_contents_.get());
    set_web_ui(&test_web_ui_);
    brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
        profile_.get())
        ->json_rpc_service()
        ->SetAPIRequestHelperForTesting(shared_url_loader_factory_);
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
          std::string_view request_string(request.request_body->elements()
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
    return GetNetworkManager()->GetAllCustomChains(CoinType::ETH);
  }

  std::vector<brave_wallet::mojom::NetworkInfoPtr> GetAllCustomChains(
      brave_wallet::mojom::CoinType coin) {
    return GetNetworkManager()->GetAllCustomChains(coin);
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
  void SetDefaultNetwork(const base::Value::List& args) {
    BraveWalletHandler::SetDefaultNetwork(args);
  }
  content::TestWebUI* web_ui() { return &test_web_ui_; }
  PrefService* prefs() { return profile_->GetPrefs(); }

  brave_wallet::NetworkManager* GetNetworkManager() {
    return brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
               profile_.get())
        ->network_manager();
  }

 private:
  ScopedTestingLocalState local_state_;
  content::BrowserTaskEnvironment browser_task_environment_;
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

  auto expected_token = brave_wallet::mojom::BlockchainToken::New();
  expected_token->coin = brave_wallet::mojom::CoinType::ETH;
  expected_token->chain_id = "0x999";
  expected_token->name = "symbol_name";
  expected_token->symbol = "symbol";
  expected_token->decimals = 11;
  expected_token->logo = "https://url1.com";
  expected_token->visible = true;
  expected_token->spl_token_program =
      brave_wallet::mojom::SPLTokenProgram::kUnsupported;

  EXPECT_THAT(brave_wallet::GetAllUserAssets(handler.prefs()),
              Not(Contains(Eq(std::ref(expected_token)))));

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

  EXPECT_THAT(brave_wallet::GetAllUserAssets(handler.prefs()),
              Contains(Eq(std::ref(expected_token))));

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
       handler.GetNetworkManager()->GetAllKnownChains(CoinType::ETH)) {
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
         handler.GetNetworkManager()->GetAllKnownChains(coin)) {
      EXPECT_EQ(*brave_wallet::ValueToNetworkInfo(networks[index++]),
                *known_chain);
    }
  }
}

TEST(TestBraveWalletHandler, SetDefaultNetwork) {
  TestBraveWalletHandler handler;

  std::vector<base::Value::Dict> values;
  brave_wallet::mojom::NetworkInfo chain1 = brave_wallet::GetTestNetworkInfo1();
  values.push_back(brave_wallet::NetworkInfoToValue(chain1));

  brave_wallet::mojom::NetworkInfo chain2 = brave_wallet::GetTestNetworkInfo2();
  values.push_back(brave_wallet::NetworkInfoToValue(chain2));
  UpdateCustomNetworks(handler.prefs(), CoinType::ETH, &values);
  EXPECT_EQ(handler.GetAllEthCustomChains().size(), 2u);

  ScopedDictPrefUpdate update(handler.prefs(), kBraveWalletSelectedNetworks);
  update->Set(brave_wallet::kEthereumPrefKey, "chain_id");
  {
    base::Value::List args;
    args.Append(base::Value("id"));
    args.Append(base::Value("chain_id2"));
    args.Append(base::Value(static_cast<int>(CoinType::ETH)));

    handler.SetDefaultNetwork(args);
    const auto& data = *handler.web_ui()->call_data()[0];
    ASSERT_TRUE(data.arg3()->is_bool());
    EXPECT_EQ(data.arg3()->GetBool(), true);

    EXPECT_EQ(handler.GetNetworkManager()->GetCurrentChainId(CoinType::ETH,
                                                             std::nullopt),
              "chain_id2");
  }
  {
    base::Value::List args;
    args.Append(base::Value("id"));
    args.Append(base::Value("unknown_chain_id"));
    args.Append(base::Value(static_cast<int>(CoinType::ETH)));

    handler.SetDefaultNetwork(args);
    const auto& data = *handler.web_ui()->call_data()[1];
    ASSERT_TRUE(data.arg3()->is_bool());
    EXPECT_EQ(data.arg3()->GetBool(), false);

    EXPECT_EQ(handler.GetNetworkManager()->GetCurrentChainId(CoinType::ETH,
                                                             std::nullopt),
              "chain_id2");
  }
}
