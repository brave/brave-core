/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/operational_patterns_util.h"

#include <map>
#include <string>

#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/brave_federated/features.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests
// -filter=BraveFederatedOperationalPatternsUtilTest*

namespace brave_federated {

class BraveFederatedOperationalPatternsUtilTest : public testing::Test {
 public:
  BraveFederatedOperationalPatternsUtilTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(BraveFederatedOperationalPatternsUtilTest,
       GetCollectionSlotAtTheBeginningOfTheMonth) {
  // Arrange
  const base::Time::Exploded exploded_time = {
      /* year */ 2022,
      /* month */ 1,
      /* day_of_week */ 6,
      /* day_of_month */ 1,
      /* hour */ 0,
      /* minute */ 0,
      /* second */ 0,
      /* millisecond */ 0,
  };
  base::Time time;
  ASSERT_TRUE(base::Time::FromLocalExploded(exploded_time, &time));

  const base::TimeDelta time_delta = time - base::Time::Now();
  task_environment_.FastForwardBy(time_delta);

  // Act
  const int collection_slot = GetCollectionSlot();

  // Assert
  const int expected_collection_slot = 0;
  EXPECT_EQ(expected_collection_slot, collection_slot);
}

TEST_F(BraveFederatedOperationalPatternsUtilTest,
       GetCollectionSlotAtTheEndOfTheFirstDayOfTheMonth) {
  // Arrange
  const base::Time::Exploded exploded_time = {
      /* year */ 2022,
      /* month */ 2,
      /* day_of_week */ 2,
      /* day_of_month */ 1,
      /* hour */ 23,
      /* minute */ 59,
      /* second */ 59,
      /* millisecond */ 999,
  };
  base::Time time;
  ASSERT_TRUE(base::Time::FromLocalExploded(exploded_time, &time));

  const base::TimeDelta time_delta = time - base::Time::Now();
  task_environment_.FastForwardBy(time_delta);

  // Act
  const int collection_slot = GetCollectionSlot();

  // Assert
  const int expected_collection_slot = 47;
  EXPECT_EQ(expected_collection_slot, collection_slot);
}

TEST_F(BraveFederatedOperationalPatternsUtilTest,
       GetCollectionSlotAtTheEndOfTheMonth) {
  // Arrange
  const base::Time::Exploded exploded_time = {
      /* year */ 2022,
      /* month */ 1,
      /* day_of_week */ 1,
      /* day_of_month */ 31,
      /* hour */ 23,
      /* minute */ 59,
      /* second */ 59,
      /* millisecond */ 0,
  };
  base::Time time;
  ASSERT_TRUE(base::Time::FromLocalExploded(exploded_time, &time));

  const base::TimeDelta time_delta = time - base::Time::Now();
  task_environment_.FastForwardBy(time_delta);

  // Act
  const int collection_slot = GetCollectionSlot();

  // Assert
  const int expected_collection_slot = 1487;
  EXPECT_EQ(expected_collection_slot, collection_slot);
}

TEST_F(BraveFederatedOperationalPatternsUtilTest,
       GetCollectionSlotsWithSlotSizeOf10Seconds) {
  // Arrange
  std::map<std::string, std::string> feature_parameters;
  feature_parameters["collection_slot_size_in_seconds"] = "10";
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{features::kFederatedLearning, feature_parameters}}, {});

  // Act
  const base::Time::Exploded exploded_time = {
      2022, 1, 2, 4, 8, 16, /* seconds */ 32, 0};
  base::Time time;
  ASSERT_TRUE(base::Time::FromLocalExploded(exploded_time, &time));

  const base::TimeDelta time_delta = time - base::Time::Now();
  task_environment_.FastForwardBy(time_delta);

  const int collection_slot = GetCollectionSlot();

  const int expected_collection_slot = 28899;
  ASSERT_EQ(expected_collection_slot, collection_slot);

  const base::Time::Exploded exploded_time_2 = {
      2022, 1, 2, 4, 8, 16, /* seconds */ 40, 0};
  base::Time time_2;
  ASSERT_TRUE(base::Time::FromLocalExploded(exploded_time_2, &time_2));

  const base::TimeDelta time_delta_2 = time_2 - base::Time::Now();
  task_environment_.FastForwardBy(time_delta_2);

  const int collection_slot_2 = GetCollectionSlot();

  // Assert
  const int expected_collection_slot_2 = 28900;
  EXPECT_EQ(expected_collection_slot_2, collection_slot_2);
}

TEST_F(BraveFederatedOperationalPatternsUtilTest, BuildCollectionPingPayload) {
  // Arrange
  const std::string collection_id = CreateCollectionId();
  const int slot = 42;

  // Act
  const std::string payload = BuildCollectionPingPayload(collection_id, slot);

  // Assert
  const std::string pattern =
      "{\"collection_id\":\"(.{32})\",\"collection_slot\":42,\"platform\":\"(.*"
      ")\",\"wiki-link\":\"https://github.com/brave/brave-browser/wiki/"
      "Operational-Patterns\"}";
  EXPECT_TRUE(RE2::FullMatch(payload, pattern));
}

TEST_F(BraveFederatedOperationalPatternsUtilTest, BuildDeletePingPayload) {
  // Arrange
  const std::string collection_id = CreateCollectionId();

  // Act
  const std::string payload = BuildDeletePingPayload(collection_id);

  // Assert
  const std::string pattern =
      "{\"collection_id\":\"(.{32})\",\"wiki-link\":\"https://github.com/brave/"
      "brave-browser/wiki/Operational-Patterns\"}";
  EXPECT_TRUE(RE2::FullMatch(payload, pattern));
}

}  // namespace brave_federated
