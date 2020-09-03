/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/endpoint/uphold/get_card/get_card.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetCardTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace uphold {

class GetCardTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetCard> card_;

  GetCardTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    card_ = std::make_unique<GetCard>(mock_ledger_impl_.get());
  }
};

TEST_F(GetCardTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
              "CreatedByApplicationId": "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
              "address": {
               "wire": "XXXXXXXXXX"
              },
              "available": "4.00",
              "balance": "4.00",
              "currency": "BAT",
              "id": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
              "label": "Brave Browser",
              "lastTransactionAt": null,
              "settings": {
               "position": 1,
               "protected": false,
               "starred": true
              },
              "createdByApplicationClientId": "4c2b665ca060d912fec5c735c734859a06118cc8",
              "normalized": [
               {
                 "available": "0.00",
                 "balance": "0.00",
                 "currency": "USD"
               }
              ],
              "wire": [
               {
                 "accountName": "Uphold Europe Limited",
                 "address": {
                   "line1": "Tartu mnt 2",
                   "line2": "10145 Tallinn, Estonia"
                 },
                 "bic": "LHVBEE22",
                 "currency": "EUR",
                 "iban": "EE76 7700 7710 0159 0178",
                 "name": "AS LHV Pank"
               },
               {
                 "accountName": "Uphold HQ, Inc.",
                 "accountNumber": "XXXXXXXXXX",
                 "address": {
                   "line1": "1359 Broadway",
                   "line2": "New York, NY 10018"
                 },
                 "bic": "MCBEUS33",
                 "currency": "USD",
                 "name": "Metropolitan Bank",
                 "routingNumber": "XXXXXXXXX"
               }
              ]
            })";
            callback(response);
          }));

  card_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const double available) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(available, 4.0);
      });
}

TEST_F(GetCardTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 401;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  card_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const double available) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
        EXPECT_EQ(available, 0.0);
      });
}

TEST_F(GetCardTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  card_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const double available) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(available, 0.0);
      });
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
