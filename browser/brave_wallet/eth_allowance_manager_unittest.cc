/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_allowance_manager.h"

#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_observer_base.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
namespace brave_wallet {

namespace {

constexpr std::string_view eth_allowance_detected_response = R"({
    "jsonrpc": "2.0",
    "id": 1,
    "result": [
        {
            "address": "$1",
            "blockHash": "0xaff41c269d9f87f9d71e826ccc612bec9eff33fe5f01a0c9b6f54bfaa8178686",
            "blockNumber": "0x101a7f1",
            "data": "0x0000000000000000000000000000000000000000000000000000000000000001",
            "logIndex": "0x92",
            "removed": false,
            "topics": [
                "0x8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925",
                "$2",
                "0x000000000000000000000000dac308312e195710467ce36effe51ac7a4ecbf01"
            ],
            "transactionHash": "0x32132b285e95d82a9b81e3f25ea8290756f36c9fedd92af8290d4ee8cd1d7f98",
            "transactionIndex": "0x38"
        }
    ]
})";

constexpr std::string_view eth_allowance_error_response = R"({
                  "error": {
                    "code": -32000,
"message": "requested too many blocks from 0
 to 27842567, maximum is set to 2048"
                  },
                  "id": 1,
                  "jsonrpc": "2.0"
                })";

constexpr char token_list_json[] = R"({
      "0x3333333333333333333333333333333333333333": {
        "name": "3333",
        "logo": "333.svg",
        "erc20": true,
        "symbol": "333",
        "decimals": 18,
        "chainId": "0x1"
      }
    })";

constexpr char allowance_chache_json[] = R"({
  "0x1": {
    "allowances_found": [
      {
        "amount": "0x0000000000000000000000000000000000000000000000000000000000000001",
        "approver_address": "0x00000000000000000000000091272b2c4990927d1fe28201cf0a6ce288a221d6",
        "contract_address": "0x0c10bf8fcb7bf5412187a595ab97a3609160b5c6",
        "spender_address": "0x000000000000000000000000dac308312e195710467ce36effe51ac7a4ecbf01"
      }
    ],
    "last_block_number": {
      "0x00000000000000000000000091272b2c4990927d1fE28201cf0A6CE288a221d6": "0x1054bfe"
    }
  },
  "0xa4b1": {
    "allowances_found": [
      {
        "amount": "0x0000000000000000000000000000000000000000000000000000000000000001",
        "approver_address": "0x00000000000000000000000091272b2c4990927d1fe28201cf0a6ce288a221d6",
        "contract_address": "0xfd086bc7cd5c481dcc9c85ebe478a1c0b69fcbb9",
        "spender_address": "0x000000000000000000000000dac308312e195710467ce36effe51ac7a4ecbf01"
      }
    ],
    "last_block_number": {
      "0x00000000000000000000000091272b2c4990927d1fE28201cf0A6CE288a221d6": "0x504d1a3"
    }
  }
})";

constexpr char incorrect_allowance_chache_data_json[] = R"({
            "0x1": {
               "allowances_found": [ {
                  "amount":
         "0x0000000000000000000000000000000000000000000000000000000000000001",
                  "approver_address":
         "0x00000000000000000000000091272b2c4990927d1fe28201cf0a6ce288a221d6",
                  "spender_address":
         "0x000000000000000000000000dac308312e195710467ce36effe51ac7a4ecbf01"
               } ],
               "last_block_number": "0x1054bfe"
            }
         })";

constexpr char incorrect_allowance_chache_block_number_json[] = R"({
            "0x1": {
               "allowances_found": [ {
                  "amount":
         "0x0000000000000000000000000000000000000000000000000000000000000001",
                  "approver_address":
         "0x00000000000000000000000091272b2c4990927d1fe28201cf0a6ce288a221d6",
                  "contract_address":
         "0x0c10bf8fcb7bf5412187a595ab97a3609160b5c6",
                  "spender_address":
         "0x000000000000000000000000dac308312e195710467ce36effe51ac7a4ecbf01"
               } ],
               "last_block_number": "123456"
            }
         })";

constexpr char get_block_response[] =
    R"({"jsonrpc":"2.0","id":1,"result":"0x10964ec"})";

constexpr char get_block_response_wrong[] =
    R"({"jsonrpc":"2.0","id":1,"result_wrong":""})";

constexpr char kPasswordBrave[] = "brave";

using AllowancesMap = std::map<std::string, mojom::AllowanceInfoPtr>;
using AllowancesMapCallback = base::OnceCallback<void(const AllowancesMap&)>;
using OnDiscoverEthAllowancesCompletedValidation =
    base::RepeatingCallback<void(const std::vector<mojom::AllowanceInfoPtr>&)>;

void FillAllowanceLogItem(base::Value::Dict& current_item,
                          const std::string& contract_address,
                          const uint256_t& log_index,
                          const std::string& approver_address,
                          const uint256_t& amount,
                          const std::string& block_number = "") {
  current_item.Set("address", contract_address);
  current_item.Set("logIndex", Uint256ValueToHex(log_index));

  auto* pr_topics_ptr = current_item.FindList("topics");
  DCHECK(pr_topics_ptr);
  (*pr_topics_ptr)[1] = base::Value(approver_address);

  std::string hex_amount;
  if (PadHexEncodedParameter(Uint256ValueToHex(amount), &hex_amount)) {
    current_item.Set("data", hex_amount);
  }

  if (!block_number.empty()) {
    current_item.Set("blockNumber", block_number);
  }
}

mojom::AllowanceInfoPtr GetAllowanceInfo(const base::Value::Dict& current_item,
                                         const std::string& chain_id) {
  const auto* contract_addr_ptr = current_item.FindString("address");
  DCHECK(contract_addr_ptr);
  const auto* pr_topics_ptr = current_item.FindList("topics");
  DCHECK(pr_topics_ptr);
  const auto* data_ptr = current_item.FindString("data");
  DCHECK(data_ptr);
  return mojom::AllowanceInfo::New(chain_id, *contract_addr_ptr,
                                   (*pr_topics_ptr)[1].GetString(),
                                   (*pr_topics_ptr)[2].GetString(), *data_ptr);
}

}  // namespace
class EthAllowanceManagerUnitTest : public testing::Test {
 public:
  EthAllowanceManagerUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)),
        task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~EthAllowanceManagerUnitTest() override = default;

 protected:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(
        features::kNativeBraveWalletFeature);

    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());
    wallet_service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_,
        BraveWalletServiceDelegate::Create(profile_.get()), GetPrefs(),
        GetLocalState());
    json_rpc_service_ = wallet_service_->json_rpc_service();
    keyring_service_ = wallet_service_->keyring_service();
    bitcoin_test_rpc_server_ = std::make_unique<BitcoinTestRpcServer>();
    wallet_service_->GetBitcoinWalletService()->SetUrlLoaderFactoryForTesting(
        bitcoin_test_rpc_server_->GetURLLoaderFactory());
    eth_allowance_manager_ = std::make_unique<EthAllowanceManager>(
        json_rpc_service_, keyring_service_, GetPrefs());
  }

  void CreateCachedAllowancesPrefs(const std::string& json) {
    std::optional<base::Value> allowance_chache_json_value =
        base::JSONReader::Read(json,
                               base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                   base::JSONParserOptions::JSON_PARSE_RFC);

    if (allowance_chache_json_value.has_value()) {
      GetPrefs()->SetDict(
          kBraveWalletEthAllowancesCache,
          allowance_chache_json_value.value().GetDict().Clone());
    }
  }

  void AddEthAccount(const std::string& account_name) {
    keyring_service_->AddAccountSync(mojom::CoinType::ETH,
                                     mojom::kDefaultKeyringId, account_name);
  }

  void CreateWallet() {
    AccountUtils(keyring_service_)
        .CreateWallet(kMnemonicDivideCruise, kPasswordBrave);
  }

  void TestAllowancesLoading(
      const std::string& current_token_list_json,
      base::OnceCallback<std::map<GURL, std::map<std::string, std::string>>(
          const std::vector<std::string>&,
          const TokenListMap&)> get_responses,
      const int& eth_account_count,
      const std::size_t& eth_allowance_completed_call_count,
      OnDiscoverEthAllowancesCompletedValidation allowances_validation,
      const std::string& get_block_response_str,
      const std::size_t& call_reset_on_pos =
          std::numeric_limits<std::size_t>::max()) {
    auto* blockchain_registry = BlockchainRegistry::GetInstance();

    TokenListMap token_list_map;
    ASSERT_TRUE(ParseTokenList(current_token_list_json, &token_list_map,
                               mojom::CoinType::ETH));

    std::vector<std::string> contract_addresses;
    for (auto const& [contract_addr, token_info] : token_list_map) {
      for (auto const& tkn : token_info) {
        contract_addresses.push_back(tkn->contract_address);
      }
    }
    ASSERT_TRUE(keyring_service_->RestoreWalletSync(kMnemonicDivideCruise,
                                                    kPasswordBrave, false));
    for (int i = 0; i < (eth_account_count - 1); i++) {
      AddEthAccount("additonal eth account");
    }

    std::vector<std::string> account_addresses;
    for (const auto& account_info : keyring_service_->GetAllAccountInfos()) {
      if (account_info->account_id->coin == mojom::CoinType::ETH) {
        std::string hex_account_address;
        ASSERT_TRUE(PadHexEncodedParameter(account_info->address,
                                           &hex_account_address));
        account_addresses.push_back(hex_account_address);
      }
    }

    auto responses =
        std::move(get_responses).Run(account_addresses, token_list_map);
    blockchain_registry->UpdateTokenList(std::move(token_list_map));

    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, responses](const network::ResourceRequest& request) {
          for (auto const& [url, address_response_map] : responses) {
            if (request.url.spec().find("nfts") != std::string::npos) {
              continue;
            }
            auto header_value = request.headers.GetHeader("X-Eth-Method");
            ASSERT_TRUE(header_value);
            if (request.url.spec() == url.spec() &&
                *header_value == "eth_blockNumber") {
              url_loader_factory_.ClearResponses();
              url_loader_factory_.AddResponse(request.url.spec(),
                                              get_block_response_str);
            } else if (request.url.spec() == url.spec() &&
                       *header_value == "eth_getLogs") {
              std::optional<base::Value> request_dict_val =
                  base::JSONReader::Read(
                      request.request_body->elements()
                          ->at(0)
                          .As<network::DataElementBytes>()
                          .AsStringPiece(),
                      base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                          base::JSONParserOptions::JSON_PARSE_RFC);
              std::string response;
              for (auto const& [address, potential_response] :
                   address_response_map) {
                const auto* params_ptr =
                    request_dict_val.value().GetDict().FindList("params");
                if (!params_ptr) {
                  continue;
                }

                const auto* address_ptr =
                    (*params_ptr)[0].GetDict().FindList("address");
                const auto* topics_ptr =
                    (*params_ptr)[0].GetDict().FindList("topics");

                if (!address_ptr || !topics_ptr) {
                  continue;
                }

                const auto request_addresses = (*address_ptr)[0].GetString();
                const auto request_approver_address =
                    (*topics_ptr)[1].GetString();

                std::optional<base::Value> potential_response_dict_val =
                    base::JSONReader::Read(
                        potential_response,
                        base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                            base::JSONParserOptions::JSON_PARSE_RFC);

                const auto* pr_error_ptr =
                    potential_response_dict_val.value().GetDict().FindDict(
                        "error");

                if (pr_error_ptr) {
                  response = potential_response;
                  continue;
                }

                const auto* pr_result_ptr =
                    potential_response_dict_val.value().GetDict().FindList(
                        "result");

                if (!pr_result_ptr) {
                  continue;
                }

                if (pr_result_ptr->empty()) {
                  response = potential_response;
                  continue;
                }

                for (const auto& curr_result : *pr_result_ptr) {
                  const auto* pr_topics_ptr =
                      curr_result.GetDict().FindList("topics");
                  if (!pr_topics_ptr) {
                    continue;
                  }

                  const auto* pr_contract_address_ptr =
                      curr_result.GetDict().FindString("address");
                  const auto pr_approver_address =
                      (*pr_topics_ptr)[1].GetString();

                  if (std::find(address_ptr->begin(), address_ptr->end(),
                                *pr_contract_address_ptr) !=
                          address_ptr->end() &&
                      request_approver_address == pr_approver_address) {
                    auto pr_dict =
                        potential_response_dict_val.value().GetDict().Clone();

                    auto* pr_dict_result_ptr = pr_dict.FindList("result");

                    if (!pr_dict_result_ptr) {
                      continue;
                    }
                    pr_dict_result_ptr->EraseIf([&](const base::Value& item) {
                      auto* contract_address_value =
                          item.GetDict().FindString("address");
                      if (!contract_address_value) {
                        return true;
                      }
                      const auto* topics_list =
                          item.GetDict().FindList("topics");
                      if (!topics_list) {
                        return true;
                      }
                      return *contract_address_value !=
                                 *pr_contract_address_ptr ||
                             (*topics_list)[1].GetString() !=
                                 request_approver_address;
                    });
                    response = pr_dict.DebugString();
                    break;
                  }
                }
                if (!response.empty()) {
                  break;
                }
              }
              ASSERT_FALSE(response.empty());
              url_loader_factory_.ClearResponses();
              url_loader_factory_.AddResponse(request.url.spec(), response);
            }
          }
        }));

    base::RunLoop run_loop;
    std::size_t call_count{0};
    std::size_t callback_count{0};
    while (eth_allowance_completed_call_count > call_count) {
      if (call_reset_on_pos == call_count) {
        eth_allowance_manager_->Reset();
      } else {
        eth_allowance_manager_->DiscoverEthAllowancesOnAllSupportedChains(
            base::BindLambdaForTesting(
                [&](std::vector<mojom::AllowanceInfoPtr> allowances) {
                  callback_count++;
                  allowances_validation.Run(allowances);
                  if (callback_count ==
                      (eth_allowance_completed_call_count -
                       (call_reset_on_pos !=
                                std::numeric_limits<std::size_t>::max()
                            ? 1
                            : 0))) {
                    run_loop.Quit();
                  }
                }));
      }
      call_count++;
    }
    run_loop.Run();
  }

  void TestLoadCachedAllowances(const std::string& chain_id,
                                const std::string& hex_account_address,
                                AllowancesMapCallback test_validation) {
    AllowancesMap allowance_map;
    eth_allowance_manager_->LoadCachedAllowances(chain_id, hex_account_address,
                                                 allowance_map);
    std::move(test_validation).Run(allowance_map);
  }

  std::map<GURL, std::map<std::string, std::string>> PrepareResponses(
      std::string_view response_json,
      const std::vector<std::string>& eth_account_address,
      const TokenListMap& token_list_map,
      base::RepeatingCallback<void(base::Value::Dict,
                                   const mojom::BlockchainTokenPtr&,
                                   uint256_t&,
                                   const std::string&,
                                   const std::string&,
                                   base::Value::List*)> per_addr_action) {
    std::map<GURL, std::map<std::string, std::string>> result;
    for (auto const& [key, token_info] : token_list_map) {
      std::string chain_id;
      std::map<std::string, std::string> resp_jsns_map;
      for (auto const& tkn : token_info) {
        chain_id = tkn->chain_id;
        auto dict = base::test::ParseJsonDict(response_json);
        if (!dict.FindList("error")) {
          auto* pr_result_ptr = dict.FindList("result");
          DCHECK(pr_result_ptr);
          auto& result_item = (*pr_result_ptr)[0].GetDict();
          auto temp_item = result_item.Clone();
          pr_result_ptr->clear();
          uint256_t log_index(0);
          for (const auto& addr : eth_account_address) {
            auto allovance_item = temp_item.Clone();
            per_addr_action.Run(std::move(allovance_item), tkn, log_index, addr,
                                chain_id, pr_result_ptr);
          }
          resp_jsns_map.insert({tkn->contract_address, dict.DebugString()});
        }
      }
      result.insert(
          {GetNetwork(chain_id, mojom::CoinType::ETH), resp_jsns_map});
    }
    return result;
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }
  TestingPrefServiceSimple* GetLocalState() { return local_state_->Get(); }
  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return wallet_service_->network_manager()->GetNetworkURL(chain_id, coin);
  }

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<BraveWalletService> wallet_service_;
  std::unique_ptr<EthAllowanceManager> eth_allowance_manager_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<BitcoinTestRpcServer> bitcoin_test_rpc_server_;
  base::test::ScopedFeatureList scoped_feature_list_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(EthAllowanceManagerUnitTest, LoadCachedAllowances) {
  CreateCachedAllowancesPrefs(allowance_chache_json);
  TestLoadCachedAllowances(
      "0x1",
      "0x00000000000000000000000091272b2c4990927d1fE28201cf0A6CE288a221d6",
      base::BindLambdaForTesting([](const AllowancesMap& allowance_map) {
        EXPECT_EQ(allowance_map.size(), 1u);
        const auto map_key =
            base::JoinString({"0x0c10bf8fcb7bf5412187a595ab97a3609160b5c6",
                              "0x00000000000000000000000091272b2c4990927d1fe282"
                              "01cf0a6ce288a221d6",
                              "0x000000000000000000000000dac308312e195710467ce3"
                              "6effe51ac7a4ecbf01"},
                             "_");

        auto allowance_iter = allowance_map.find(map_key);
        EXPECT_TRUE(allowance_map.end() != allowance_iter);
        const auto allowance = allowance_iter->second.Clone();
        EXPECT_EQ(allowance->contract_address,
                  "0x0c10bf8fcb7bf5412187a595ab97a3609160b5c6");
        EXPECT_EQ(allowance->approver_address,
                  "0x00000000000000000000000091272b2c4990927d1fe28201cf0a6ce288"
                  "a221d6");
        EXPECT_EQ(allowance->spender_address,
                  "0x000000000000000000000000dac308312e195710467ce36effe51ac7a4"
                  "ecbf01");
        EXPECT_EQ(allowance->amount,
                  "0x0000000000000000000000000000000000000000000000000000000000"
                  "000001");
      }));
}

TEST_F(EthAllowanceManagerUnitTest, CouldNotLoadCachedAllowancesPrefsEmpty) {
  TestLoadCachedAllowances(
      "0x1",
      "0x00000000000000000000000091272b2c4990927d1fE28201cf0A6CE288a221d6",
      base::BindLambdaForTesting([](const AllowancesMap& allowance_map) {
        EXPECT_TRUE(allowance_map.empty());
      }));
}

TEST_F(EthAllowanceManagerUnitTest, CouldNotLoadCachedAllowancesByAddress) {
  CreateCachedAllowancesPrefs(allowance_chache_json);
  TestLoadCachedAllowances(
      "0x1",
      "0x000000000000000000000000000000000000000000000000000000000000AAAA",
      base::BindLambdaForTesting([](const AllowancesMap& allowance_map) {
        EXPECT_TRUE(allowance_map.empty());
      }));

  TestLoadCachedAllowances(
      "0x99",
      "0x00000000000000000000000091272b2c4990927d1fE28201cf0A6CE288a221d6",
      base::BindLambdaForTesting([](const AllowancesMap& allowance_map) {
        EXPECT_TRUE(allowance_map.empty());
      }));
}

TEST_F(EthAllowanceManagerUnitTest,
       CouldNotLoadCachedAllowancesIncorrectCacheData) {
  CreateCachedAllowancesPrefs(incorrect_allowance_chache_data_json);
  TestLoadCachedAllowances(
      "0x1",
      "0x00000000000000000000000091272b2c4990927d1fe28201cf0a6ce288a221d6",
      base::BindLambdaForTesting([](const AllowancesMap& allowance_map) {
        EXPECT_TRUE(allowance_map.empty());
      }));
}

TEST_F(EthAllowanceManagerUnitTest,
       CouldNotLoadCachedAllowancesIncorrectCacheBlockNumber) {
  CreateCachedAllowancesPrefs(incorrect_allowance_chache_block_number_json);
  TestLoadCachedAllowances(
      "0x1",
      "0x00000000000000000000000091272b2c4990927d1fe28201cf0a6ce288a221d6",
      base::BindLambdaForTesting([](const AllowancesMap& allowance_map) {
        EXPECT_TRUE(allowance_map.empty());
      }));
}

TEST_F(EthAllowanceManagerUnitTest, BreakAllowanceDiscoveringIfTokenListEmpty) {
  CreateWallet();
  TokenListMap token_list_map;
  BlockchainRegistry::GetInstance()->UpdateTokenList(std::move(token_list_map));
  int on_completed_call_counter(0);

  eth_allowance_manager_->DiscoverEthAllowancesOnAllSupportedChains(
      base::BindLambdaForTesting(
          [&](std::vector<mojom::AllowanceInfoPtr> allowances) {
            ASSERT_TRUE((++on_completed_call_counter == 1) &&
                        allowances.empty());
          }));
}

TEST_F(EthAllowanceManagerUnitTest, AllowancesLoading) {
  std::vector<mojom::AllowanceInfoPtr> expected_allowances;
  // Generates one allowance log per ETH account address.
  auto generate_responses = base::BindLambdaForTesting(
      [&](const std::vector<std::string>& eth_account_address,
          const TokenListMap& token_list_map) {
        return PrepareResponses(
            eth_allowance_detected_response, eth_account_address,
            token_list_map,
            base::BindLambdaForTesting(
                [&](base::Value::Dict allovance_item,
                    const mojom::BlockchainTokenPtr& tkn, uint256_t& log_index,
                    const std::string& addr, const std::string& chain_id,
                    base::Value::List* pr_result_ptr) {
                  FillAllowanceLogItem(allovance_item, tkn->contract_address,
                                       ++log_index, addr, 1);
                  expected_allowances.push_back(
                      GetAllowanceInfo(allovance_item, chain_id));
                  pr_result_ptr->Append(std::move(allovance_item));
                }));
      });

  auto allowances_validation = base::BindLambdaForTesting(
      [&expected_allowances](
          const std::vector<mojom::AllowanceInfoPtr>& allowances) {
        ASSERT_EQ(allowances.size(), expected_allowances.size());
        for (const auto& expected_allowance : expected_allowances) {
          const auto& allowance_iter = std::find_if(
              allowances.begin(), allowances.end(),
              [&expected_allowance](const auto& item) {
                return expected_allowance->amount == item->amount &&
                       expected_allowance->contract_address ==
                           item->contract_address &&
                       expected_allowance->approver_address ==
                           item->approver_address &&
                       expected_allowance->spender_address ==
                           item->spender_address;
              });
          ASSERT_FALSE(allowance_iter == allowances.end());
        }
      });
  size_t account_count(2);
  TestAllowancesLoading(token_list_json, generate_responses, account_count, 3,
                        allowances_validation, get_block_response);

  const auto& allowance_cashe_dict =
      GetPrefs()->GetDict(kBraveWalletEthAllowancesCache);
  const auto* chain_id_dict = allowance_cashe_dict.FindDict("0x1");
  ASSERT_TRUE(nullptr != chain_id_dict);
  const auto* last_block_number_dict_ptr =
      chain_id_dict->FindDict("last_block_number");
  ASSERT_TRUE(nullptr != last_block_number_dict_ptr);
  const auto* last_block_number_ptr = last_block_number_dict_ptr->FindString(
      "0x000000000000000000000000f81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
  ASSERT_TRUE(nullptr != last_block_number_ptr);
  EXPECT_EQ(*last_block_number_ptr, "0x10964ec");
  const auto* allowances_found_ptr =
      chain_id_dict->FindList("allowances_found");
  ASSERT_TRUE(nullptr != allowances_found_ptr);
  EXPECT_EQ(allowances_found_ptr->size(), account_count);
}

TEST_F(EthAllowanceManagerUnitTest, AllowancesLoadingFailedGetBlock) {
  std::vector<mojom::AllowanceInfoPtr> expected_allowances;
  // Generates one allowance log per ETH account address.
  auto generate_responses = base::BindLambdaForTesting(
      [&](const std::vector<std::string>& eth_account_address,
          const TokenListMap& token_list_map) {
        return PrepareResponses(
            eth_allowance_detected_response, eth_account_address,
            token_list_map,
            base::BindLambdaForTesting(
                [&](base::Value::Dict allovance_item,
                    const mojom::BlockchainTokenPtr& tkn, uint256_t& log_index,
                    const std::string& addr, const std::string& chain_id,
                    base::Value::List* pr_result_ptr) {
                  FillAllowanceLogItem(allovance_item, tkn->contract_address,
                                       ++log_index, addr, 1);
                  expected_allowances.push_back(
                      GetAllowanceInfo(allovance_item, chain_id));
                  pr_result_ptr->Append(std::move(allovance_item));
                }));
      });

  auto allowances_validation = base::BindLambdaForTesting(
      [](const std::vector<mojom::AllowanceInfoPtr>& allowances) {
        ASSERT_EQ(allowances.size(), 0u);
      });
  size_t account_count(2);
  TestAllowancesLoading(token_list_json, generate_responses, account_count, 3,
                        allowances_validation, get_block_response_wrong);

  const auto& allowance_cashe_dict =
      GetPrefs()->GetDict(kBraveWalletEthAllowancesCache);
  const auto* chain_id_dict = allowance_cashe_dict.FindDict("0x1");
  ASSERT_TRUE(!chain_id_dict);
}

TEST_F(EthAllowanceManagerUnitTest, AllowancesRevoked) {
  // Generates an allowance and revokation logs per account address.
  auto generate_revoked_responses = base::BindLambdaForTesting(
      [&](const std::vector<std::string>& eth_account_address,
          const TokenListMap& token_list_map) {
        return PrepareResponses(
            eth_allowance_detected_response, eth_account_address,
            token_list_map,
            base::BindLambdaForTesting(
                [&](base::Value::Dict allovance_item,
                    const mojom::BlockchainTokenPtr& tkn, uint256_t& log_index,
                    const std::string& addr, const std::string& chain_id,
                    base::Value::List* pr_result_ptr) {
                  FillAllowanceLogItem(allovance_item, tkn->contract_address,
                                       ++log_index, addr, 1);
                  auto revoke_item = allovance_item.Clone();
                  FillAllowanceLogItem(revoke_item, tkn->contract_address,
                                       ++log_index, addr, 0);
                  // Add allowance record.
                  pr_result_ptr->Append(std::move(allovance_item));
                  // Add revocation record.
                  pr_result_ptr->Append(std::move(revoke_item));
                }));
      });

  auto allowances_validation = base::BindLambdaForTesting(
      [](const std::vector<mojom::AllowanceInfoPtr>& allowances) {
        ASSERT_TRUE(allowances.empty());
      });

  TestAllowancesLoading(token_list_json, generate_revoked_responses, 2, 3,
                        allowances_validation, get_block_response);

  const auto& allowance_cashe_dict =
      GetPrefs()->GetDict(kBraveWalletEthAllowancesCache);
  const auto* chain_id_dict = allowance_cashe_dict.FindDict("0x1");
  ASSERT_TRUE(nullptr != chain_id_dict);
  const auto* allowances_found_ptr =
      chain_id_dict->FindList("allowances_found");
  ASSERT_TRUE(nullptr != allowances_found_ptr);

  ASSERT_TRUE(allowances_found_ptr->empty());
}

TEST_F(EthAllowanceManagerUnitTest, AllowancesIgnorePendingBlocks) {
  // Generates an logs with block in the pending state.
  auto generate_pending_responses = base::BindLambdaForTesting(
      [&](const std::vector<std::string>& eth_account_address,
          const TokenListMap& token_list_map) {
        return PrepareResponses(
            eth_allowance_detected_response, eth_account_address,
            token_list_map,
            base::BindLambdaForTesting(
                [&](base::Value::Dict allovance_item,
                    const mojom::BlockchainTokenPtr& tkn, uint256_t& log_index,
                    const std::string& addr, const std::string& chain_id,
                    base::Value::List* pr_result_ptr) {
                  // Mark block number as 0x0 like for pending state.
                  FillAllowanceLogItem(allovance_item, tkn->contract_address,
                                       ++log_index, addr, 1, "0x0");
                  // Add allowance record.
                  pr_result_ptr->Append(std::move(allovance_item));
                }));
      });

  auto allowances_validation = base::BindLambdaForTesting(
      [](const std::vector<mojom::AllowanceInfoPtr>& allowances) {
        // There are no any allowances found.
        ASSERT_TRUE(allowances.empty());
      });

  TestAllowancesLoading(token_list_json, generate_pending_responses, 1, 1,
                        allowances_validation, get_block_response);

  const auto& allowance_cashe_dict =
      GetPrefs()->GetDict(kBraveWalletEthAllowancesCache);
  const auto* chain_id_dict = allowance_cashe_dict.FindDict("0x1");
  ASSERT_TRUE(nullptr != chain_id_dict);
  const auto* allowances_found_ptr =
      chain_id_dict->FindList("allowances_found");
  ASSERT_TRUE(nullptr != allowances_found_ptr);
  ASSERT_TRUE(allowances_found_ptr->empty());
}

TEST_F(EthAllowanceManagerUnitTest, AllowancesIgnoreWrongTopicsData) {
  // Generates logs with wrong topics data (one item is missing).
  auto generate_pending_responses = base::BindLambdaForTesting(
      [&](const std::vector<std::string>& eth_account_address,
          const TokenListMap& token_list_map) {
        return PrepareResponses(
            eth_allowance_detected_response, eth_account_address,
            token_list_map,
            base::BindLambdaForTesting(
                [&](base::Value::Dict allovance_item,
                    const mojom::BlockchainTokenPtr& tkn, uint256_t& log_index,
                    const std::string& addr, const std::string& chain_id,
                    base::Value::List* pr_result_ptr) {
                  FillAllowanceLogItem(allovance_item, tkn->contract_address,
                                       ++log_index, addr, 1);
                  auto* topics_ptr = allovance_item.FindList("topics");
                  DCHECK(topics_ptr);
                  // Remove spender topics data.
                  topics_ptr->EraseValue(topics_ptr->back());
                  // Add allowance record.
                  pr_result_ptr->Append(std::move(allovance_item));
                }));
      });

  auto allowances_validation = base::BindLambdaForTesting(
      [](const std::vector<mojom::AllowanceInfoPtr>& allowances) {
        // There are no any allowances found.
        ASSERT_TRUE(allowances.empty());
      });

  TestAllowancesLoading(token_list_json, generate_pending_responses, 1, 1,
                        allowances_validation, get_block_response);

  const auto& allowance_cashe_dict =
      GetPrefs()->GetDict(kBraveWalletEthAllowancesCache);
  const auto* chain_id_dict = allowance_cashe_dict.FindDict("0x1");
  ASSERT_TRUE(nullptr != chain_id_dict);
  const auto* allowances_found_ptr =
      chain_id_dict->FindList("allowances_found");
  ASSERT_TRUE(nullptr != allowances_found_ptr);
  ASSERT_TRUE(allowances_found_ptr->empty());
}

TEST_F(EthAllowanceManagerUnitTest, AllowancesIgnoreWrongAmountData) {
  // Generates logs with wrong amount format.
  auto generate_wrong_amount_responses = base::BindLambdaForTesting(
      [&](const std::vector<std::string>& eth_account_address,
          const TokenListMap& token_list_map) {
        return PrepareResponses(
            eth_allowance_detected_response, eth_account_address,
            token_list_map,
            base::BindLambdaForTesting(
                [&](base::Value::Dict allovance_item,
                    const mojom::BlockchainTokenPtr& tkn, uint256_t& log_index,
                    const std::string& addr, const std::string& chain_id,
                    base::Value::List* pr_result_ptr) {
                  FillAllowanceLogItem(allovance_item, tkn->contract_address,
                                       ++log_index, addr, 1);
                  // Set amount to wrong format.
                  allovance_item.Set("data", "0");
                  // Add allowance record.
                  pr_result_ptr->Append(std::move(allovance_item));
                }));
      });

  auto allowances_validation = base::BindLambdaForTesting(
      [](const std::vector<mojom::AllowanceInfoPtr>& allowances) {
        // There are no any allowances found.
        ASSERT_TRUE(allowances.empty());
      });

  TestAllowancesLoading(token_list_json, generate_wrong_amount_responses, 1, 1,
                        allowances_validation, get_block_response);

  const auto& allowance_cashe_dict =
      GetPrefs()->GetDict(kBraveWalletEthAllowancesCache);
  const auto* chain_id_dict = allowance_cashe_dict.FindDict("0x1");
  ASSERT_TRUE(nullptr != chain_id_dict);
  const auto* allowances_found_ptr =
      chain_id_dict->FindList("allowances_found");
  ASSERT_TRUE(nullptr != allowances_found_ptr);
  ASSERT_TRUE(allowances_found_ptr->empty());
}

TEST_F(EthAllowanceManagerUnitTest, NoAllowancesLoaded) {
  // Generates empty logs response.
  auto generate_empty_response = base::BindLambdaForTesting(
      [&](const std::vector<std::string>& eth_account_address,
          const TokenListMap& token_list_map) {
        return PrepareResponses(
            eth_allowance_detected_response, eth_account_address,
            token_list_map,
            base::BindLambdaForTesting(
                [&](base::Value::Dict allovance_item,
                    const mojom::BlockchainTokenPtr& tkn, uint256_t& log_index,
                    const std::string& addr, const std::string& chain_id,
                    base::Value::List* pr_result_ptr) {
                  // Do nothing
                }));
      });

  auto allowances_validation = base::BindLambdaForTesting(
      [](const std::vector<mojom::AllowanceInfoPtr>& allowances) {
        // There are no any allowances found.
        ASSERT_TRUE(allowances.empty());
      });

  TestAllowancesLoading(token_list_json, generate_empty_response, 1, 1,
                        allowances_validation, get_block_response);

  ASSERT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletEthAllowancesCache));
  const auto& allowance_cashe_dict =
      GetPrefs()->GetDict(kBraveWalletEthAllowancesCache);
  const auto* chain_id_dict = allowance_cashe_dict.FindDict("0x1");
  ASSERT_TRUE(nullptr != chain_id_dict);
  const auto* allowances_found_ptr =
      chain_id_dict->FindList("allowances_found");
  ASSERT_TRUE(nullptr != allowances_found_ptr);
  ASSERT_TRUE(allowances_found_ptr->empty());
}

TEST_F(EthAllowanceManagerUnitTest, NoAllowancesLoadedForSkippedNetwork) {
  // Generates logs response with response.
  auto generate_error_response = base::BindLambdaForTesting(
      [&](const std::vector<std::string>& eth_account_address,
          const TokenListMap& token_list_map) {
        return std::map<GURL, std::map<std::string, std::string>>({
            {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
             {{"0x3333333333333333333333333333333333333333",
               std::string(eth_allowance_error_response)}}},
        });
      });

  auto allowances_validation = base::BindLambdaForTesting(
      [](const std::vector<mojom::AllowanceInfoPtr>& allowances) {
        // There are no any allowances found.
        ASSERT_TRUE(allowances.empty());
      });

  TestAllowancesLoading(token_list_json, generate_error_response, 0, 5,
                        allowances_validation, get_block_response);

  ASSERT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletEthAllowancesCache));
  const auto& allowance_cashe_dict =
      GetPrefs()->GetDict(kBraveWalletEthAllowancesCache);
  const auto* chain_id_dict = allowance_cashe_dict.FindDict("0x1");
  ASSERT_TRUE(nullptr != chain_id_dict);
  const auto* allowances_found_ptr =
      chain_id_dict->FindList("allowances_found");
  ASSERT_TRUE(nullptr != allowances_found_ptr);
  ASSERT_TRUE(allowances_found_ptr->empty());
}

TEST_F(EthAllowanceManagerUnitTest, AllowancesLoadingReset) {
  std::vector<mojom::AllowanceInfoPtr> expected_allowances;
  // Generates one allowance log per ETH account address.
  auto generate_responses = base::BindLambdaForTesting(
      [&](const std::vector<std::string>& eth_account_address,
          const TokenListMap& token_list_map) {
        return PrepareResponses(
            eth_allowance_detected_response, eth_account_address,
            token_list_map,
            base::BindLambdaForTesting(
                [&](base::Value::Dict allovance_item,
                    const mojom::BlockchainTokenPtr& tkn, uint256_t& log_index,
                    const std::string& addr, const std::string& chain_id,
                    base::Value::List* pr_result_ptr) {
                  FillAllowanceLogItem(allovance_item, tkn->contract_address,
                                       ++log_index, addr, 1);
                  expected_allowances.push_back(
                      GetAllowanceInfo(allovance_item, chain_id));
                  pr_result_ptr->Append(std::move(allovance_item));
                }));
      });

  auto allowances_validation = base::BindLambdaForTesting(
      [&expected_allowances](
          const std::vector<mojom::AllowanceInfoPtr>& allowances) {
        ASSERT_EQ(expected_allowances.size(), 2UL);
        ASSERT_TRUE(allowances.empty());
      });
  TestAllowancesLoading(token_list_json, generate_responses, 2, 2,
                        allowances_validation, get_block_response, 1);

  const auto& allowance_cashe_dict =
      GetPrefs()->GetDict(kBraveWalletEthAllowancesCache);
  const auto* chain_id_dict = allowance_cashe_dict.FindDict("0x1");
  ASSERT_TRUE(nullptr == chain_id_dict);
}

}  // namespace brave_wallet
