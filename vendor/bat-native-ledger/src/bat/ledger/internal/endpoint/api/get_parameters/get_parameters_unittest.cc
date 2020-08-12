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
#include "bat/ledger/internal/endpoint/api/get_parameters/get_parameters.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetParametersTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace api {

class GetParametersTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetParameters> parameters_;

  GetParametersTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<bat_ledger::MockLedgerImpl>(mock_ledger_client_.get());
    parameters_ = std::make_unique<GetParameters>(mock_ledger_impl_.get());
  }
};

TEST_F(GetParametersTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 200;
            response.url = url;
            response.body = R"({
              "batRate": 0.2476573499489187,
              "autocontribute": {
                "choices": [
                  5,
                  10,
                  15
                ],
                "defaultChoice": 20
              },
              "tips": {
                "defaultTipChoices": [
                  1,
                  10,
                  100
                ],
                "defaultMonthlyChoices": [
                  5,
                  10,
                  15
                ]
              }
            })";
            callback(response);
          }));

  parameters_->Request([](
      const ledger::Result result,
      const ledger::RewardsParameters& parameters) {
    EXPECT_EQ(result, ledger::Result::LEDGER_OK);
    EXPECT_EQ(parameters.rate, 0.2476573499489187);
    EXPECT_EQ(parameters.auto_contribute_choice, 20);
    EXPECT_EQ(parameters.auto_contribute_choices.size(), 3ul);
    EXPECT_EQ(parameters.auto_contribute_choices[0], 5);
    EXPECT_EQ(parameters.auto_contribute_choices[1], 10);
    EXPECT_EQ(parameters.auto_contribute_choices[2], 15);
    EXPECT_EQ(parameters.tip_choices.size(), 3ul);
    EXPECT_EQ(parameters.tip_choices[0], 1);
    EXPECT_EQ(parameters.tip_choices[1], 10);
    EXPECT_EQ(parameters.tip_choices[2], 100);
    EXPECT_EQ(parameters.monthly_tip_choices.size(), 3ul);
    EXPECT_EQ(parameters.monthly_tip_choices[0], 5);
    EXPECT_EQ(parameters.monthly_tip_choices[1], 10);
    EXPECT_EQ(parameters.monthly_tip_choices[2], 15);
  });
}

TEST_F(GetParametersTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 400;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  parameters_->Request([](
      const ledger::Result result,
      const ledger::RewardsParameters& parameters) {
    EXPECT_EQ(result, ledger::Result::RETRY_SHORT);
  });
}

TEST_F(GetParametersTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 500;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  parameters_->Request([](
      const ledger::Result result,
      const ledger::RewardsParameters& parameters) {
    EXPECT_EQ(result, ledger::Result::RETRY_SHORT);
  });
}

TEST_F(GetParametersTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 453;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  parameters_->Request([](
      const ledger::Result result,
      const ledger::RewardsParameters& parameters) {
    EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
  });
}

TEST_F(GetParametersTest, WrongListValues) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 200;
            response.url = url;
            response.body = R"({
              "batRate": 0.2476573499489187,
              "autocontribute": {
                "choices": [
                  "5",
                  "10",
                  "15",
                  "20",
                  "25",
                  "50",
                  "100"
                ],
                "defaultChoice": 20
              },
              "tips": {
                "defaultTipChoices": [
                  "1",
                  "10",
                  "100"
                ],
                "defaultMonthlyChoices": [
                  "1",
                  "10",
                  "100"
                ]
              }
            })";
            callback(response);
          }));

  parameters_->Request([](
      const ledger::Result result,
      const ledger::RewardsParameters& parameters) {
    EXPECT_EQ(result, ledger::Result::LEDGER_OK);
    EXPECT_EQ(parameters.rate, 0.2476573499489187);
    EXPECT_EQ(parameters.auto_contribute_choice, 20);
    EXPECT_TRUE(parameters.auto_contribute_choices.empty());
    EXPECT_TRUE(parameters.tip_choices.empty());
    EXPECT_TRUE(parameters.monthly_tip_choices.empty());
  });
}

TEST_F(GetParametersTest, DoubleListValues) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 200;
            response.url = url;
            response.body = R"({
              "batRate": 0.2476573499489187,
              "autocontribute": {
                "choices": [
                  5.0,
                  10.5,
                  15.0
                ],
                "defaultChoice": 20
              },
              "tips": {
                "defaultTipChoices": [
                  1.0,
                  10.5,
                  100.0
                ],
                "defaultMonthlyChoices": [
                  5.0,
                  10.5,
                  15.0
                ]
              }
            })";
            callback(response);
          }));

  parameters_->Request([](
      const ledger::Result result,
      const ledger::RewardsParameters& parameters) {
    EXPECT_EQ(result, ledger::Result::LEDGER_OK);
    EXPECT_EQ(parameters.rate, 0.2476573499489187);
    EXPECT_EQ(parameters.auto_contribute_choice, 20);
    EXPECT_EQ(parameters.auto_contribute_choices[0], 5);
    EXPECT_EQ(parameters.auto_contribute_choices[1], 10.5);
    EXPECT_EQ(parameters.auto_contribute_choices[2], 15);
    EXPECT_EQ(parameters.tip_choices[0], 1);
    EXPECT_EQ(parameters.tip_choices[1], 10.5);
    EXPECT_EQ(parameters.tip_choices[2], 100);
    EXPECT_EQ(parameters.monthly_tip_choices[0], 5);
    EXPECT_EQ(parameters.monthly_tip_choices[1], 10.5);
    EXPECT_EQ(parameters.monthly_tip_choices[2], 15);
  });
}

}  // namespace api
}  // namespace endpoint
}  // namespace ledger
