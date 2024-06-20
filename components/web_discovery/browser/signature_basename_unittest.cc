/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/signature_basename.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/test/task_environment.h"
#include "brave/components/web_discovery/browser/regex_util.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "brave/components/web_discovery/browser/wdp_service.h"
#include "components/prefs/testing_pref_service.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

namespace {

constexpr size_t kMsInHour = 60 * 60 * 1000;

int GetPeriodHoursSinceEpoch(size_t period_hours) {
  auto hours_since_epoch =
      base::Time::Now().InMillisecondsSinceUnixEpoch() / kMsInHour;
  auto epoch_period_hours = period_hours * (hours_since_epoch / period_hours);
  return epoch_period_hours;
}

base::TimeDelta TimeUntilNextPeriod(int period, int epoch_period_hours) {
  return base::Time::FromMillisecondsSinceUnixEpoch(
             (epoch_period_hours + period) * static_cast<int64_t>(kMsInHour)) -
         base::Time::Now();
}

std::vector<const uint8_t> GenerateExpectedBasename(std::string action,
                                                    int period,
                                                    int limit,
                                                    base::Value::List key_list,
                                                    size_t actual_count,
                                                    int epoch_period_hours) {
  base::Value::List expected_tag_list;
  expected_tag_list.Append(action);
  expected_tag_list.Append(period);
  expected_tag_list.Append(limit);
  expected_tag_list.Append(std::move(key_list));
  expected_tag_list.Append(static_cast<int>(epoch_period_hours));
  expected_tag_list.Append(static_cast<int>(actual_count));

  std::string tag_json;
  EXPECT_TRUE(base::JSONWriter::Write(base::Value(std::move(expected_tag_list)),
                                      &tag_json));

  auto tag_hash = crypto::SHA256HashString(tag_json);
  return std::vector<const uint8_t>(tag_hash.begin(), tag_hash.end());
}

base::Value::Dict GeneratePayload(std::string action,
                                  base::Value::Dict inner_payload) {
  base::Value::Dict payload;
  payload.Set("action", action);
  payload.Set("payload", std::move(inner_payload));
  return payload;
}

}  // namespace

class WebDiscoverySignatureBasenameTest : public testing::Test {
 public:
  WebDiscoverySignatureBasenameTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~WebDiscoverySignatureBasenameTest() override = default;

  // testing::Test:
  void SetUp() override {
    WDPService::RegisterProfilePrefs(profile_prefs_.registry());

    auto action_config = std::make_unique<SourceMapActionConfig>();
    action_config->keys.push_back("q->url");
    action_config->period = 24;
    action_config->limit = 3;
    server_config_.source_map_actions["query"] = std::move(action_config);

    action_config = std::make_unique<SourceMapActionConfig>();
    action_config->keys.push_back("field->obj");
    action_config->period = 12;
    action_config->limit = 1;
    server_config_.source_map_actions["img"] = std::move(action_config);

    action_config = std::make_unique<SourceMapActionConfig>();
    action_config->keys.push_back("field");
    action_config->period = 12;
    action_config->limit = 1;
    server_config_.source_map_actions["basic"] = std::move(action_config);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  ServerConfig server_config_;
  TestingPrefServiceSimple profile_prefs_;
  RegexUtil regex_util_;
};

TEST_F(WebDiscoverySignatureBasenameTest, BasenameForURL) {
  base::flat_set<size_t> used_counts;

  base::Value::List key_list;
  key_list.Append("examplecomtesttestpage");

  base::Value::Dict inner_payload;
  inner_payload.Set("q", "https://www.EXample.com/test test/page");
  auto payload = GeneratePayload("query", std::move(inner_payload));

  auto epoch_period_hours = GetPeriodHoursSinceEpoch(24);
  for (size_t i = 0; i < 3; i++) {
    auto actual_basename =
        GenerateBasename(&profile_prefs_, server_config_, regex_util_, payload);
    ASSERT_TRUE(actual_basename);
    EXPECT_LT(actual_basename->count, 3u);
    EXPECT_FALSE(used_counts.contains(actual_basename->count));

    auto expected_basename =
        GenerateExpectedBasename("query", 24, 3, key_list.Clone(),
                                 actual_basename->count, epoch_period_hours);

    EXPECT_EQ(actual_basename->basename, expected_basename);
    used_counts.insert(actual_basename->count);

    SaveBasenameCount(&profile_prefs_, actual_basename->count_tag_hash,
                      actual_basename->count);
  }

  EXPECT_FALSE(
      GenerateBasename(&profile_prefs_, server_config_, regex_util_, payload));
}

TEST_F(WebDiscoverySignatureBasenameTest, BasenameNotSaved) {
  base::Value::Dict inner_payload;
  inner_payload.Set("q", "https://www.example.com/test/page");
  auto payload = GeneratePayload("query", std::move(inner_payload));

  for (size_t i = 0; i < 10; i++) {
    EXPECT_TRUE(GenerateBasename(&profile_prefs_, server_config_, regex_util_,
                                 payload));
  }
}

TEST_F(WebDiscoverySignatureBasenameTest, BasenameLimitExpiry) {
  base::Value::Dict inner_payload;
  inner_payload.Set("q", "https://www.example.com/test/page");
  auto payload = GeneratePayload("query", std::move(inner_payload));

  for (size_t i = 0; i < 3; i++) {
    auto epoch_period_hours = GetPeriodHoursSinceEpoch(24);
    for (size_t j = 0; j < 3; j++) {
      auto basename = GenerateBasename(&profile_prefs_, server_config_,
                                       regex_util_, payload);
      ASSERT_TRUE(basename);
      SaveBasenameCount(&profile_prefs_, basename->count_tag_hash,
                        basename->count);
    }

    auto time_until_next_period = TimeUntilNextPeriod(24, epoch_period_hours);
    task_environment_.AdvanceClock(time_until_next_period / 2);
    EXPECT_FALSE(GenerateBasename(&profile_prefs_, server_config_, regex_util_,
                                  payload));
    task_environment_.AdvanceClock(time_until_next_period / 2);
  }
}

TEST_F(WebDiscoverySignatureBasenameTest, BasenameForFlattenedObj) {
  auto field_obj = base::JSONReader::Read(R"({
    "this": {
      "is": {
        "test": "object"
      }
    },
    "example1": [ 1, 2 ],
    "example2": { "abc": "def" }
  })");
  ASSERT_TRUE(field_obj);
  auto expected_flattened_obj = base::JSONReader::Read(R"([
    [
      [["example1", "0"], 1],
      [["example1", "1"], 2],
      [["example2", "abc"], "def"],
      [["this", "is", "test"], "object"]
    ]
  ])");
  ASSERT_TRUE(expected_flattened_obj);

  base::Value::Dict inner_payload;
  inner_payload.Set("field", std::move(*field_obj));
  auto payload = GeneratePayload("img", std::move(inner_payload));

  auto actual_basename =
      GenerateBasename(&profile_prefs_, server_config_, regex_util_, payload);
  ASSERT_TRUE(actual_basename);
  EXPECT_EQ(actual_basename->count, 0u);

  auto epoch_period_hours = GetPeriodHoursSinceEpoch(24);
  auto expected_basename = GenerateExpectedBasename(
      "img", 12, 1, expected_flattened_obj->GetList().Clone(), 0u,
      epoch_period_hours);

  EXPECT_EQ(actual_basename->basename, expected_basename);

  SaveBasenameCount(&profile_prefs_, actual_basename->count_tag_hash,
                    actual_basename->count);

  EXPECT_FALSE(
      GenerateBasename(&profile_prefs_, server_config_, regex_util_, payload));
}

TEST_F(WebDiscoverySignatureBasenameTest, BasenameSimple) {
  base::Value::List key_list;
  key_list.Append("test");

  base::Value::Dict inner_payload;
  inner_payload.Set("field", "test");
  auto payload = GeneratePayload("basic", std::move(inner_payload));

  auto actual_basename =
      GenerateBasename(&profile_prefs_, server_config_, regex_util_, payload);
  ASSERT_TRUE(actual_basename);
  EXPECT_EQ(actual_basename->count, 0u);

  auto epoch_period_hours = GetPeriodHoursSinceEpoch(24);
  auto expected_basename = GenerateExpectedBasename(
      "basic", 12, 1, std::move(key_list), 0u, epoch_period_hours);

  EXPECT_EQ(actual_basename->basename, expected_basename);
}

TEST_F(WebDiscoverySignatureBasenameTest, BasenameNoAction) {
  base::Value::Dict inner_payload;
  inner_payload.Set("field", "test");
  auto payload = GeneratePayload("bad_action", std::move(inner_payload));

  ASSERT_FALSE(
      GenerateBasename(&profile_prefs_, server_config_, regex_util_, payload));
}

}  // namespace web_discovery
