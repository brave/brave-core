/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/features.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
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
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

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
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kFederatedLearningParameters;
  const char kFieldTrialParameterOperationalPatternsEnabled[] =
      "operational_patterns_enabled";
  kFederatedLearningParameters[kFieldTrialParameterOperationalPatternsEnabled] =
      "true";
  enabled_features.push_back(
      {features::kFederatedLearning, kFederatedLearningParameters});

  const std::vector<base::Feature> disabled_features;

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

TEST(BraveFederatedLearningFeaturesTest, DefaultCollectionSlotSizeInMinutes) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int collection_slot_size = features::GetCollectionSlotSizeValue();

  // Assert
  const int expected_collection_slot_size = 30;
  EXPECT_EQ(expected_collection_slot_size, collection_slot_size);
}

TEST(BraveFederatedLearningFeaturesTest, CollectionSizeInMinutes) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kFederatedLearningParameters;
  const char kFieldTrialParameterCollectionSlotSizeInMinutes[] =
      "collection_slot_size_in_minutes";
  kFederatedLearningParameters
      [kFieldTrialParameterCollectionSlotSizeInMinutes] = "15";
  enabled_features.push_back(
      {features::kFederatedLearning, kFederatedLearningParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int collection_slot_size = features::GetCollectionSlotSizeValue();

  // Assert
  const int expected_collection_slot_size = 15;
  EXPECT_EQ(expected_collection_slot_size, collection_slot_size);
}

TEST(BraveFederatedLearningFeaturesTest,
     DefaultSimulateLocalTrainingStepDuration) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int local_training_step_duration =
      features::GetSimulateLocalTrainingStepDurationValue();

  // Assert
  const int expected_local_training_step_duration = 5;
  EXPECT_EQ(expected_local_training_step_duration,
            local_training_step_duration);
}

TEST(BraveFederatedLearningFeaturesTest, SimulateLocalTrainingStepDuration) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kFederatedLearningParameters;
  const char kFieldTrialParameterSimulateLocalTrainingStepDurationInMinutes[] =
      "simulate_local_training_step_duration_in_minutes";
  kFederatedLearningParameters
      [kFieldTrialParameterSimulateLocalTrainingStepDurationInMinutes] = "10";
  enabled_features.push_back(
      {features::kFederatedLearning, kFederatedLearningParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int local_training_step_duration =
      features::GetSimulateLocalTrainingStepDurationValue();

  // Assert
  const int expected_local_training_step_duration = 10;
  EXPECT_EQ(expected_local_training_step_duration,
            local_training_step_duration);
}

TEST(BraveFederatedLearningFeaturesTest, DefaultCollectionIDLifetimeInDays) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int collection_id_lifetime = features::GetCollectionIdLifetime();

  // Assert
  const int expected_collection_id_lifetime = 1;
  EXPECT_EQ(expected_collection_id_lifetime, collection_id_lifetime);
}

TEST(BraveFederatedLearningFeaturesTest, CollectionIDLifetimeInDays) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kFederatedLearningParameters;
  const char kFieldTrialParameterCollectionIDLifetimeInDays[] =
      "collection_id_lifetime_in_days";
  kFederatedLearningParameters[kFieldTrialParameterCollectionIDLifetimeInDays] =
      "30";
  enabled_features.push_back(
      {features::kFederatedLearning, kFederatedLearningParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int collection_id_lifetime = features::GetCollectionIdLifetime();

  // Assert
  const int expected_collection_id_lifetime = 30;
  EXPECT_EQ(expected_collection_id_lifetime, collection_id_lifetime);
}

}  // namespace brave_federated
