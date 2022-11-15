/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <tuple>
#include <utility>

#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/database/database_mock.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/promotion/promotion_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=UpholdTest*

using ::testing::_;
using ::testing::AtMost;
using ::testing::Between;
using ::testing::Exactly;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;
using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

namespace ledger {
namespace uphold {

class UpholdTest : public Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<database::MockDatabase> mock_database_;
  std::unique_ptr<promotion::MockPromotion> mock_promotion_;
  std::unique_ptr<Uphold> uphold_;

  UpholdTest()
      : mock_ledger_client_{std::make_unique<MockLedgerClient>()},
        mock_ledger_impl_{
            std::make_unique<MockLedgerImpl>(mock_ledger_client_.get())},
        mock_database_{
            std::make_unique<database::MockDatabase>(mock_ledger_impl_.get())},
        mock_promotion_{std::make_unique<promotion::MockPromotion>(
            mock_ledger_impl_.get())},
        uphold_{std::make_unique<Uphold>(mock_ledger_impl_.get())} {}
};

TEST_F(UpholdTest, FetchBalanceConnectedWallet) {
  const std::string wallet = FakeEncryption::Base64EncryptString(R"({
      "token":"token",
      "address":"address"
      "status":1
    })");
  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(Return(wallet));
  EXPECT_CALL(*mock_ledger_client_, LoadURL(_, _)).Times(0);

  FetchBalanceCallback callback =
      base::BindOnce([](mojom::Result result, double balance) {
        ASSERT_EQ(result, mojom::Result::LEDGER_OK);
        ASSERT_EQ(balance, 0.0);
      });

  uphold_->FetchBalance(std::move(callback));
}

absl::optional<mojom::WalletStatus> GetStatusFromJSON(
    const std::string& uphold_wallet) {
  auto value = base::JSONReader::Read(uphold_wallet);
  if (value && value->is_dict()) {
    if (auto status = value->GetDict().FindInt("status")) {
      return static_cast<mojom::WalletStatus>(*status);
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
    std::string,                        // input Uphold wallet
    std::string,                        // input Rewards wallet
    // NOLINTNEXTLINE
    mojom::UrlResponse,                  // Rewards UnLink (Claim) Wallet response
    mojom::Result,                       // expected result
    absl::optional<mojom::WalletStatus>  // expected status
>;

struct DisconnectUpholdWallet : UpholdTest,
                                WithParamInterface<DisconnectWalletParamType> {};  // NOLINT

INSTANTIATE_TEST_SUITE_P(
  UpholdTest,
  DisconnectUpholdWallet,
  Values(
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet succeeded. (kNotConnected)
      "kNotConnected_unlink_wallet_succeeded",
      R"({ "status": 0 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      mojom::Result::LEDGER_OK,
      mojom::WalletStatus::kNotConnected
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet failed. (kNotConnected)
      "kNotConnected_unlink_wallet_failed",
      R"({ "status": 0 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      mojom::Result::LEDGER_ERROR,
      mojom::WalletStatus::kNotConnected
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet succeeded. (kConnected)
      "kConnected_unlink_wallet_succeeded",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      mojom::Result::LEDGER_OK,
      mojom::WalletStatus::kNotConnected
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet failed. (kConnected)
      "kConnected_unlink_wallet_failed",
      R"({ "status": 2, "token": "0047c2fd8f023e067354dbdb5639ee67acf77150", "address": "962ef3b8-bc12-4619-a349-c8083931b795" })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      mojom::Result::LEDGER_ERROR,
      mojom::WalletStatus::kConnected
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet succeeded. (kLoggedOut)
      "kLoggedOut_unlink_wallet_succeeded",
      R"({ "status": 4 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        {},
        {}
      },
      mojom::Result::LEDGER_OK,
      mojom::WalletStatus::kNotConnected
    },
    // NOLINTNEXTLINE
    DisconnectWalletParamType{  // Rewards UnLink (Claim) Wallet failed. (kLoggedOut)
      "kLoggedOut_unlink_wallet_failed",
      R"({ "status": 4 })",
      R"({ "payment_id": "f375da3c-c206-4f09-9422-665b8e5952db", "recovery_seed": "OG2zYotDSeZ81qLtr/uq5k/GC6WE5/7BclT1lHi4l+w=" })",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR,
        {},
        {}
      },
      mojom::Result::LEDGER_ERROR,
      mojom::WalletStatus::kLoggedOut
    }),
  NameSuffixGenerator<DisconnectWalletParamType>
);
// clang-format on

TEST_P(DisconnectUpholdWallet, Paths) {
  const auto& params = GetParam();
  std::string uphold_wallet = std::get<1>(params);
  const auto& input_rewards_wallet = std::get<2>(params);
  const auto& rewards_unlink_wallet_response = std::get<3>(params);
  const auto expected_result = std::get<4>(params);
  const auto expected_status = std::get<5>(params);

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(
          [&] { return FakeEncryption::Base64EncryptString(uphold_wallet); });

  ON_CALL(*mock_ledger_client_, SetStringState(state::kWalletUphold, _))
      .WillByDefault([&](const std::string&, const std::string& value) {
        uphold_wallet = *FakeEncryption::Base64DecryptString(value);
        return true;
      });

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletBrave))
      .WillByDefault(Return(input_rewards_wallet));

  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          [&](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            std::move(callback).Run(rewards_unlink_wallet_response);
          });

  mock_ledger_impl_->SetInitializedForTesting();

  ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(Return(mock_database_.get()));

  mock_ledger_impl_->DisconnectWallet(
      constant::kWalletUphold, [&](mojom::Result result) {
        ASSERT_EQ(result, expected_result);

        const auto status = GetStatusFromJSON(uphold_wallet);
        if (status && expected_status) {
          ASSERT_EQ(*status, *expected_status);
        } else {
          ASSERT_TRUE(!status && !expected_status);
        }
      });
}

}  // namespace uphold
}  // namespace ledger
