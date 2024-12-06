/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/features.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests -filter=BraveFederatedLearningFeaturesTest*

namespace brave_federated {

TEST(BraveFederatedLearningFeaturesTest, FederatedLearningEnabled) {
  // Arrange

  // Act
  const bool is_enabled = features::IsFederatedLearningEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
}

TEST(BraveFederatedLearningFeaturesTest, DefaultOperationalPatternsEnabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool operational_patterns_enabled =
      features::IsOperationalPatternsEnabled();

  // Assert
  const bool expected_operational_patterns_enabled = false;
  EXPECT_EQ(expected_operational_patterns_enabled,
            operational_patterns_enabled);
}

TEST(BraveFederatedLearningFeaturesTest, OperationalPatternsEnabled) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams kFederatedLearningParameters;
  static constexpr char kFieldTrialParameterOperationalPatternsEnabled[] =
      "operational_patterns_enabled";
  kFederatedLearningParameters[kFieldTrialParameterOperationalPatternsEnabled] =
      "true";
  enabled_features.emplace_back(features::kFederatedLearning,
                                kFederatedLearningParameters);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool operational_patterns_enabled =
      features::IsOperationalPatternsEnabled();

  // Assert
  const bool expected_operational_patterns_enabled = true;
  EXPECT_EQ(expected_operational_patterns_enabled,
            operational_patterns_enabled);
}

TEST(BraveFederatedLearningFeaturesTest, DefaultCollectionSlotSizeInSeconds) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int collection_slot_size = features::GetCollectionSlotSizeInSeconds();

  // Assert
  const int expected_collection_slot_size = 30 * base::Time::kSecondsPerMinute;
  EXPECT_EQ(expected_collection_slot_size, collection_slot_size);
}

TEST(BraveFederatedLearningFeaturesTest, CollectionSizeInSeconds) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams kFederatedLearningParameters;
  static constexpr char kFieldTrialParameterCollectionSlotSizeInSeconds[] =
      "collection_slot_size_in_seconds";
  kFederatedLearningParameters
      [kFieldTrialParameterCollectionSlotSizeInSeconds] = "420";
  enabled_features.emplace_back(features::kFederatedLearning,
                                kFederatedLearningParameters);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int collection_slot_size = features::GetCollectionSlotSizeInSeconds();

  // Assert
  const int expected_collection_slot_size = 420;
  EXPECT_EQ(expected_collection_slot_size, collection_slot_size);
}

TEST(BraveFederatedLearningFeaturesTest, DefaultMockTaskDuration) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int mock_task_duration = features::GetMockTaskDurationInSeconds();

  // Assert
  const int expected_mock_task_duration = 2 * base::Time::kSecondsPerMinute;
  EXPECT_EQ(expected_mock_task_duration, mock_task_duration);
}

TEST(BraveFederatedLearningFeaturesTest, MockTaskDuration) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams kFederatedLearningParameters;
  static constexpr char kFieldTrialParameterMockTaskDurationInSeconds[] =
      "mock_task_duration_in_seconds";
  kFederatedLearningParameters[kFieldTrialParameterMockTaskDurationInSeconds] =
      "600";
  enabled_features.emplace_back(features::kFederatedLearning,
                                kFederatedLearningParameters);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int mock_task_duration = features::GetMockTaskDurationInSeconds();

  // Assert
  const int expected_mock_task_duration = 10 * base::Time::kSecondsPerMinute;
  EXPECT_EQ(expected_mock_task_duration, mock_task_duration);
}

TEST(BraveFederatedLearningFeaturesTest, DefaultCollectionIdLifetimeInSeconds) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int collection_id_lifetime =
      features::GetCollectionIdLifetimeInSeconds();

  // Assert
  const int expected_collection_id_lifetime = 1 * base::Time::kHoursPerDay *
                                              base::Time::kMinutesPerHour *
                                              base::Time::kSecondsPerMinute;
  EXPECT_EQ(expected_collection_id_lifetime, collection_id_lifetime);
}

TEST(BraveFederatedLearningFeaturesTest, CollectionIdLifetimeInSeconds) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams kFederatedLearningParameters;
  static constexpr char kFieldTrialParameterCollectionIdLifetimeInSeconds[] =
      "collection_id_lifetime_in_seconds";
  kFederatedLearningParameters
      [kFieldTrialParameterCollectionIdLifetimeInSeconds] = "2592000";
  enabled_features.emplace_back(features::kFederatedLearning,
                                kFederatedLearningParameters);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int collection_id_lifetime =
      features::GetCollectionIdLifetimeInSeconds();

  // Assert
  const int expected_collection_id_lifetime = 30 * base::Time::kHoursPerDay *
                                              base::Time::kMinutesPerHour *
                                              base::Time::kSecondsPerMinute;
  EXPECT_EQ(expected_collection_id_lifetime, collection_id_lifetime);
}

TEST(BraveFederatedLearningFeaturesTest, DefaultMockCollectionRequests) {
  // Arrange

  // Act
  const bool mock_collection_requests = features::MockCollectionRequests();

  // Assert
  EXPECT_FALSE(mock_collection_requests);
}

}  // namespace brave_federated
