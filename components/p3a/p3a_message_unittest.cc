// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/p3a_message.h"

#include "base/time/time.h"
#include "brave/components/p3a/metric_config.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/p3a/uploader.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !BUILDFLAG(IS_IOS)
#include "brave/components/brave_referrals/common/pref_names.h"
#endif  // !BUILDFLAG(IS_IOS)

namespace p3a {

namespace {

constexpr char kTestMetricName[] = "Brave.Test.Message";
constexpr uint64_t kTestMetricValue = 3;

}  // namespace

class P3AMessageTest : public testing::Test {
 protected:
  void SetUp() override {
    local_state_.registry()->RegisterDictionaryPref(kActivationDatesDictPref);
    local_state_.registry()->RegisterDictionaryPref(kCustomAttributesDictPref);

#if !BUILDFLAG(IS_IOS)
    local_state_.registry()->RegisterStringPref(kReferralPromoCode, {});
#endif

    base::Time install_time;
    ASSERT_TRUE(base::Time::FromString("2020-01-01", &install_time));
    meta_.Init(&local_state_, "release", install_time);
  }

  TestingPrefServiceSimple local_state_;
  MessageMetainfo meta_;
};

// Verifies GenerateP3AConstellationMessage serializes metric name, value, and
// channel correctly using fully deterministic attributes.
TEST_F(P3AMessageTest, Basic) {
  constexpr MetricConfig kConfig = {
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex,
                                     MetricAttribute::kChannel,
                                     MetricAttribute::kDateOfInstall},
  };

  std::string message = GenerateP3AConstellationMessage(
      kTestMetricName, kTestMetricValue, meta_, kP3AUploadType, &kConfig);

  EXPECT_EQ(message,
            "metric_name|Brave.Test.Message"
            ";metric_value|3"
            ";channel|release"
            ";dtoi|none");

  constexpr MetricConfig kNullAttrConfig = {
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex,
                                     std::nullopt, MetricAttribute::kChannel,
                                     MetricAttribute::kDateOfInstall},
  };

  std::string message_null_attr =
      GenerateP3AConstellationMessage(kTestMetricName, kTestMetricValue, meta_,
                                      kP3AUploadType, &kNullAttrConfig);

  EXPECT_EQ(message_null_attr,
            "metric_name|Brave.Test.Message"
            ";metric_value|3"
            ";channel|release"
            ";dtoi|none");
}

// Verifies that custom attributes are included with the "custom_" prefix, and
// that missing attributes are not included in the message.
TEST_F(P3AMessageTest, CustomAttributes) {
  {
    ScopedDictPrefUpdate update(&local_state_, kCustomAttributesDictPref);
    update->Set("color", "blue");
    update->Set("size", "large");
  }

  constexpr MetricConfig kConfig = {
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex,
                                     MetricAttribute::kCustomAttribute,
                                     MetricAttribute::kCustomAttribute,
                                     MetricAttribute::kDateOfInstall},
      .custom_attributes = CustomAttributes{"color", "size"},
  };

  std::string message = GenerateP3AConstellationMessage(
      kTestMetricName, kTestMetricValue, meta_, kP3AUploadType, &kConfig);

  EXPECT_EQ(message,
            "metric_name|Brave.Test.Message"
            ";metric_value|3"
            ";custom_color|blue"
            ";custom_size|large"
            ";dtoi|none");

  constexpr MetricConfig kMissingFirstConfig = {
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex,
                                     MetricAttribute::kCustomAttribute,
                                     MetricAttribute::kCustomAttribute,
                                     MetricAttribute::kDateOfInstall},
      .custom_attributes = CustomAttributes{"nonexistent", "color"},
  };

  std::string message_missing_first =
      GenerateP3AConstellationMessage(kTestMetricName, kTestMetricValue, meta_,
                                      kP3AUploadType, &kMissingFirstConfig);

  EXPECT_EQ(message_missing_first,
            "metric_name|Brave.Test.Message"
            ";metric_value|3"
            ";custom_color|blue"
            ";dtoi|none");

  constexpr MetricConfig kAllMissingConfig = {
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex,
                                     MetricAttribute::kCustomAttribute,
                                     MetricAttribute::kDateOfInstall},
      .custom_attributes = CustomAttributes{"nonexistent"},
  };

  std::string message_all_missing =
      GenerateP3AConstellationMessage(kTestMetricName, kTestMetricValue, meta_,
                                      kP3AUploadType, &kAllMissingConfig);

  EXPECT_EQ(message_all_missing,
            "metric_name|Brave.Test.Message"
            ";metric_value|3"
            ";dtoi|none");
}

}  // namespace p3a
