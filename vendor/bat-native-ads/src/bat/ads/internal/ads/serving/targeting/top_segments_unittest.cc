/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/targeting/top_segments.h"

#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/ranges/algorithm.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_builder.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/features/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/features/purchase_intent_features.h"
#include "bat/ads/internal/features/text_classification_features.h"
#include "bat/ads/internal/processors/behavioral/bandits/bandit_feedback_info.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_segments.h"
#include "bat/ads/internal/processors/behavioral/purchase_intent/purchase_intent_processor.h"
#include "bat/ads/internal/processors/contextual/text_classification/text_classification_processor.h"
#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource_util.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "bat/ads/internal/segments/segment_alias.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::targeting {

namespace {

constexpr char kTextClassificationFeatureName[] = "TextClassification";
constexpr char kEpsilonGreedyBanditFeatureName[] = "EpsilonGreedyBandit";

struct ModelCombinationsParamInfo final {
  bool epsilon_greedy_bandits_enabled;
  bool purchase_intent_enabled;
  bool text_classification_enabled;
  bool previously_processed;
  size_t number_of_segments;
};

// Expected number of segments for all possible model combinations for both,
// never processed and previously processed state
constexpr ModelCombinationsParamInfo kTests[] = {
    // Never processed
    {false, false, false, false, 0},
    {false, false, true, false, 0},
    {false, true, false, false, 0},
    {false, true, true, false, 0},
    {true, false, false, false, 3},
    {true, false, true, false, 3},
    {true, true, false, false, 3},
    {true, true, true, false, 3},
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

SegmentList GetSegmentList() {
  SegmentList segments;
  base::ranges::transform(GetSegments(), std::back_inserter(segments),
                          [](const base::StringPiece& segment) {
                            return static_cast<std::string>(segment);
                          });
  return segments;
}

void ProcessEpsilonGreedyBandit() {
  const std::vector<processor::BanditFeedbackInfo> feedbacks = {
      {"science", mojom::NotificationAdEventType::kClicked},
      {"science", mojom::NotificationAdEventType::kClicked},
      {"science", mojom::NotificationAdEventType::kClicked},
      {"travel", mojom::NotificationAdEventType::kDismissed},
      {"travel", mojom::NotificationAdEventType::kClicked},
      {"travel", mojom::NotificationAdEventType::kClicked},
      {"technology & computing", mojom::NotificationAdEventType::kDismissed},
      {"technology & computing", mojom::NotificationAdEventType::kDismissed},
      {"technology & computing", mojom::NotificationAdEventType::kClicked}};

  for (const base::StringPiece segment : GetSegments()) {
    processor::EpsilonGreedyBandit::Process(
        {static_cast<std::string>(segment),
         mojom::NotificationAdEventType::kDismissed});
  }

  for (const auto& feedback : feedbacks) {
    processor::EpsilonGreedyBandit::Process(feedback);
  }
}

}  // namespace

class BatAdsTopSegmentsTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ModelCombinationsParamInfo> {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    // We always instantitate processors even if features are disabled
    epsilon_greedy_bandit_processor_ =
        std::make_unique<processor::EpsilonGreedyBandit>();

    purchase_intent_resource_ = std::make_unique<resource::PurchaseIntent>();
    purchase_intent_resource_->Load();
    purchase_intent_processor_ = std::make_unique<processor::PurchaseIntent>(
        purchase_intent_resource_.get());

    text_classification_resource_ =
        std::make_unique<resource::TextClassification>();
    text_classification_resource_->Load();
    task_environment_.RunUntilIdle();
    text_classification_processor_ =
        std::make_unique<processor::TextClassification>(
            text_classification_resource_.get());
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

  std::unique_ptr<processor::EpsilonGreedyBandit>
      epsilon_greedy_bandit_processor_;
  std::unique_ptr<resource::PurchaseIntent> purchase_intent_resource_;
  std::unique_ptr<processor::PurchaseIntent> purchase_intent_processor_;
  std::unique_ptr<resource::TextClassification> text_classification_resource_;
  std::unique_ptr<processor::TextClassification> text_classification_processor_;
};

TEST_P(BatAdsTopSegmentsTest, GetSegments) {
  // Arrange
  resource::SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  const ModelCombinationsParamInfo param(GetParam());
  if (param.previously_processed) {
    ProcessEpsilonGreedyBandit();
    ProcessTextClassification();
    ProcessPurchaseIntent();
  }

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  std::vector<base::test::FeatureRef> disabled_features;

  if (param.epsilon_greedy_bandits_enabled) {
    base::FieldTrialParams params;
    params["epsilon_value"] =
        "0.0";  // Set bandit to always exploit for deterministic execution
    enabled_features.emplace_back(features::kEpsilonGreedyBandit, params);
  } else {
    disabled_features.emplace_back(features::kEpsilonGreedyBandit);
  }

  if (param.purchase_intent_enabled) {
    enabled_features.push_back({features::kPurchaseIntent, {}});
  } else {
    disabled_features.emplace_back(features::kPurchaseIntent);
  }

  if (param.text_classification_enabled) {
    enabled_features.push_back({features::kTextClassification, {}});
  } else {
    disabled_features.emplace_back(features::kTextClassification);
  }

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const UserModelInfo user_model = BuildUserModel();
  const SegmentList segments = GetTopChildSegments(user_model);

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

INSTANTIATE_TEST_SUITE_P(BatAdsTopSegmentsTest,
                         BatAdsTopSegmentsTest,
                         ::testing::ValuesIn(kTests),
                         GetTestCaseName);

TEST_F(BatAdsTopSegmentsTest, GetSegmentsForAllModelsIfPreviouslyProcessed) {
  // Arrange
  resource::SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  ProcessEpsilonGreedyBandit();
  ProcessTextClassification();
  ProcessPurchaseIntent();

  std::map<std::string, std::string> params;
  params["epsilon_value"] =
      "0.0";  // Set bandit to always exploit for deterministic execution

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{features::kPurchaseIntent, /*default params*/ {}},
       {features::kEpsilonGreedyBandit, params},
       {features::kTextClassification, /*default params*/ {}}},
      {});

  // Act
  const UserModelInfo user_model = BuildUserModel();
  const SegmentList segments = GetTopChildSegments(user_model);

  // Assert
  const SegmentList expected_segments = {
      "technology & computing-technology & computing",
      "personal finance-banking",
      "food & drink-cooking",
      "science",
      "travel",
      "technology & computing",
      "segment 3",
      "segment 2"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsTopSegmentsTest, GetSegmentsForFieldTrialParticipationPath) {
  // Arrange
  resource::SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  ProcessEpsilonGreedyBandit();
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
  const UserModelInfo user_model = BuildUserModel();
  const SegmentList segments = GetTopChildSegments(user_model);

  // Assert
  // Even though text classification has been processed we don't expect
  // winning segments from it since the trial disabled the model
  EXPECT_EQ(5U, segments.size());
}

}  // namespace ads::targeting
