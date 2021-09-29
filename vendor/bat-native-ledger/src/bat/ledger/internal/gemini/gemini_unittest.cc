/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <tuple>
#include <utility>

#include "base/json/json_reader.h"
#include "base/test/task_environment.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/database/database_mock.h"
#include "bat/ledger/internal/gemini/gemini.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiTest*

using ::testing::_;
using ::testing::Return;
using ::testing::Test;
using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

namespace ledger {
namespace gemini {

class GeminiTest : public Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<database::MockDatabase> mock_database_;
  std::unique_ptr<Gemini> gemini_;

  GeminiTest()
      : mock_ledger_client_{std::make_unique<MockLedgerClient>()},
        mock_ledger_impl_{
            std::make_unique<MockLedgerImpl>(mock_ledger_client_.get())},
        mock_database_{
            std::make_unique<database::MockDatabase>(mock_ledger_impl_.get())},
        gemini_{std::make_unique<Gemini>(mock_ledger_impl_.get())} {}
};

absl::optional<type::WalletStatus> GetStatusFromJSON(
    const std::string& gemini_wallet) {
  auto value = base::JSONReader::Read(gemini_wallet);
  if (value && value->is_dict()) {
    base::DictionaryValue* dictionary = nullptr;
    if (value->GetAsDictionary(&dictionary)) {
      if (auto status = dictionary->FindIntKey("status")) {
        return static_cast<type::WalletStatus>(*status);
      }
    }
  }

  return {};
}

template <typename ParamType>
std::string NameSuffixGenerator(const TestParamInfo<ParamType>& info) {
  return std::get<0>(info.param);
}

// clang-format off
using DisconnectWalletParamType = std::tuple<
    std::string,                        // test name suffix
    std::string,                        // input Gemini wallet
    std::string,                        // input Rewards wallet
    // NOLINTNEXTLINE
    type::UrlResponse,                  // Rewards UnLink (Claim) Wallet response
    type::Result,                       // expected result
    absl::optional<type::WalletStatus>  // expected status
>;

struct DisconnectGeminiWallet : GeminiTest,
                                WithParamInterface<DisconnectWalletParamType> {};  // NOLINT

INSTANTIATE_TEST_SUITE_P(
  GeminiTest,
  DisconnectGeminiWallet,
  Values(
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet succeeded. (NOT_CONNECTED)
      "00_NOT_CONNECTED_unlink_wallet_succeeded",
      R"({ "status": 0 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      type::Result::LEDGER_OK,
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet failed. (NOT_CONNECTED)
      "01_NOT_CONNECTED_unlink_wallet_failed",
      R"({ "status": 0 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::LEDGER_ERROR,
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet succeeded. (VERIFIED)
      "02_VERIFIED_unlink_wallet_succeeded",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      type::Result::LEDGER_OK,
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet failed. (VERIFIED)
      "03_VERIFIED_unlink_wallet_failed",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::LEDGER_ERROR,
      type::WalletStatus::VERIFIED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet succeeded. (DISCONNECTED_VERIFIED)
      "04_DISCONNECTED_VERIFIED_unlink_wallet_succeeded",
      R"({ "status": 4 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      type::Result::LEDGER_OK,
      type::WalletStatus::NOT_CONNECTED
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet failed. (DISCONNECTED_VERIFIED)
      "05_DISCONNECTED_VERIFIED_unlink_wallet_failed",
      R"({ "status": 4 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      type::Result::LEDGER_ERROR,
      type::WalletStatus::DISCONNECTED_VERIFIED
    }),
  NameSuffixGenerator<DisconnectWalletParamType>
);
// clang-format on

TEST_P(DisconnectGeminiWallet, Paths) {
  const auto& params = GetParam();
  std::string gemini_wallet = std::get<1>(params);
  const auto& input_rewards_wallet = std::get<2>(params);
  const auto& rewards_unlink_wallet_response = std::get<3>(params);
  const auto expected_result = std::get<4>(params);
  const auto expected_status = std::get<5>(params);

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletGemini))
      .WillByDefault(
          [&] { return FakeEncryption::Base64EncryptString(gemini_wallet); });

  ON_CALL(*mock_ledger_client_, SetStringState(state::kWalletGemini, _))
      .WillByDefault([&](const std::string&, const std::string& value) {
        gemini_wallet = *FakeEncryption::Base64DecryptString(value);
        return true;
      });

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletBrave))
      .WillByDefault(Return(input_rewards_wallet));

  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          [&](type::UrlRequestPtr, client::LoadURLCallback callback) {
            callback(rewards_unlink_wallet_response);
          });

  mock_ledger_impl_->SetInitializedForTesting();

  ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(Return(mock_database_.get()));

  mock_ledger_impl_->DisconnectWallet(
      constant::kWalletGemini, [&](type::Result result) {
        ASSERT_EQ(result, expected_result);

        const auto status = GetStatusFromJSON(gemini_wallet);
        if (status && expected_status) {
          ASSERT_EQ(*status, *expected_status);
        } else {
          ASSERT_TRUE(!status && !expected_status);
        }
      });
}

}  // namespace gemini
}  // namespace ledger
