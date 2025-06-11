/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/remote_config_manager.h"

#include <memory>
#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/test/task_environment.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/p3a/metric_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {

constexpr auto kBaseConfigs =
    base::MakeFixedFlatMap<std::string_view, MetricConfig>({
        {"Brave.Uptime.BrowserOpenMinutes",
         MetricConfig{
             .constellation_only = true,
             .append_attributes =
                 MetricAttributesToAppend{MetricAttribute::kDateOfInstall},
         }},
        {"Brave.Shields.UsageStatus",
         MetricConfig{
             .ephemeral = true,
             .nebula = true,
             .disable_country_strip = true,
             .record_activation_date = true,
         }},
        {"Brave.Rewards.WalletBalance",
         MetricConfig{
             .attributes = MetricAttributes{MetricAttribute::kVersion,
                                            MetricAttribute::kChannel},
         }},
    });

constexpr auto kLogTypes =
    base::MakeFixedFlatMap<std::string_view, MetricLogType>({
        {"Brave.Uptime.BrowserOpenMinutes", MetricLogType::kExpress},
        {"Brave.Shields.UsageStatus", MetricLogType::kTypical},
        {"Brave.Rewards.WalletBalance", MetricLogType::kSlow},
        {"Brave.UnmodifiedMetric", MetricLogType::kTypical},
    });

}  // namespace

class P3ARemoteConfigManagerTest : public testing::Test,
                                   public RemoteConfigManager::Delegate {
 public:
  P3ARemoteConfigManagerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  std::optional<MetricLogType> GetLogTypeForHistogram(
      std::string_view histogram_name) const override {
    auto it = kLogTypes.find(histogram_name);
    if (it == kLogTypes.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  const MetricConfig* GetBaseMetricConfig(
      std::string_view histogram_name) const override {
    auto it = kBaseConfigs.find(histogram_name);
    if (it == kBaseConfigs.end()) {
      return nullptr;
    }
    return &it->second;
  }

  void OnRemoteConfigLoaded() override { run_loop_.Quit(); }

  void SetUp() override {
    ASSERT_TRUE(bad_dir_.CreateUniqueTempDir());

    test_data_path_ =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA).AppendASCII("p3a");

    remote_config_manager_ =
        std::make_unique<RemoteConfigManager>(this, nullptr);
  }

  void TearDown() override { remote_config_manager_.reset(); }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::FilePath test_data_path_;
  base::ScopedTempDir bad_dir_;
  std::unique_ptr<RemoteConfigManager> remote_config_manager_;
  base::RunLoop run_loop_;
};

TEST_F(P3ARemoteConfigManagerTest, LoadRemoteConfig) {
  remote_config_manager_->LoadRemoteConfig(test_data_path_);
  run_loop_.Run();

  const auto* uptime_config = remote_config_manager_->GetRemoteMetricConfig(
      "Brave.Uptime.BrowserOpenMinutes");
  ASSERT_TRUE(uptime_config);
  EXPECT_TRUE(uptime_config->ephemeral);
  EXPECT_TRUE(uptime_config->nebula);
  EXPECT_FALSE(uptime_config->constellation_only);
  EXPECT_TRUE(uptime_config->disable_country_strip);
  EXPECT_TRUE(uptime_config->record_activation_date);
  ASSERT_TRUE(uptime_config->attributes.has_value());
  EXPECT_EQ(
      (*uptime_config->attributes)[0].value_or(MetricAttribute::kMaxValue),
      MetricAttribute::kVersion);
  EXPECT_EQ(
      (*uptime_config->attributes)[1].value_or(MetricAttribute::kMaxValue),
      MetricAttribute::kPlatform);
  EXPECT_EQ(
      uptime_config->append_attributes[0].value_or(MetricAttribute::kMaxValue),
      MetricAttribute::kDateOfInstall);
  ASSERT_TRUE(uptime_config->activation_metric_name.has_value());
  EXPECT_EQ(uptime_config->activation_metric_name.value(),
            "Brave.Core.LastUsage");
  ASSERT_TRUE(uptime_config->cadence.has_value());
  EXPECT_EQ(uptime_config->cadence.value(), MetricLogType::kExpress);

  const auto* shields_config = remote_config_manager_->GetRemoteMetricConfig(
      "Brave.Shields.UsageStatus");
  ASSERT_TRUE(shields_config);
  EXPECT_FALSE(shields_config->ephemeral);
  EXPECT_FALSE(shields_config->nebula);
  EXPECT_TRUE(shields_config->constellation_only);
  EXPECT_FALSE(shields_config->disable_country_strip);
  EXPECT_FALSE(shields_config->record_activation_date);
  ASSERT_TRUE(shields_config->cadence.has_value());
  EXPECT_EQ(shields_config->cadence.value(), MetricLogType::kTypical);

  const auto* rewards_config = remote_config_manager_->GetRemoteMetricConfig(
      "Brave.Rewards.WalletBalance");
  ASSERT_TRUE(rewards_config);
  EXPECT_FALSE(rewards_config->ephemeral);
  EXPECT_FALSE(rewards_config->nebula);
  EXPECT_FALSE(rewards_config->constellation_only);
  ASSERT_TRUE(rewards_config->attributes.has_value());
  EXPECT_EQ(
      (*rewards_config->attributes)[0].value_or(MetricAttribute::kMaxValue),
      MetricAttribute::kAnswerIndex);
  EXPECT_EQ(
      (*rewards_config->attributes)[1].value_or(MetricAttribute::kMaxValue),
      MetricAttribute::kChannel);
  EXPECT_EQ(
      (*rewards_config->attributes)[2].value_or(MetricAttribute::kMaxValue),
      MetricAttribute::kDateOfInstall);

  const auto* unmodified_config =
      remote_config_manager_->GetRemoteMetricConfig("Brave.UnmodifiedMetric");
  EXPECT_FALSE(unmodified_config);

  EXPECT_FALSE(
      remote_config_manager_->GetRemoteMetricConfig("Brave.NonExistentMetric"));
}

TEST_F(P3ARemoteConfigManagerTest, InvalidConfig) {
  // Don't set up the test data, which will result in an empty config
  remote_config_manager_->LoadRemoteConfig(bad_dir_.GetPath());
  run_loop_.Run();

  // All calls to GetRemoteMetricConfig should return nullptr
  EXPECT_FALSE(remote_config_manager_->GetRemoteMetricConfig(
      "Brave.Uptime.BrowserOpenMinutes"));
  EXPECT_FALSE(remote_config_manager_->GetRemoteMetricConfig(
      "Brave.Shields.UsageStatus"));
  EXPECT_FALSE(remote_config_manager_->GetRemoteMetricConfig(
      "Brave.Rewards.WalletBalance"));
}

}  // namespace p3a
