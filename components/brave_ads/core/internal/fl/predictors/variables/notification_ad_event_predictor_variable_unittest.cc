/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_event_predictor_variable.h"

#include <sstream>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

std::string GetValueForEventType(
    const mojom::NotificationAdEventType event_type) {
  NotificationAdEventPredictorVariable predictor_variable(event_type);
  return predictor_variable.GetValue();
}

std::string GetEventTypeAsString(
    const mojom::NotificationAdEventType event_type) {
  std::stringstream ss;
  ss << event_type;
  return ss.str();
}

}  // namespace

TEST(BraveAdsNotificationAdEventPredictorVariableTest, GetDataType) {
  // Arrange
  const NotificationAdEventPredictorVariable predictor_variable(
      mojom::NotificationAdEventType::kViewed);

  // Act & Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kString,
            predictor_variable.GetDataType());
}

TEST(BraveAdsNotificationAdEventPredictorVariableTest, GetValueWhenClicked) {
  // Arrange
  const mojom::NotificationAdEventType event_type =
      mojom::NotificationAdEventType::kClicked;

  // Act & Assert
  EXPECT_EQ(GetEventTypeAsString(event_type), GetValueForEventType(event_type));
}

TEST(BraveAdsNotificationAdEventPredictorVariableTest, GetValueWhenDismissed) {
  // Arrange
  const mojom::NotificationAdEventType event_type =
      mojom::NotificationAdEventType::kDismissed;

  // Act & Assert
  EXPECT_EQ(GetEventTypeAsString(event_type), GetValueForEventType(event_type));
}

TEST(BraveAdsNotificationAdEventPredictorVariableTest, GetValueWhenTimedOut) {
  // Arrange
  const mojom::NotificationAdEventType event_type =
      mojom::NotificationAdEventType::kTimedOut;

  // Act & Assert
  EXPECT_EQ(GetEventTypeAsString(event_type), GetValueForEventType(event_type));
}

}  // namespace brave_ads
