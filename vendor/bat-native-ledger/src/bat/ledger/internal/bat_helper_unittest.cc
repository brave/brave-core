/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatHelperTest.*

TEST(BatHelperTest, isProbiValid) {
  // zero probi
  bool result = braveledger_bat_helper::isProbiValid("0");
  ASSERT_EQ(result, true);

  // 1 BAT
  result = braveledger_bat_helper::isProbiValid("1000000000000000000");
  ASSERT_EQ(result, true);

  // -1 BAT
  result = braveledger_bat_helper::isProbiValid("-1000000000000000000");
  ASSERT_EQ(result, true);

  // not correct probi
  result = braveledger_bat_helper::isProbiValid("10-00000000000000000");
  ASSERT_EQ(result, false);

  // not correct probi
  result = braveledger_bat_helper::isProbiValid("fds000000000");
  ASSERT_EQ(result, false);

  // not correct probi
  result = braveledger_bat_helper::isProbiValid(
      "100000000000000000010000000000000000001000000000000000000");
  ASSERT_EQ(result, false);
}

TEST(BatHelperTest, HasSameDomainAndPath) {
  std::string url("https://k8923479-sub.cdn.ttvwn.net/v1/segment/");
  std::string url_portion("ttvwn.net");
  std::string path("/v1/segment");
  // regular url
  bool result = braveledger_bat_helper::HasSameDomainAndPath(
      url, url_portion, path);
  ASSERT_EQ(result, true);

  // empty url with portion
  url = std::string();
  result = braveledger_bat_helper::HasSameDomainAndPath(
      url, url_portion, path);
  ASSERT_EQ(result, false);

  // url with empty portion and path
  url = "https://k8923479-sub.cdn.ttvwn.net/v1/segment/";
  url_portion = std::string();
  result = braveledger_bat_helper::HasSameDomainAndPath(
      url, url_portion, std::string());
  ASSERT_EQ(result, false);

  // all empty
  url = url_portion = path = std::string();
  result = braveledger_bat_helper::HasSameDomainAndPath(
      url, url_portion, path);
  ASSERT_EQ(result, false);

  // portion not all part of host
  url = "https://k8923479-sub.cdn.ttvwn.net/v1/segment/";
  url_portion = "cdn.ttvwn.net";
  path = "/v1/seg";
  result = braveledger_bat_helper::HasSameDomainAndPath(
      url, url_portion, path);
  ASSERT_EQ(result, true);

  // domain is malicious
  url = "https://www.baddomain.com/k8923479-sub.cdn.ttvwn.net/v1/segment/";
  result = braveledger_bat_helper::HasSameDomainAndPath(
      url, url_portion, path);
  ASSERT_EQ(result, false);

  // portion without leading . matched to malicious
  url_portion = "cdn.ttvwn.net/v1/seg";
  result = braveledger_bat_helper::HasSameDomainAndPath(
      url, url_portion, path);
  ASSERT_EQ(result, false);

  // domain is malicious
  url =
      "https://www.baddomain.com/query?=k8923479-sub.cdn.ttvwn.net/v1/segment/";
  result = braveledger_bat_helper::HasSameDomainAndPath(
      url, url_portion, path);
  ASSERT_EQ(result, false);
}

TEST(BatHelperTest, TransactionSerialization) {
  braveledger_bat_helper::TRANSACTION_ST transaction;
  transaction.viewingId_ = "VIEWING_ID";
  transaction.surveyorId_ = "SURVEYOR_ID";
  transaction.contribution_probi_ = "CONTRIBUTION_PROBI";
  transaction.submissionStamp_ = "SUBMISSION_STAMP";
  transaction.anonizeViewingId_ = "ANONIZE_VIEWING_ID";
  transaction.registrarVK_ = "REGISTRAR_VK";
  transaction.masterUserToken_ = "MASTER_USER_TOKEN";
  transaction.surveyorIds_ = { "SURVEYOR_ID" };
  transaction.votes_ = 5;
  transaction.contribution_rates_ = {
    { "BAT", 1.0 },
    { "ETH", 2.0 },
    { "LTC", 3.0 },
    { "BTC", 4.0 },
    { "USD", 5.0 },
    { "EUR", 6.0 },
  };

  braveledger_bat_helper::TRANSACTION_BALLOT_ST ballot;
  ballot.publisher_ = "brave.com";
  ballot.offset_ = 5;
  transaction.ballots_.push_back(ballot);

  std::string json;
  braveledger_bat_helper::saveToJsonString(transaction, &json);
  ASSERT_FALSE(json.empty());

  braveledger_bat_helper::TRANSACTION_ST deserialized;
  ASSERT_TRUE(deserialized.loadFromJson(json));

  ASSERT_EQ(deserialized.viewingId_, transaction.viewingId_);
  ASSERT_EQ(deserialized.surveyorId_, transaction.surveyorId_);
  ASSERT_EQ(deserialized.contribution_probi_, transaction.contribution_probi_);
  ASSERT_EQ(deserialized.submissionStamp_, transaction.submissionStamp_);
  ASSERT_EQ(deserialized.anonizeViewingId_, transaction.anonizeViewingId_);
  ASSERT_EQ(deserialized.registrarVK_, transaction.registrarVK_);
  ASSERT_EQ(deserialized.masterUserToken_, transaction.masterUserToken_);
  ASSERT_EQ(deserialized.surveyorIds_, transaction.surveyorIds_);
  ASSERT_EQ(deserialized.votes_, transaction.votes_);
  ASSERT_EQ(deserialized.contribution_rates_, transaction.contribution_rates_);
  ASSERT_EQ(deserialized.ballots_[0].publisher_,
    transaction.ballots_[0].publisher_);
  ASSERT_EQ(deserialized.ballots_[0].offset_,
    transaction.ballots_[0].offset_);
}
