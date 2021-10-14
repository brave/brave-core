/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/weekly_storage/weekly_event_storage.h"

#include <memory>
#include <utility>

#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
constexpr char kPrefName[] = "brave.weekly_event_test";
}

class WeeklyEventStorageTest : public ::testing::Test {
 public:
  enum class TestValues {
    kNull,
    kFoo,
    kBar,
    kBrave,
    kMaxValue = kBrave,
  };

  WeeklyEventStorageTest() : clock_(new base::SimpleTestClock) {
    pref_service_.registry()->RegisterListPref(kPrefName);

    state_ = std::make_unique<WeeklyEventStorage<TestValues>>(
        &pref_service_, kPrefName, std::unique_ptr<base::Clock>(clock_));
    clock_->SetNow(base::Time::Now());
  }

 protected:
  base::SimpleTestClock* clock_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<WeeklyEventStorage<TestValues>> state_;
};

TEST_F(WeeklyEventStorageTest, StartsEmpty) {
  EXPECT_FALSE(state_->HasEvent());
  EXPECT_EQ(state_->GetLatest(), absl::nullopt);
}

TEST_F(WeeklyEventStorageTest, AddEvents) {
  state_->Add(TestValues::kNull);
  EXPECT_TRUE(state_->HasEvent());
  EXPECT_EQ(state_->GetLatest(), absl::optional<TestValues>(TestValues::kNull));

  state_->Add(TestValues::kBrave);
  EXPECT_EQ(state_->GetLatest(),
            absl::optional<TestValues>(TestValues::kBrave));
}

TEST_F(WeeklyEventStorageTest, ForgetsOldEvents) {
  // Add an initial event.
  state_->Add(TestValues::kFoo);
  EXPECT_EQ(state_->GetLatest(), absl::optional<TestValues>(TestValues::kFoo));

  // Jump to the next week.
  clock_->Advance(base::TimeDelta::FromDays(8));
  // Should have forgotten about older days.
  EXPECT_FALSE(state_->HasEvent());

  // Newer events should still accumulate.
  state_->Add(TestValues::kNull);
  state_->Add(TestValues::kBar);
  EXPECT_EQ(state_->GetLatest(), absl::optional<TestValues>(TestValues::kBar));
}

TEST_F(WeeklyEventStorageTest, IntermittentUsage) {
  auto value = TestValues::kFoo;
  for (int day = 0; day < 10; day++) {
    clock_->Advance(base::TimeDelta::FromDays(day % 3));
    state_->Add(value);
  }
  EXPECT_EQ(state_->GetLatest(), absl::optional<TestValues>(value));
}

TEST_F(WeeklyEventStorageTest, InfrequentUsage) {
  state_->Add(TestValues::kFoo);
  clock_->Advance(base::TimeDelta::FromDays(6));
  state_->Add(TestValues::kBar);
  EXPECT_EQ(state_->GetLatest(), absl::optional<TestValues>(TestValues::kBar));
  clock_->Advance(base::TimeDelta::FromDays(10));
  EXPECT_EQ(state_->GetLatest(), absl::nullopt);
}

// Verify serialization order across reloads, since GetLatest
// relies on this.
// FIXME(rillian): Disabled because it crashes as written.
TEST_F(WeeklyEventStorageTest, DISABLED_SerializationOrder) {
  // Add a series of events.
  state_->Add(TestValues::kFoo);
  state_->Add(TestValues::kBar);
  clock_->Advance(base::TimeDelta::FromDays(1));
  state_->Add(TestValues::kFoo);
  state_->Add(TestValues::kBrave);
  clock_->Advance(base::TimeDelta::FromDays(1));
  EXPECT_EQ(state_->GetLatest(),
            absl::optional<TestValues>(TestValues::kBrave));

  // Drop the WeeklyEventStorage object.
  state_.reset();

  // Create a new one.
  state_ = std::make_unique<WeeklyEventStorage<TestValues>>(
      &pref_service_, kPrefName, std::unique_ptr<base::Clock>(clock_));

  // Most recently added event should still be the latest.
  EXPECT_EQ(state_->GetLatest(),
            absl::optional<TestValues>(TestValues::kBrave));
}
