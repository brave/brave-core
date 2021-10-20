/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_available/get_available.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetAvailableTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class GetAvailableTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetAvailable> available_;

  GetAvailableTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    available_ = std::make_unique<GetAvailable>(mock_ledger_impl_.get());
  }
};

TEST_F(GetAvailableTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
             "promotions": [
               {
                 "id": "83b3b77b-e7c3-455b-adda-e476fa0656d2",
                 "createdAt": "2020-06-08T15:04:45.352584Z",
                 "expiresAt": "2020-10-08T15:04:45.352584Z",
                 "version": 5,
                 "suggestionsPerGrant": 120,
                 "approximateValue": "30",
                 "type": "ugp",
                 "available": true,
                 "platform": "desktop",
                 "publicKeys": [
                   "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I="
                 ],
                 "legacyClaimed": false
               }
             ]
            })";
            callback(response);
          }));

  available_->Request(
      "macos",
      [](
          const type::Result result,
          type::PromotionList list,
          const std::vector<std::string>& corrupted_promotions) {
        type::Promotion expected_promotion;
        expected_promotion.id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";
        expected_promotion.created_at = 1591628685;
        expected_promotion.expires_at = 1602169485;
        expected_promotion.version = 5;
        expected_promotion.suggestions = 120;
        expected_promotion.approximate_value = 30.0;
        expected_promotion.type = type::PromotionType::UGP;
        expected_promotion.public_keys =
            "[\"dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=\"]";
        expected_promotion.legacy_claimed = false;

        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_TRUE(corrupted_promotions.empty());
        EXPECT_EQ(list.size(), 1ul);
        EXPECT_TRUE(expected_promotion.Equals(*list[0]));
      });
}

TEST_F(GetAvailableTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  available_->Request(
      "macos",
      [](
          const type::Result result,
          type::PromotionList list,
          const std::vector<std::string>& corrupted_promotions) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_TRUE(list.empty());
        EXPECT_TRUE(corrupted_promotions.empty());
      });
}

TEST_F(GetAvailableTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 404;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  available_->Request(
      "macos",
      [](
          const type::Result result,
          type::PromotionList list,
          const std::vector<std::string>& corrupted_promotions) {
        EXPECT_EQ(result, type::Result::NOT_FOUND);
        EXPECT_TRUE(list.empty());
        EXPECT_TRUE(corrupted_promotions.empty());
      });
}

TEST_F(GetAvailableTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  available_->Request(
      "macos",
      [](
          const type::Result result,
          type::PromotionList list,
          const std::vector<std::string>& corrupted_promotions) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_TRUE(list.empty());
        EXPECT_TRUE(corrupted_promotions.empty());
      });
}

TEST_F(GetAvailableTest, ServerErrorRandom) {
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

  available_->Request(
      "macos",
      [](
          const type::Result result,
          type::PromotionList list,
          const std::vector<std::string>& corrupted_promotions) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_TRUE(list.empty());
        EXPECT_TRUE(corrupted_promotions.empty());
      });
}

TEST_F(GetAvailableTest, ServerWrongResponse) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body =  R"({
             "promotions": [
                {
                  "foo": 0
                }
              ]
            })";
            callback(response);
          }));

  available_->Request(
      "macos",
      [](
          const type::Result result,
          type::PromotionList list,
          const std::vector<std::string>& corrupted_promotions) {
        EXPECT_EQ(result, type::Result::CORRUPTED_DATA);
        EXPECT_TRUE(list.empty());
        EXPECT_TRUE(corrupted_promotions.empty());
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
