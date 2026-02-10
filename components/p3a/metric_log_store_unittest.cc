// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/metric_log_store.h"

#include <memory>
#include <string_view>

#include "base/containers/flat_set.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/metric_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

constexpr char kTestExpressMetric[] = "Brave.Test.ExpressMetric";
constexpr char kTestUnknownMetric[] = "Brave.Test.UnknownMetric";
constexpr char kTestDeferredMetric[] = "Brave.Test.DeferredMetric";

class P3AMetricLogStoreTest : public testing::Test,
                              public MetricLogStore::Delegate {
 public:
  P3AMetricLogStoreTest() {}
  ~P3AMetricLogStoreTest() override {}

  std::string SerializeLog(std::string_view histogram_name,
                           uint64_t value,
                           MetricLogType log_type,
                           const std::string& upload_type) override {
    return std::string(histogram_name) + "_" + base::NumberToString(value) +
           "_" + upload_type;
  }

  std::optional<MetricLogType> GetLogTypeForHistogram(
      std::string_view histogram_name) const override {
    if (histogram_name == kTestExpressMetric) {
      return MetricLogType::kExpress;
    }
    if (histogram_name == kTestDeferredMetric) {
      return MetricLogType::kTypical;
    }
    return p3a::kCollectedTypicalHistograms.contains(histogram_name)
               ? std::make_optional(MetricLogType::kTypical)
               : std::nullopt;
  }

  bool IsEphemeralMetric(const std::string& histogram_name) const override {
    return false;
  }

  bool ShouldDeferMetric(const std::string& histogram_name) const override {
    return defer_metrics_.contains(histogram_name);
  }

 protected:
  void SetUp() override {
    MetricLogStore::RegisterPrefs(local_state.registry());
    SetUpLogStore();
  }

  void SetUpLogStore() {
    log_store = std::make_unique<MetricLogStore>(*this, local_state,
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
  base::flat_set<std::string> defer_metrics_;
};

TEST_F(P3AMetricLogStoreTest, GetAllLogs) {
  UpdateSomeValues(9);
  ConsumeMessages(9);
}

TEST_F(P3AMetricLogStoreTest, GetAllLogsAfterReload) {
  UpdateSomeValues(15);

  SetUpLogStore();
  log_store->LoadPersistedUnsentLogs();
  log_store->RemoveObsoleteLogs();

  ConsumeMessages(15);
}

TEST_F(P3AMetricLogStoreTest, GetAllLogsAfterTimeReset) {
  UpdateSomeValues(15);
  ConsumeMessages(15);

  log_store->ResetUploadStamps();
  ConsumeMessages(15);
}

TEST_F(P3AMetricLogStoreTest, ShouldNotLoadUnknownMetric) {
  log_store->UpdateValue(kTestUnknownMetric, 3);
  log_store->UpdateValue(kTestExpressMetric, 4);

  ASSERT_TRUE(log_store->has_unsent_logs());

  SetUpLogStore();
  log_store->LoadPersistedUnsentLogs();
  log_store->RemoveObsoleteLogs();

  ASSERT_FALSE(log_store->has_unsent_logs());
}

TEST_F(P3AMetricLogStoreTest, DeferredMetricNotUnsent) {
  defer_metrics_.insert(kTestDeferredMetric);

  log_store->UpdateValue(kTestDeferredMetric, 5);
  ASSERT_FALSE(log_store->has_unsent_logs());
}

TEST_F(P3AMetricLogStoreTest, DeferredMetricMovedAfterReevaluation) {
  defer_metrics_.insert(kTestDeferredMetric);

  log_store->UpdateValue(kTestDeferredMetric, 5);
  ASSERT_FALSE(log_store->has_unsent_logs());

  defer_metrics_.erase(kTestDeferredMetric);
  log_store->ReevaluateDeferredEntries();
  ASSERT_TRUE(log_store->has_unsent_logs());

  log_store->StageNextLog();
  EXPECT_EQ(log_store->staged_log_key(), kTestDeferredMetric);
  log_store->DiscardStagedLog();
  ASSERT_FALSE(log_store->has_unsent_logs());
}

TEST_F(P3AMetricLogStoreTest, DeferredMetricDoesNotBlockNormalMetric) {
  defer_metrics_.insert(kTestDeferredMetric);

  auto first_typical =
      std::string(p3a::kCollectedTypicalHistograms.begin()->first);
  log_store->UpdateValue(first_typical, 1);
  log_store->UpdateValue(kTestDeferredMetric, 5);

  ASSERT_TRUE(log_store->has_unsent_logs());

  log_store->StageNextLog();
  EXPECT_EQ(log_store->staged_log_key(), first_typical);
  log_store->DiscardStagedLog();

  ASSERT_FALSE(log_store->has_unsent_logs());

  defer_metrics_.erase(kTestDeferredMetric);
  log_store->ReevaluateDeferredEntries();
  ASSERT_TRUE(log_store->has_unsent_logs());

  log_store->StageNextLog();
  EXPECT_EQ(log_store->staged_log_key(), kTestDeferredMetric);
  log_store->DiscardStagedLog();

  ASSERT_FALSE(log_store->has_unsent_logs());
}

TEST_F(P3AMetricLogStoreTest, DeferredMetricPersistedAndReloaded) {
  defer_metrics_.insert(kTestDeferredMetric);

  log_store->UpdateValue(kTestDeferredMetric, 5);
  ASSERT_FALSE(log_store->has_unsent_logs());

  SetUpLogStore();
  log_store->LoadPersistedUnsentLogs();
  ASSERT_FALSE(log_store->has_unsent_logs());

  defer_metrics_.erase(kTestDeferredMetric);
  log_store->ReevaluateDeferredEntries();
  ASSERT_TRUE(log_store->has_unsent_logs());
}

}  // namespace p3a
