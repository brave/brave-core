/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_ordinals_rpc.h"

#include <memory>
#include <string>

#include "base/test/mock_callback.h"
#include "base/test/scoped_command_line.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/bitcoin_ordinals_rpc_responses.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "components/grit/brave_components_strings.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using testing::Eq;
using testing::Truly;

namespace brave_wallet {
namespace {
auto MatchError(const std::string& error) {
  return Truly([=](auto& arg) { return arg.error() == error; });
}

std::string InternalError() {
  return l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
}

std::string ParsingError() {
  return l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR);
}

}  // namespace

class BitcoinOrdinalsRpcUnitTest : public testing::Test {
 public:
  BitcoinOrdinalsRpcUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~BitcoinOrdinalsRpcUnitTest() override = default;

  void SetUp() override {
    command_line_.GetProcessCommandLine()->AppendSwitchASCII(
        switches::kBitcoinOrdinalsMainnetRpcUrl, mainnet_rpc_url_);
    command_line_.GetProcessCommandLine()->AppendSwitchASCII(
        switches::kBitcoinOrdinalsTestnetRpcUrl, testnet_rpc_url_);

    bitcoin_ordinals_rpc_ =
        std::make_unique<bitcoin_ordinals_rpc::BitcoinOrdinalsRpc>(
            shared_url_loader_factory_);
  }

 protected:
  base::test::ScopedCommandLine command_line_;
  std::string mainnet_rpc_url_ = "https://ordinals.mainnet.com/";
  std::string testnet_rpc_url_ = "https://ordinals.testnet.com/";
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<bitcoin_ordinals_rpc::BitcoinOrdinalsRpc>
      bitcoin_ordinals_rpc_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(BitcoinOrdinalsRpcUnitTest, Throttling) {
  using GetOutpointInfoCallback =
      bitcoin_ordinals_rpc::BitcoinOrdinalsRpc::GetOutpointInfoCallback;

  BitcoinOutpoint outpoint = *BitcoinOutpoint::FromRpc(
      "cd0410e7a00180c18efb4360170cb00d843fba33ff24ed35d3476d98c5babd8b", "1");

  bitcoin_ordinals_rpc::OutpointInfo outpoint_info;
  outpoint_info.inscriptions.push_back("123");

  // For mainnet there is no throttling and always 5 requests.
  struct {
    const bool mainnet;
    const char* param;
    const size_t expected_size;
  } test_cases[] = {{true, "0", 5},  {true, "3", 3},  {true, "10", 5},
                    {false, "0", 5}, {false, "3", 3}, {false, "10", 5}};

  for (auto& test_case : test_cases) {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeaturesAndParameters(
        {{features::kBraveWalletBitcoinOrdinalsFeature,
          {{features::kBitcoinOrdinalsRpcThrottle.name, test_case.param}}}},
        {});

    base::MockCallback<GetOutpointInfoCallback> callback;

    const std::string req_url =
        (test_case.mainnet ? mainnet_rpc_url_ : testnet_rpc_url_) +
        "output/"
        "cd0410e7a00180c18efb4360170cb00d843fba33ff24ed35d3476d98c5babd8b:1";

    url_loader_factory_.ClearResponses();

    auto* chain_id =
        (test_case.mainnet ? mojom::kBitcoinMainnet : mojom::kBitcoinTestnet);

    // GetOutpointInfo works.
    EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                  return arg.value().ToValue() == outpoint_info.ToValue();
                })))
        .Times(5);
    bitcoin_ordinals_rpc_->GetOutpointInfo(chain_id, outpoint, callback.Get());
    bitcoin_ordinals_rpc_->GetOutpointInfo(chain_id, outpoint, callback.Get());
    bitcoin_ordinals_rpc_->GetOutpointInfo(chain_id, outpoint, callback.Get());
    bitcoin_ordinals_rpc_->GetOutpointInfo(chain_id, outpoint, callback.Get());
    bitcoin_ordinals_rpc_->GetOutpointInfo(chain_id, outpoint, callback.Get());
    task_environment_.RunUntilIdle();

    EXPECT_EQ(url_loader_factory_.pending_requests()->size(),
              test_case.expected_size);
    url_loader_factory_.AddResponse(req_url, GetJSON(outpoint_info.ToValue()));
    task_environment_.RunUntilIdle();
    testing::Mock::VerifyAndClearExpectations(&callback);
  }
}

TEST_F(BitcoinOrdinalsRpcUnitTest, GetOutpointInfo) {
  using GetOutpointInfoCallback =
      bitcoin_ordinals_rpc::BitcoinOrdinalsRpc::GetOutpointInfoCallback;
  base::MockCallback<GetOutpointInfoCallback> callback;

  BitcoinOutpoint outpoint = *BitcoinOutpoint::FromRpc(
      "cd0410e7a00180c18efb4360170cb00d843fba33ff24ed35d3476d98c5babd8b", "1");

  bitcoin_ordinals_rpc::OutpointInfo outpoint_info;
  outpoint_info.inscriptions.push_back("123");

  const std::string req_url =
      mainnet_rpc_url_ +
      "output/"
      "cd0410e7a00180c18efb4360170cb00d843fba33ff24ed35d3476d98c5babd8b:1";

  // GetOutpointInfo works.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                return arg.value().ToValue() == outpoint_info.ToValue();
              })));
  url_loader_factory_.AddResponse(req_url, GetJSON(outpoint_info.ToValue()));
  bitcoin_ordinals_rpc_->GetOutpointInfo(mojom::kBitcoinMainnet, outpoint,
                                         callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Invalid value returned.
  EXPECT_CALL(callback, Run(MatchError(ParsingError())));
  url_loader_factory_.AddResponse(req_url, "some string");
  bitcoin_ordinals_rpc_->GetOutpointInfo(mojom::kBitcoinMainnet, outpoint,
                                         callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // HTTP Error returned.
  EXPECT_CALL(callback, Run(MatchError(InternalError())));
  url_loader_factory_.AddResponse(req_url, "123",
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  bitcoin_ordinals_rpc_->GetOutpointInfo(mojom::kBitcoinMainnet, outpoint,
                                         callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Testnet works.
  EXPECT_CALL(callback, Run(Truly([&](auto& arg) {
                return arg.value().ToValue() == outpoint_info.ToValue();
              })));
  url_loader_factory_.AddResponse(
      testnet_rpc_url_ +
          "output/"
          "cd0410e7a00180c18efb4360170cb00d843fba33ff24ed35d3476d98c5babd8b:1",
      GetJSON(outpoint_info.ToValue()));
  bitcoin_ordinals_rpc_->GetOutpointInfo(mojom::kBitcoinTestnet, outpoint,
                                         callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

}  // namespace brave_wallet
