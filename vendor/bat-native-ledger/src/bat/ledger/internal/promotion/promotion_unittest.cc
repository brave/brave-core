/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/promotion/promotion.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PromotionTest.*

using ::testing::_;
using ::testing::Invoke;
using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_promotion {

std::string GetResponse(const std::string& url) {
  std::map<std::string, std::string> response;

  // Fetch promotions
  response.insert(std::make_pair(
      "https://grant.rewards.brave.com/v1/promotions?"
      "migrate=true&paymentId=this_is_id&platform=",
      R"({
      "promotions":[{
        "id":"36baa4c3-f92d-4121-b6d9-db44cb273a02",
        "createdAt":"2019-10-30T23:17:15.681226Z",
        "expiresAt":"2020-02-29T23:17:15.681226Z",
        "version":5,
        "suggestionsPerGrant":70,
        "approximateValue":"17.5",
        "type":"ugp",
        "available":true,
        "platform":"desktop",
        "publicKeys":["vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE="],
        "legacyClaimed":false
      }]})"));

  return response[url];
}

class PromotionTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<Promotion> promotion_;

  PromotionTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<bat_ledger::MockLedgerImpl>(mock_ledger_client_.get());
    promotion_ = std::make_unique<Promotion>(mock_ledger_impl_.get());
  }

  void SetUp() override {
    const std::string payment_id = "this_is_id";
    ON_CALL(*mock_ledger_impl_, GetPaymentId())
      .WillByDefault(testing::ReturnRefOfCopy(payment_id));

    const std::string wallet_passphrase = "phrase";
    ON_CALL(*mock_ledger_impl_, GetWalletPassphrase())
      .WillByDefault(testing::Return(wallet_passphrase));

    ON_CALL(*mock_ledger_impl_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
        Invoke([](
            const std::string& url,
            const std::vector<std::string>& headers,
            const std::string& content,
            const std::string& contentType,
            const ledger::UrlMethod method,
            ledger::LoadURLCallback callback) {
          callback(200, GetResponse(url), {});
        }));
  }
};

TEST_F(PromotionTest, LegacyPromotionIsNotOverwritten) {
  ledger::FetchPromotionCallback fetch_promotion_callback =
      std::bind(
          [&](ledger::Result result,
              ledger::PromotionList promotions) {
          },
      _1,
      _2);

  bool inserted = false;
  ON_CALL(*mock_ledger_impl_, GetAllPromotions(_))
    .WillByDefault(
        Invoke([&inserted](ledger::GetAllPromotionsCallback callback) {
          auto promotion = ledger::Promotion::New();
          ledger::PromotionMap map;
          if (inserted) {
            const std::string id = "36baa4c3-f92d-4121-b6d9-db44cb273a02";
            promotion->id = id;
            promotion->public_keys =
                "[\"vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE=\"]";
            promotion->legacy_claimed = true;
            promotion->status = ledger::PromotionStatus::ATTESTED;
            map.insert(std::make_pair(id, std::move(promotion)));
          }

          callback(std::move(map));
      }));

  EXPECT_CALL(*mock_ledger_impl_, SavePromotion(_, _)).Times(1);

  promotion_->Fetch(fetch_promotion_callback);
  inserted = true;
  promotion_->Fetch(fetch_promotion_callback);
}


}  // namespace braveledger_promotion
