/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/top_segments.h"

#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_builder.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feedback_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_processor.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_segments.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/resource/epsilon_greedy_bandit_resource_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_processor.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_processor.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kTextClassificationFeatureName[] = "TextClassification";
constexpr char kEpsilonGreedyBanditFeatureName[] = "EpsilonGreedyBandit";

struct ParamInfo final {
  bool epsilon_greedy_bandits_enabled;
  bool purchase_intent_enabled;
  bool text_classification_enabled;
  bool previously_processed;
  size_t number_of_segments;
} constexpr kTests[] = {
    // Expected number of segments for all possible model combinations for both,
    // never processed and previously processed state.

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

}  // namespace

class BraveAdsTopSegmentsTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    epsilon_greedy_bandit_processor_ =
        std::make_unique<EpsilonGreedyBanditProcessor>();

    purchase_intent_resource_ = std::make_unique<PurchaseIntentResource>();
    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                     kCountryComponentId);
    purchase_intent_processor_ =
        std::make_unique<PurchaseIntentProcessor>(*purchase_intent_resource_);

    text_classification_resource_ =
        std::make_unique<TextClassificationResource>();
    NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion,
                                     kLanguageComponentId);
    text_classification_processor_ =
        std::make_unique<TextClassificationProcessor>(
            *text_classification_resource_);

    NotifyDidInitializeAds();

    task_environment_.RunUntilIdle();
  }

  void ProcessEpsilonGreedyBandit() {
    const std::vector<EpsilonGreedyBanditFeedbackInfo> feedbacks = {
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
      epsilon_greedy_bandit_processor_->Process(
          {static_cast<std::string>(segment),
           mojom::NotificationAdEventType::kDismissed});
    }

    for (const auto& feedback : feedbacks) {
      epsilon_greedy_bandit_processor_->Process(feedback);
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

  std::unique_ptr<EpsilonGreedyBanditProcessor>
      epsilon_greedy_bandit_processor_;
  std::unique_ptr<PurchaseIntentResource> purchase_intent_resource_;
  std::unique_ptr<PurchaseIntentProcessor> purchase_intent_processor_;
  std::unique_ptr<TextClassificationResource> text_classification_resource_;
  std::unique_ptr<TextClassificationProcessor> text_classification_processor_;
};

TEST_P(BraveAdsTopSegmentsTest, GetSegments) {
  // Arrange
  SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  const ParamInfo param(GetParam());
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
    enabled_features.emplace_back(kEpsilonGreedyBanditFeatures, params);
  } else {
    disabled_features.emplace_back(kEpsilonGreedyBanditFeatures);
  }

  if (param.purchase_intent_enabled) {
    enabled_features.push_back({kPurchaseIntentFeature, {}});
  } else {
    disabled_features.emplace_back(kPurchaseIntentFeature);
  }

  if (param.text_classification_enabled) {
    enabled_features.push_back({kTextClassificationFeature, {}});
  } else {
    disabled_features.emplace_back(kTextClassificationFeature);
  }

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  BuildUserModel(base::BindOnce(
      [](const ParamInfo param, const UserModelInfo& user_model) {
        const SegmentList segments = GetTopChildSegments(user_model);

        // Assert
        EXPECT_EQ(param.number_of_segments, segments.size());
      },
      param));
}

static std::string TestParamToString(
    ::testing::TestParamInfo<ParamInfo> test_param) {
  const std::string epsilon_greedy_bandits_enabled =
      test_param.param.epsilon_greedy_bandits_enabled
          ? "EpsilonGreedyBanditEnabledAnd"
          : "";

  const std::string purchase_intent_enabled =
      test_param.param.purchase_intent_enabled ? "PurchaseIntentEnabledAnd"
                                               : "";

  const std::string text_classification_enabled =
      test_param.param.text_classification_enabled
          ? "TextClassificationEnabledAnd"
          : "";

  const std::string previously_processed = test_param.param.previously_processed
                                               ? "PreviouslyProcessed"
                                               : "NeverProcessed";

  return base::ReplaceStringPlaceholders(
      "For$1$2$3$4",
      {epsilon_greedy_bandits_enabled, purchase_intent_enabled,
       text_classification_enabled, previously_processed},
      nullptr);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsTopSegmentsTest,
                         ::testing::ValuesIn(kTests),
                         TestParamToString);

TEST_F(BraveAdsTopSegmentsTest, GetSegmentsForAllModelsIfPreviouslyProcessed) {
  // Arrange
  SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

  ProcessEpsilonGreedyBandit();
  ProcessTextClassification();
  ProcessPurchaseIntent();

  std::map<std::string, std::string> params;
  params["epsilon_value"] =
      "0.0";  // Set bandit to always exploit for deterministic execution

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{kPurchaseIntentFeature, /*default params*/ {}},
       {kEpsilonGreedyBanditFeatures, params},
       {kTextClassificationFeature, /*default params*/ {}}},
      {});

  // Act
  BuildUserModel(base::BindOnce([](const UserModelInfo& user_model) {
    const SegmentList segments = GetTopChildSegments(user_model);

    // Assert
    const SegmentList expected_segments = {
        "segment 3",
        "segment 2",
        "science",
        "travel",
        "technology & computing",
        "technology & computing-technology & computing",
        "personal finance-banking",
        "food & drink-cooking"};

    EXPECT_EQ(expected_segments, segments);
  }));
}

TEST_F(BraveAdsTopSegmentsTest, GetSegmentsForFieldTrialParticipationPath) {
  // Arrange
  SetEpsilonGreedyBanditEligibleSegments(GetSegmentList());

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
  BuildUserModel(base::BindOnce([](const UserModelInfo& user_model) {
    const SegmentList segments = GetTopChildSegments(user_model);

    // Assert
    // Even though text classification has been processed we don't expect
    // winning segments from it since the trial disabled the model
    EXPECT_EQ(5U, segments.size());
  }));
}

}  // namespace brave_ads
