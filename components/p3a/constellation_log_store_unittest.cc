// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/constellation_log_store.h"

#include <memory>
#include <set>

#include "base/strings/string_number_conversions.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {

constexpr size_t kTestKeepEpochCount = 4;

}  // namespace

class P3AConstellationLogStoreTest : public testing::Test {
 public:
  P3AConstellationLogStoreTest() {}

 protected:
  void SetUp() override {
    ConstellationLogStore::RegisterPrefs(local_state.registry());
    log_store = std::make_unique<ConstellationLogStore>(local_state,
                                                        kTestKeepEpochCount);
  }

  std::string GenerateMockConstellationMessage() {
    return "log msg " +
           base::NumberToString(curr_test_constellation_message_id++);
  }

  void UpdateSomeMessages(uint8_t epoch, size_t message_count) {
    log_store->UpdateMessage("Brave.Test.Metric1", epoch,
                             "should be overwritten");
    for (uint64_t i = 1; i <= message_count; i++) {
      std::string histogram_name =
          "Brave.Test.Metric" + base::NumberToString(i);
      std::string content = GenerateMockConstellationMessage();
      log_store->UpdateMessage(histogram_name, epoch, content);
    }
  }

  void ConsumeMessages(size_t message_count) {
    std::set<std::string> consumed_log_set;

    ASSERT_TRUE(log_store->has_unsent_logs());
    ASSERT_FALSE(log_store->has_staged_log());
    for (uint64_t i = 1; i <= message_count; i++) {
      log_store->StageNextLog();
      ASSERT_TRUE(log_store->has_staged_log());

      ASSERT_EQ(consumed_log_set.find(log_store->staged_log()),
                consumed_log_set.end());
      consumed_log_set.insert(log_store->staged_log());

      log_store->MarkStagedLogAsSent();
      log_store->DiscardStagedLog();
      ASSERT_FALSE(log_store->has_staged_log());
    }
    ASSERT_FALSE(log_store->has_unsent_logs());
    ASSERT_FALSE(log_store->has_staged_log());
  }

  size_t curr_test_constellation_message_id;
  std::unique_ptr<ConstellationLogStore> log_store;
  TestingPrefServiceSimple local_state;
};

TEST_F(P3AConstellationLogStoreTest, CurrentEpochStaging) {
  log_store->SetCurrentEpoch(1);

  UpdateSomeMessages(1, 8);
  ConsumeMessages(8);
}

TEST_F(P3AConstellationLogStoreTest, PreviousEpochStaging) {
  log_store->SetCurrentEpoch(1);

  UpdateSomeMessages(1, 5);
  log_store->SetCurrentEpoch(2);
  log_store->LoadPersistedUnsentLogs();

  // Should consume messages from first epoch
  ConsumeMessages(5);
}

TEST_F(P3AConstellationLogStoreTest, PreviousEpochsStaging) {
  log_store->SetCurrentEpoch(1);
  UpdateSomeMessages(1, 5);

  log_store->SetCurrentEpoch(2);
  UpdateSomeMessages(2, 7);

  log_store->SetCurrentEpoch(3);
  UpdateSomeMessages(3, 2);

  log_store->SetCurrentEpoch(4);
  log_store->LoadPersistedUnsentLogs();
  // The following 10 messages should not be staged because
  // they are not part of the previous epochs
  UpdateSomeMessages(4, 10);

  // Should consume messages from first three epochs
  ConsumeMessages(24);
}

TEST_F(P3AConstellationLogStoreTest, UpdatePreviousEpochMessage) {
  log_store->SetCurrentEpoch(1);

  log_store->SetCurrentEpoch(2);
  log_store->LoadPersistedUnsentLogs();

  UpdateSomeMessages(1, 3);

  ConsumeMessages(3);
}

TEST_F(P3AConstellationLogStoreTest, DiscardShouldNotDelete) {
  log_store->SetCurrentEpoch(1);

  UpdateSomeMessages(1, 1);

  log_store->SetCurrentEpoch(2);
  log_store->LoadPersistedUnsentLogs();

  log_store->StageNextLog();
  ASSERT_TRUE(log_store->has_staged_log());

  log_store->DiscardStagedLog();
  ASSERT_FALSE(log_store->has_staged_log());
  ASSERT_TRUE(log_store->has_unsent_logs());

  log_store->StageNextLog();
  ASSERT_TRUE(log_store->has_staged_log());

  log_store->MarkStagedLogAsSent();
  log_store->DiscardStagedLog();
  ASSERT_FALSE(log_store->has_staged_log());
  ASSERT_FALSE(log_store->has_unsent_logs());
}

TEST_F(P3AConstellationLogStoreTest, ShouldDeleteOldMessages) {
  log_store->SetCurrentEpoch(1);

  UpdateSomeMessages(1, 3);

  log_store->SetCurrentEpoch(kTestKeepEpochCount + 1);
  UpdateSomeMessages(kTestKeepEpochCount + 1, 8);

  log_store->SetCurrentEpoch(kTestKeepEpochCount + 2);
  log_store->LoadPersistedUnsentLogs();

  // Should only consume messages from the latest previous epoch
}

}  // namespace p3a
