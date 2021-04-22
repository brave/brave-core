/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ad_targeting/data_types/behavioral/bandits/epsilon_greedy_bandit_segments.h"
#include "bat/ads/internal/ad_targeting/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"
#include "bat/ads/internal/ad_targeting/processors/behavioral/purchase_intent/purchase_intent_processor.h"
#include "bat/ads/internal/ad_targeting/processors/contextual/text_classification/text_classification_processor.h"
#include "bat/ads/internal/features/bandits/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/features/purchase_intent/purchase_intent_features.h"
#include "bat/ads/internal/features/text_classification/text_classification_features.h"
#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {

namespace {

const char kTextClassificationFeatureName[] = "TextClassification";
const char kEpsilonGreedyBanditFeatureName[] = "EpsilonGreedyBandit";

struct ModelCombinationsParamInfo {
  bool epsilon_greedy_bandits_enabled;
  bool purchase_intent_enabled;
  bool text_classification_enabled;
  bool previously_processed;
  size_t number_of_segments;
};

// Expected number of segments for all possible model combinations for both,
// never processed and previously processed state
const ModelCombinationsParamInfo kTests[] = {
    // Never processed
    {false, false, false, false, 0},
    {false, false, true, false, 1},
    {false, true, false, false, 0},
    {false, true, true, false, 1},
    {true, false, false, false, 3},
    {true, false, true, false, 4},
    {true, true, false, false, 3},
    {true, true, true, false, 4},
    // Previously processed
    {false, false, false, true, 0},
    {false, false, true, true, 3},
    {false, true, false, true, 2},
    {false, true, true, true, 5},
    {true, false, false, true, 3},
    {true, false, true, true, 6},
    {true, true, false, true, 5},
    {true, true, true, true, 8},
};

}  // namespace

class BatAdsAdTargetingTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ModelCombinationsParamInfo> {
 protected:
  BatAdsAdTargetingTest() = default;

  ~BatAdsAdTargetingTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    // We always instantitate processors even if features are disabled
    bandit_processor_ = std::make_unique<processor::EpsilonGreedyBandit>();

    purchase_intent_resource_ = std::make_unique<resource::PurchaseIntent>();
    purchase_intent_resource_->Load();
    purchase_intent_processor_ = std::make_unique<processor::PurchaseIntent>(
        purchase_intent_resource_.get());

    text_classification_resource_ =
        std::make_unique<resource::TextClassification>();
    text_classification_resource_->Load();
    text_classification_processor_ =
        std::make_unique<processor::TextClassification>(
            text_classification_resource_.get());
  }

  void SaveSegments(const std::vector<std::string>& segments) {
    const std::string json = SerializeSegments(segments);

    AdsClientHelper::Get()->SetStringPref(
        prefs::kEpsilonGreedyBanditEligibleSegments, json);
  }

  void SaveAllSegments() { SaveSegments(kSegments); }

  void ProcessBandit() {
    const std::vector<processor::BanditFeedbackInfo> feedbacks = {
        {"science", AdNotificationEventType::kClicked},
        {"science", AdNotificationEventType::kClicked},
        {"science", AdNotificationEventType::kClicked},
        {"travel", AdNotificationEventType::kDismissed},
        {"travel", AdNotificationEventType::kClicked},
        {"travel", AdNotificationEventType::kClicked},
        {"technology & computing", AdNotificationEventType::kDismissed},
        {"technology & computing", AdNotificationEventType::kDismissed},
        {"technology & computing", AdNotificationEventType::kClicked}};

    for (const auto& segment : kSegments) {
      bandit_processor_->Process(
          {segment, AdNotificationEventType::kDismissed});
    }

    for (const auto& feedback : feedbacks) {
      bandit_processor_->Process(feedback);
    }
  }

  void ProcessTextClassification() {
    const std::vector<std::string> texts = {
        "Some content about cooking food",
        "Some content about finance & banking",
        "Some content about technology & computing"};

    for (const auto& text : texts) {
      text_classification_processor_->Process(text);
    }
  }

  void ProcessPurchaseIntent() {
    const std::vector<GURL> urls = {
        GURL("https://www.brave.com/test?foo=bar"),
        GURL("https://www.basicattentiontoken.org/test?bar=foo"),
        GURL("https://www.brave.com/test?foo=bar")};

    for (const auto& url : urls) {
      purchase_intent_processor_->Process(url);
    }
  }

  std::unique_ptr<processor::EpsilonGreedyBandit> bandit_processor_;
  std::unique_ptr<resource::PurchaseIntent> purchase_intent_resource_;
  std::unique_ptr<processor::PurchaseIntent> purchase_intent_processor_;
  std::unique_ptr<resource::TextClassification> text_classification_resource_;
  std::unique_ptr<processor::TextClassification> text_classification_processor_;
};

TEST_P(BatAdsAdTargetingTest, GetSegments) {
  // Arrange
  SaveAllSegments();

  ModelCombinationsParamInfo param(GetParam());
  if (param.previously_processed) {
    ProcessBandit();
    ProcessTextClassification();
    ProcessPurchaseIntent();
  }

  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  std::vector<base::Feature> disabled_features;

  if (param.epsilon_greedy_bandits_enabled) {
    const char kEpsilonValue[] = "epsilon_value";
    base::FieldTrialParams kEpsilonGreedyBanditParameters;
    // Set bandit to always exploit for deterministic execution
    kEpsilonGreedyBanditParameters[kEpsilonValue] = "0.0";
    enabled_features.push_back(
        {features::kEpsilonGreedyBandit, kEpsilonGreedyBanditParameters});
  } else {
    disabled_features.push_back(features::kEpsilonGreedyBandit);
  }

  if (param.purchase_intent_enabled) {
    enabled_features.push_back({features::kPurchaseIntent, {}});
  } else {
    disabled_features.push_back(features::kPurchaseIntent);
  }

  if (param.text_classification_enabled) {
    enabled_features.push_back({features::kTextClassification, {}});
  } else {
    disabled_features.push_back(features::kTextClassification);
  }

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  AdTargeting ad_targeting;
  const SegmentList segments = ad_targeting.GetSegments();

  // Assert
  EXPECT_EQ(param.number_of_segments, segments.size());
}

static std::string GetTestCaseName(
    ::testing::TestParamInfo<ModelCombinationsParamInfo> param_info) {
  const std::string epsilon_greedy_bandits_enabled =
      param_info.param.epsilon_greedy_bandits_enabled
          ? "EpsilonGreedyBanditEnabledAnd"
          : "";

  const std::string purchase_intent_enabled =
      param_info.param.purchase_intent_enabled ? "PurchaseIntentEnabledAnd"
                                               : "";

  const std::string text_classification_enabled =
      param_info.param.text_classification_enabled
          ? "TextClassificationEnabledAnd"
          : "";

  const std::string previously_processed = param_info.param.previously_processed
                                               ? "PreviouslyProcessed"
                                               : "NeverProcessed";

  return base::StringPrintf(
      "For%s%s%s%s", epsilon_greedy_bandits_enabled.c_str(),
      purchase_intent_enabled.c_str(), text_classification_enabled.c_str(),
      previously_processed.c_str());
}

INSTANTIATE_TEST_SUITE_P(BatAdsAdTargetingTest,
                         BatAdsAdTargetingTest,
                         ::testing::ValuesIn(kTests),
                         GetTestCaseName);

TEST_F(BatAdsAdTargetingTest, GetSegmentsForAllModelsIfPreviouslyProcessed) {
  // Arrange
  SaveAllSegments();

  ProcessBandit();
  ProcessTextClassification();
  ProcessPurchaseIntent();

  const char kEpsilonValue[] = "epsilon_value";
  std::map<std::string, std::string> kEpsilonGreedyBanditParameters;
  // Set bandit to always exploit for deterministic execution
  kEpsilonGreedyBanditParameters[kEpsilonValue] = "0.0";

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{features::kPurchaseIntent, /* default params */ {}},
       {features::kEpsilonGreedyBandit, kEpsilonGreedyBanditParameters},
       {features::kTextClassification, /* default params */ {}}},
      {});

  // Act
  AdTargeting ad_targeting;
  const SegmentList segments = ad_targeting.GetSegments();

  // Assert
  const SegmentList expected_segments = {
      "technology & computing-technology & computing",
      "personal finance-banking",
      "food & drink-cooking",
      "segment 3",
      "segment 2",
      "science",
      "travel",
      "technology & computing"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsAdTargetingTest, GetSegmentsForFieldTrialParticipationPath) {
  // Arrange
  SaveAllSegments();

  ProcessBandit();
  ProcessTextClassification();
  ProcessPurchaseIntent();

  auto feature_list = std::make_unique<base::FeatureList>();
  feature_list->RegisterFieldTrialOverride(
      kEpsilonGreedyBanditFeatureName,
      base::FeatureList::OVERRIDE_ENABLE_FEATURE,
      base::FieldTrialList::CreateFieldTrial("EpsilonGreedyBanditStudy", "A"));
  feature_list->RegisterFieldTrialOverride(
      kTextClassificationFeatureName,
      base::FeatureList::OVERRIDE_DISABLE_FEATURE,
      base::FieldTrialList::CreateFieldTrial("EpsilonGreedyBanditStudy", "A"));

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatureList(std::move(feature_list));

  // Act
  AdTargeting ad_targeting;
  const SegmentList segments = ad_targeting.GetSegments();

  // Assert
  // Even though text classification has been processed we don't expect
  // winning segments from it since the trial disabled the model
  EXPECT_EQ(5U, segments.size());
}

}  // namespace ad_targeting
}  // namespace ads
