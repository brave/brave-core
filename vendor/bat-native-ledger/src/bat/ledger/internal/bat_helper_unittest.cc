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

TEST(BatHelperTest, ReconcileDirectionSerialization) {
  braveledger_bat_helper::RECONCILE_DIRECTION direction("duck.com", 2.5, "BAT");
  std::string json;
  braveledger_bat_helper::saveToJsonString(direction, &json);
  ASSERT_FALSE(json.empty());

  braveledger_bat_helper::RECONCILE_DIRECTION direction_from_json;
  ASSERT_TRUE(direction_from_json.loadFromJson(json));

  ASSERT_EQ(direction_from_json.publisher_key_, "duck.com");
  ASSERT_EQ(direction_from_json.amount_, 2.5);
  ASSERT_EQ(direction_from_json.currency_, "BAT");
}

TEST(BatHelperTest, ReconcileDirectionDeserializationOfIntAmount) {
  // An older serialization of RECONCILE_DIRECTION stored the amount as an int.
  // Ensure that the older format can still be deserialized.
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  writer.StartObject();

  writer.String("publisher_key");
  writer.String("duckduckgo.com");

  writer.String("amount");
  writer.Int(10);

  writer.String("currency");
  writer.String("BAT");

  writer.EndObject();

  std::string json = buffer.GetString();

  braveledger_bat_helper::RECONCILE_DIRECTION direction_from_json;
  ASSERT_TRUE(direction_from_json.loadFromJson(json));

  ASSERT_EQ(direction_from_json.publisher_key_, "duckduckgo.com");
  ASSERT_EQ(direction_from_json.amount_, 10);
  ASSERT_EQ(direction_from_json.currency_, "BAT");
}

TEST(BatHelperTest, CurrentReconcileDirectionsSerialization) {
  braveledger_bat_helper::CURRENT_RECONCILE reconcile;
  reconcile.directions_ = {
    { "duckduckgo.com", 2.5, "BAT" },
    { "3zsistemi.si", 3.75, "BAT" },
  };
  std::string json;
  braveledger_bat_helper::saveToJsonString(reconcile, &json);
  ASSERT_FALSE(json.empty());

  braveledger_bat_helper::CURRENT_RECONCILE reconcile_from_json;
  ASSERT_TRUE(reconcile_from_json.loadFromJson(json));

  const auto& directions_from_json = reconcile_from_json.directions_;
  ASSERT_EQ(directions_from_json[0].publisher_key_, "duckduckgo.com");
  ASSERT_EQ(directions_from_json[0].amount_, 2.5);
  ASSERT_EQ(directions_from_json[0].currency_, "BAT");

  ASSERT_EQ(directions_from_json[1].publisher_key_, "3zsistemi.si");
  ASSERT_EQ(directions_from_json[1].amount_, 3.75);
  ASSERT_EQ(directions_from_json[1].currency_, "BAT");
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
