// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/metric_log_store.h"

#include <memory>
#include <set>
#include <string_view>

#include "base/strings/string_number_conversions.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/metric_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

class P3AMetricLogStoreTest : public testing::Test,
                              public MetricLogStore::Delegate {
 public:
  P3AMetricLogStoreTest() {}
  ~P3AMetricLogStoreTest() override {}

  std::string SerializeLog(std::string_view histogram_name,
                           uint64_t value,
                           MetricLogType log_type,
                           bool is_constellation,
                           const std::string& upload_type) override {
    return std::string(histogram_name) + "_" + base::NumberToString(value) +
           "_" + base::NumberToString(is_constellation) + "_" + upload_type;
  }

  bool IsActualMetric(const std::string& histogram_name) const override {
    return p3a::kCollectedTypicalHistograms.contains(histogram_name);
  }

  bool IsEphemeralMetric(const std::string& histogram_name) const override {
    return false;
  }

 protected:
  void SetUp() override {
    MetricLogStore::RegisterPrefs(local_state.registry());
    SetUpLogStore();
  }

  void SetUpLogStore() {
    log_store = std::make_unique<MetricLogStore>(*this, local_state, false,
                                                 MetricLogType::kTypical);
  }

  void UpdateSomeValues(size_t message_count) {
    auto histogram_it = p3a::kCollectedTypicalHistograms.begin();
    for (size_t i = 1; i <= message_count &&
                       histogram_it != p3a::kCollectedTypicalHistograms.end();
         i++) {
      log_store->UpdateValue(std::string(histogram_it->first), 2);
      histogram_it++;
    }
  }

  void ConsumeMessages(size_t message_count) {
    std::set<std::string> consumed_log_set;

    ASSERT_TRUE(log_store->has_unsent_logs());
    ASSERT_FALSE(log_store->has_staged_log());
    for (uint64_t i = 1; i <= message_count; i++) {
      log_store->StageNextLog();
      ASSERT_TRUE(log_store->has_staged_log());

      std::string combined_log =
          log_store->staged_log_key() + log_store->staged_log();
      ASSERT_EQ(consumed_log_set.find(combined_log), consumed_log_set.end());
      consumed_log_set.insert(combined_log);

      log_store->DiscardStagedLog();
      ASSERT_FALSE(log_store->has_staged_log());
    }
    ASSERT_FALSE(log_store->has_unsent_logs());
    ASSERT_FALSE(log_store->has_staged_log());
  }

  std::unique_ptr<MetricLogStore> log_store;
  TestingPrefServiceSimple local_state;
};

TEST_F(P3AMetricLogStoreTest, GetAllLogs) {
  UpdateSomeValues(9);
  ConsumeMessages(9);
}

TEST_F(P3AMetricLogStoreTest, GetAllLogsAfterReload) {
  UpdateSomeValues(15);

  SetUpLogStore();
  log_store->LoadPersistedUnsentLogs();

  ConsumeMessages(15);
}

TEST_F(P3AMetricLogStoreTest, GetAllLogsAfterTimeReset) {
  UpdateSomeValues(15);
  ConsumeMessages(15);

  log_store->ResetUploadStamps();
  ConsumeMessages(15);
}

TEST_F(P3AMetricLogStoreTest, ShouldNotLoadUnknownMetric) {
  log_store->UpdateValue("Brave.UnknownMetric", 3);

  SetUpLogStore();
  log_store->LoadPersistedUnsentLogs();

  ASSERT_FALSE(log_store->has_unsent_logs());
}

}  // namespace p3a
