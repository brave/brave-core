/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_recover_wallet/get_recover_wallet.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetRecoverWalletTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class GetRecoverWalletTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetRecoverWallet> wallet_;

  GetRecoverWalletTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    wallet_ = std::make_unique<GetRecoverWallet>(mock_ledger_impl_.get());
  }
};

TEST_F(GetRecoverWalletTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](type::UrlRequestPtr, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.body = R"({
              "paymentId": "d59d4b69-f66e-4ee8-9c88-1c522e02ffd3",
              "walletProvider": {
                "id": "a9d12d76-2b6d-4f8b-99df-bb801bff9407",
                "name": "brave"
              },
              "altcurrency": "BAT",
              "publicKey": "79d7da2a756cc8d9403d0353a64fae5698e01b44a2c2745"
            })";
            std::move(callback).Run(response);
          }));

  wallet_->Request("79d7da2a756cc8d9403d0353a64fae5698e01b44a2c2745",
                   [](type::Result result, const std::string& payment_id) {
                     EXPECT_EQ(result, type::Result::LEDGER_OK);
                     EXPECT_EQ(payment_id,
                               "d59d4b69-f66e-4ee8-9c88-1c522e02ffd3");
                   });
}

TEST_F(GetRecoverWalletTest, ServerOKAnonymousUpholdWallet) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](type::UrlRequestPtr, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.body = R"({
              "paymentId": "d59d4b69-f66e-4ee8-9c88-1c522e02ffd3",
              "walletProvider": {
                "id": "a9d12d76-2b6d-4f8b-99df-bb801bff9407",
                "name": "uphold"
              },
              "altcurrency": "BAT",
              "publicKey": "79d7da2a756cc8d9403d0353a64fae5698e01b44a2c2745"
            })";
            std::move(callback).Run(response);
          }));

  wallet_->Request("79d7da2a756cc8d9403d0353a64fae5698e01b44a2c2745",
                   [](type::Result result, const std::string& payment_id) {
                     EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                     EXPECT_EQ(payment_id, "");
                   });
}

TEST_F(GetRecoverWalletTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](type::UrlRequestPtr, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 400;
            response.body = "";
            std::move(callback).Run(response);
          }));

  wallet_->Request("79d7da2a756cc8d9403d0353a64fae5698e01b44a2c2745",
                   [](type::Result result, const std::string& payment_id) {
                     EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                     EXPECT_EQ(payment_id, "");
                   });
}

TEST_F(GetRecoverWalletTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](type::UrlRequestPtr, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 404;
            response.body = "";
            std::move(callback).Run(response);
          }));

  wallet_->Request("79d7da2a756cc8d9403d0353a64fae5698e01b44a2c2745",
                   [](type::Result result, const std::string& payment_id) {
                     EXPECT_EQ(result, type::Result::NOT_FOUND);
                     EXPECT_EQ(payment_id, "");
                   });
}

TEST_F(GetRecoverWalletTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](type::UrlRequestPtr, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 453;
            response.body = "";
            std::move(callback).Run(response);
          }));

  wallet_->Request("79d7da2a756cc8d9403d0353a64fae5698e01b44a2c2745",
                   [](type::Result result, const std::string& payment_id) {
                     EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                     EXPECT_EQ(payment_id, "");
                   });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
