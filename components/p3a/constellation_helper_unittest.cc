// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/constellation_helper.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/json/values_util.h"
#include "base/memory/raw_ptr.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/p3a/metric_config.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/p3a_message.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/p3a/star_randomness_test_util.h"
#include "brave/components/p3a/uploader.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !BUILDFLAG(IS_IOS)
#include "brave/components/brave_referrals/common/pref_names.h"
#endif  // !BUILDFLAG(IS_IOS)

namespace p3a {

namespace {

constexpr uint8_t kTestSlowEpoch = 2;
constexpr uint8_t kTestTypicalEpoch = 5;
constexpr uint8_t kTestExpressEpoch = 7;
constexpr char kTestSlowNextEpochTime[] = "2086-06-22T18:00:00Z";
constexpr char kTestTypicalNextEpochTime[] = "2086-06-24T18:00:00Z";
constexpr char kTestExpressNextEpochTime[] = "2086-07-01T18:00:00Z";
constexpr char kTestHistogramName[] = "Brave.Test.Histogram";
constexpr char kTestActivationHistogramName[] =
    "Brave.Test.ActivationHistogram";
constexpr char kTestNebulaHistogramName[] = "Brave.Core.WeeklyUsage.Nebula";
constexpr char kTestHost[] = "https://localhost:8443";

std::string FormatUTCDateFromTime(const base::Time& time) {
  base::Time::Exploded exploded;
  time.UTCExplode(&exploded);
  return absl::StrFormat("%d-%02d-%02d", exploded.year, exploded.month,
                         exploded.day_of_month);
}

std::vector<std::string> SplitMessageLayers(const std::string& message) {
  return base::SplitString(message, kP3AMessageConstellationLayerSeparator,
                           base::WhitespaceHandling::TRIM_WHITESPACE,
                           base::SplitResult::SPLIT_WANT_NONEMPTY);
}

}  // namespace

class P3AConstellationHelperTest : public testing::Test {
 public:
  P3AConstellationHelperTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(base::Time::FromString("2022-01-01", &install_time_));
    p3a_config_.disable_star_attestation = true;
    p3a_config_.star_randomness_host = kTestHost;

    ConstellationHelper::RegisterPrefs(local_state_.registry());
    local_state_.registry()->RegisterDictionaryPref(
        p3a::kActivationDatesDictPref);

#if !BUILDFLAG(IS_IOS)
    local_state_.registry()->RegisterStringPref(kReferralPromoCode, {});
#endif  // !BUILDFLAG(IS_IOS)

    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();

          MetricLogType log_type =
              ValidateURLAndGetMetricLogType(request.url, kTestHost);

          std::string response;
          if (request.url.spec().ends_with("/info")) {
            response =
                HandleInfoRequest(request, log_type, GetTestEpoch(log_type),
                                  GetTestNextEpochTime(log_type));
            info_request_made_[log_type] = true;
          } else if (request.url.spec().ends_with("/randomness")) {
            response = HandleRandomnessRequest(request, GetTestEpoch(log_type));
            points_request_made_[log_type] = true;
          }
          if (!response.empty()) {
            if (interceptor_send_bad_response_) {
              response = "this is a bad response that is not json";
            }
            url_loader_factory_.AddResponse(request.url.spec(), response);
          }
        }));
  }

  void SetUpHelper() {
    server_info_from_callback_ = nullptr;

    helper_ = std::make_unique<ConstellationHelper>(
        &local_state_, shared_url_loader_factory_,
        base::BindLambdaForTesting(
            [this](std::string histogram_name, MetricLogType log_type,
                   uint8_t epoch, bool is_success,
                   std::unique_ptr<std::string> serialized_message) {
              histogram_name_from_callback_ = histogram_name;
              epoch_from_callback_ = epoch;
              serialized_message_from_callback_ = std::move(serialized_message);
              is_success_from_callback_ = is_success;
            }),
        base::BindLambdaForTesting(
            [this](MetricLogType log_type, RandomnessServerInfo* server_info) {
              server_info_from_callback_ = server_info;
            }),
        &p3a_config_);
    task_environment_.RunUntilIdle();
  }

  uint8_t GetTestEpoch(MetricLogType log_type) {
    switch (log_type) {
      case MetricLogType::kSlow:
        return kTestSlowEpoch;
      case MetricLogType::kTypical:
        return kTestTypicalEpoch;
      case MetricLogType::kExpress:
        return kTestExpressEpoch;
    }
    NOTREACHED();
  }

  const char* GetTestNextEpochTime(MetricLogType log_type) {
    switch (log_type) {
      case MetricLogType::kSlow:
        return kTestSlowNextEpochTime;
      case MetricLogType::kTypical:
        return kTestTypicalNextEpochTime;
      case MetricLogType::kExpress:
        return kTestExpressNextEpochTime;
    }
    NOTREACHED();
  }

  void CheckInfoRequestMade(MetricLogType log_type) {
    for (MetricLogType check_log_type : kAllMetricLogTypes) {
      if (check_log_type == log_type) {
        ASSERT_TRUE(info_request_made_[check_log_type]);
      } else {
        ASSERT_FALSE(info_request_made_[check_log_type]);
      }
    }
  }

  void CheckPointsRequestMade(MetricLogType log_type) {
    for (MetricLogType check_log_type : kAllMetricLogTypes) {
      if (check_log_type == log_type) {
        ASSERT_TRUE(points_request_made_[check_log_type]);
      } else {
        ASSERT_FALSE(points_request_made_[check_log_type]);
      }
    }
  }

  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  P3AConfig p3a_config_;
  std::unique_ptr<ConstellationHelper> helper_;
  TestingPrefServiceSimple local_state_;
  bool interceptor_send_bad_response_ = false;

  raw_ptr<RandomnessServerInfo> server_info_from_callback_ = nullptr;
  std::unique_ptr<std::string> serialized_message_from_callback_;
  bool is_success_from_callback_ = false;

  std::string histogram_name_from_callback_;
  uint8_t epoch_from_callback_;
  base::Time install_time_;

  base::flat_map<MetricLogType, bool> info_request_made_;
  base::flat_map<MetricLogType, bool> points_request_made_;
};

TEST_F(P3AConstellationHelperTest, CanRetrieveServerInfo) {
  SetUpHelper();
  for (MetricLogType log_type : kAllMetricLogTypes) {
    uint8_t test_epoch = GetTestEpoch(log_type);
    base::Time exp_next_epoch_time;

    ASSERT_TRUE(base::Time::FromString(GetTestNextEpochTime(log_type),
                                       &exp_next_epoch_time));

    helper_->UpdateRandomnessServerInfo(log_type);
    task_environment_.RunUntilIdle();

    CheckInfoRequestMade(log_type);

    EXPECT_NE(server_info_from_callback_, nullptr);
    EXPECT_EQ(server_info_from_callback_->current_epoch, test_epoch);

    EXPECT_EQ(server_info_from_callback_->next_epoch_time, exp_next_epoch_time);

    // See if cached server info is used on next execution
    info_request_made_[log_type] = false;
    server_info_from_callback_ = nullptr;

    SetUpHelper();
    helper_->UpdateRandomnessServerInfo(log_type);
    task_environment_.RunUntilIdle();

    ASSERT_FALSE(info_request_made_[log_type]);
    EXPECT_NE(server_info_from_callback_, nullptr);
    EXPECT_EQ(server_info_from_callback_->current_epoch, test_epoch);
    EXPECT_EQ(server_info_from_callback_->next_epoch_time, exp_next_epoch_time);
    server_info_from_callback_ = nullptr;
  }
}

TEST_F(P3AConstellationHelperTest, CannotRetrieveServerInfo) {
  SetUpHelper();
  for (MetricLogType log_type : kAllMetricLogTypes) {
    base::Time exp_next_epoch_time;
    ASSERT_TRUE(base::Time::FromString(GetTestNextEpochTime(log_type),
                                       &exp_next_epoch_time));

    interceptor_send_bad_response_ = true;

    helper_->UpdateRandomnessServerInfo(log_type);
    task_environment_.RunUntilIdle();

    CheckInfoRequestMade(log_type);

    // callback should not be executed if info retrieval failed
    EXPECT_EQ(server_info_from_callback_, nullptr);

    // See if info retrieval retry is scheduled
    info_request_made_[log_type] = false;
    task_environment_.FastForwardBy(base::Seconds(6));

    ASSERT_TRUE(info_request_made_[log_type]);
    EXPECT_EQ(server_info_from_callback_, nullptr);
    info_request_made_[log_type] = false;
  }
}

TEST_F(P3AConstellationHelperTest, GenerateBasicMessage) {
  SetUpHelper();
  for (MetricLogType log_type : kAllMetricLogTypes) {
    uint8_t test_epoch = GetTestEpoch(log_type);

    helper_->UpdateRandomnessServerInfo(log_type);
    task_environment_.RunUntilIdle();

    MessageMetainfo meta_info;
    meta_info.Init(&local_state_, "release", install_time_);

    helper_->StartMessagePreparation(
        kTestHistogramName, log_type,
        GenerateP3AConstellationMessage(kTestHistogramName, test_epoch,
                                        meta_info, kP3AUploadType, nullptr),
        false);
    task_environment_.RunUntilIdle();

    CheckPointsRequestMade(log_type);

    EXPECT_NE(serialized_message_from_callback_, nullptr);
    EXPECT_NE(serialized_message_from_callback_->size(), 0U);

    EXPECT_EQ(histogram_name_from_callback_, kTestHistogramName);
    EXPECT_EQ(epoch_from_callback_, test_epoch);
    points_request_made_[log_type] = false;
  }
}

TEST_F(P3AConstellationHelperTest, IncludeRefcode) {
  MessageMetainfo meta_info;
  meta_info.Init(&local_state_, "release", install_time_);

  std::string message_with_no_refcode = GenerateP3AConstellationMessage(
      kTestHistogramName, 0, meta_info, kP3AUploadType, nullptr);
  std::vector<std::string> no_refcode_layers =
      SplitMessageLayers(message_with_no_refcode);

  EXPECT_EQ(no_refcode_layers.size(), 8U);
  EXPECT_FALSE(no_refcode_layers.at(7).starts_with("ref"));

  MetricConfig refcode_config{.append_attributes = {MetricAttribute::kRef}};
  std::string message_with_refcode = GenerateP3AConstellationMessage(
      kTestHistogramName, 0, meta_info, kP3AUploadType, &refcode_config);
  std::vector<std::string> refcode_layers =
      SplitMessageLayers(message_with_refcode);

  EXPECT_EQ(refcode_layers.size(), 9U);
  EXPECT_EQ(refcode_layers.at(8), "ref|none");

#if !BUILDFLAG(IS_IOS)
  local_state_.SetString(kReferralPromoCode, "BRV003");
  meta_info.Init(&local_state_, "release", install_time_);

  message_with_refcode = GenerateP3AConstellationMessage(
      kTestHistogramName, 0, meta_info, kP3AUploadType, &refcode_config);
  refcode_layers = SplitMessageLayers(message_with_refcode);

  EXPECT_EQ(refcode_layers.size(), 9U);
  EXPECT_EQ(refcode_layers.at(8), "ref|BRV003");

  local_state_.SetString(kReferralPromoCode, "ZRK009");
  meta_info.Init(&local_state_, "release", install_time_);

  message_with_refcode = GenerateP3AConstellationMessage(
      kTestHistogramName, 0, meta_info, kP3AUploadType, &refcode_config);
  refcode_layers = SplitMessageLayers(message_with_refcode);

  EXPECT_EQ(refcode_layers.size(), 9U);
  EXPECT_EQ(refcode_layers.at(8), "ref|other");
#endif  // !BUILDFLAG(IS_IOS)
}

TEST_F(P3AConstellationHelperTest, CustomAttributes) {
  MessageMetainfo meta_info;
  meta_info.Init(&local_state_, "release", install_time_);

  // Test with custom attributes list
  MetricConfig config{.attributes = MetricAttributes{
                          MetricAttribute::kAnswerIndex,
                          MetricAttribute::kVersion,
                          MetricAttribute::kPlatform,
                          MetricAttribute::kGeneralPlatform,
                          MetricAttribute::kChannel,
                          MetricAttribute::kRegion,
                          MetricAttribute::kSubregion,
                      }};

  std::string message = GenerateP3AConstellationMessage(
      kTestHistogramName, 7, meta_info, kP3AUploadType, &config);

  std::vector<std::string> layers = SplitMessageLayers(message);

  // Verify number of layers matches number of non-null attributes
  EXPECT_EQ(layers.size(), 8U);

  // Verify each layer's content
  EXPECT_EQ(layers[0], base::StrCat({"metric_name|", kTestHistogramName}));
  EXPECT_EQ(layers[1], "metric_value|7");
  EXPECT_EQ(layers[2], "version|" + meta_info.version());
  EXPECT_EQ(layers[3], "platform|" + meta_info.platform());
  EXPECT_EQ(layers[4], "general_platform|" + meta_info.general_platform());
  EXPECT_EQ(layers[5], "channel|release");
  EXPECT_EQ(layers[6],
            base::StrCat({"region|", meta_info.region_identifiers().region}));
  EXPECT_EQ(
      layers[7],
      base::StrCat({"subregion|", meta_info.region_identifiers().sub_region}));
}

TEST_F(P3AConstellationHelperTest, NebulaMessage) {
  MessageMetainfo meta_info;
  meta_info.Init(&local_state_, "release", install_time_);

  MetricConfig nebula_config{.nebula = true};
  std::string message = GenerateP3AConstellationMessage(
      kTestHistogramName, 3, meta_info, kP3AUploadType, &nebula_config);
  std::vector<std::string> no_refcode_layers = SplitMessageLayers(message);

  EXPECT_EQ(no_refcode_layers.size(), 7U);
  EXPECT_EQ(no_refcode_layers[0],
            base::StrCat({"metric_name_and_value|", kTestHistogramName, "=3"}));
}

TEST_F(P3AConstellationHelperTest, NebulaSample) {
  size_t points_request_count = 0;
  for (int i = 0; i < 100; i++) {
    SetUpHelper();

    helper_->UpdateRandomnessServerInfo(MetricLogType::kTypical);
    task_environment_.RunUntilIdle();

    MessageMetainfo meta_info;
    meta_info.Init(&local_state_, "release", install_time_);

    helper_->StartMessagePreparation(
        kTestNebulaHistogramName, MetricLogType::kTypical,
        GenerateP3AConstellationMessage(kTestNebulaHistogramName,
                                        kTestTypicalEpoch, meta_info,
                                        kP3AUploadType, nullptr),
        true);
    task_environment_.RunUntilIdle();

    if (points_request_made_[MetricLogType::kTypical]) {
      points_request_made_[MetricLogType::kTypical] = false;
      points_request_count++;
      EXPECT_NE(serialized_message_from_callback_, nullptr);
      EXPECT_NE(serialized_message_from_callback_->size(), 0U);
    } else {
      EXPECT_EQ(serialized_message_from_callback_, nullptr);
    }
    EXPECT_TRUE(is_success_from_callback_);
    EXPECT_EQ(epoch_from_callback_, kTestTypicalEpoch);
    EXPECT_EQ(histogram_name_from_callback_, kTestNebulaHistogramName);
  }
  EXPECT_GE(points_request_count, 1u);
  EXPECT_LE(points_request_count, 25u);
}

TEST_F(P3AConstellationHelperTest, ActivationDateAttributes) {
  // Create metric config with both activation attributes
  MetricConfig config{.attributes = MetricAttributes{
                          MetricAttribute::kAnswerIndex,
                          MetricAttribute::kWeekOfActivation,
                          MetricAttribute::kDateOfActivation,
                      }};

  MessageMetainfo meta_info;
  meta_info.Init(&local_state_, "release", install_time_);

  // Test case 1: No activation date set
  auto message = GenerateP3AConstellationMessage(
      kTestHistogramName, 3, meta_info, kP3AUploadType, &config);

  auto layers = SplitMessageLayers(message);

  // Verify activation attributes show "none" when no activation date is set
  EXPECT_EQ(layers.size(), 4U);
  EXPECT_EQ(layers[2], "woa|none");
  EXPECT_EQ(layers[3], "dtoa|none");

  // Test case 2: Recent activation date (within threshold)
  base::Time activation_time = base::Time::Now() - base::Days(1);
  base::Time activation_week = brave_stats::GetLastMondayTime(activation_time);

  {
    ScopedDictPrefUpdate update(&local_state_, p3a::kActivationDatesDictPref);
    update->Set(kTestHistogramName, base::TimeToValue(activation_time));
  }

  message = GenerateP3AConstellationMessage(kTestHistogramName, 3, meta_info,
                                            kP3AUploadType, &config);

  layers = SplitMessageLayers(message);

  // Verify activation attributes show dates for recent activation
  EXPECT_EQ(layers.size(), 4U);
  EXPECT_EQ(layers[2], "woa|" + FormatUTCDateFromTime(activation_week));
  EXPECT_EQ(layers[3], "dtoa|" + FormatUTCDateFromTime(activation_time));

  activation_time = activation_time - base::Days(14);
  activation_week = brave_stats::GetLastMondayTime(activation_time);
  {
    ScopedDictPrefUpdate update(&local_state_, p3a::kActivationDatesDictPref);
    update->Set(kTestActivationHistogramName,
                base::TimeToValue(activation_time));
  }
  // Test with separate activation histogram name
  config.activation_metric_name = kTestActivationHistogramName;

  message = GenerateP3AConstellationMessage(kTestHistogramName, 3, meta_info,
                                            kP3AUploadType, &config);

  layers = SplitMessageLayers(message);

  // Verify activation attributes show dates for recent activation
  EXPECT_EQ(layers.size(), 4U);
  EXPECT_EQ(layers[2], "woa|" + FormatUTCDateFromTime(activation_week));
  EXPECT_EQ(layers[3], "dtoa|" + FormatUTCDateFromTime(activation_time));

  activation_time = activation_time - base::Days(17);
  {
    ScopedDictPrefUpdate update(&local_state_, p3a::kActivationDatesDictPref);
    update->Set(kTestActivationHistogramName,
                base::TimeToValue(activation_time));
  }

  message = GenerateP3AConstellationMessage(kTestHistogramName, 3, meta_info,
                                            kP3AUploadType, &config);

  layers = SplitMessageLayers(message);

  // Verify activation attributes show "none" for old activation date
  EXPECT_EQ(layers.size(), 4U);
  EXPECT_EQ(layers[2], "woa|none");
  EXPECT_EQ(layers[3], "dtoa|none");
}

}  // namespace p3a
