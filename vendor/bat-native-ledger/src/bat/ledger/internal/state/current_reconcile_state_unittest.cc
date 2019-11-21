/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>

#include "bat/ledger/internal/state/current_reconcile_state.h"
#include "bat/ledger/internal/properties/reconcile_direction_properties.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=CurrentReconcileStateTest.*

namespace ledger {

// TODO(tmancey): Test transition from legacy list to reconcile directions

TEST(CurrentReconcileStateTest, ToJsonSerialization) {
  // Arrange
  CurrentReconcileProperties current_reconcile_properties;
  current_reconcile_properties.viewing_id = "ViewingId";
  current_reconcile_properties.anonize_viewing_id = "AnonizeViewingId";
  current_reconcile_properties.registrar_vk = "RegistrarVk";
  current_reconcile_properties.pre_flight = "PreFlight";
  current_reconcile_properties.master_user_token = "MasterUserToken";
  current_reconcile_properties.surveyor_id = "SurveyorId";

  current_reconcile_properties.timestamp =
      std::numeric_limits<uint32_t>::max();
  current_reconcile_properties.rates = {
    { "BAT", 1.0 },
    { "ETH", 2.0 },
    { "LTC", 3.0 },
    { "BTC", 4.0 },
    { "USD", 5.0 },
    { "EUR", 6.0 }
  };
  current_reconcile_properties.amount = "Amount";
  current_reconcile_properties.currency = "Currency";
  current_reconcile_properties.fee = std::numeric_limits<double>::max();

  ReconcileDirectionProperties reconcile_direction_properties;
  reconcile_direction_properties.publisher_key = "PublisherKey";
  reconcile_direction_properties.amount_percent =
      std::numeric_limits<double>::max();
  current_reconcile_properties.directions.push_back(
      reconcile_direction_properties);

  current_reconcile_properties.type = RewardsType::ONE_TIME_TIP;
  current_reconcile_properties.retry_step = ContributionRetry::STEP_RECONCILE;
  current_reconcile_properties.retry_level =
      std::numeric_limits<int32_t>::max();
  current_reconcile_properties.destination = "Destination";
  current_reconcile_properties.proof = "Proof";

  // Act
  const CurrentReconcileState current_reconcile_state;
  const std::string json =
      current_reconcile_state.ToJson(current_reconcile_properties);

  // Assert
  CurrentReconcileProperties expected_current_reconcile_properties;
  current_reconcile_state.FromJson(json,
      &expected_current_reconcile_properties);
  EXPECT_EQ(expected_current_reconcile_properties,
      current_reconcile_properties);
}

TEST(CurrentReconcileStateTest, FromJsonDeserialization) {
  // Arrange
  CurrentReconcileProperties current_reconcile_properties;
  current_reconcile_properties.viewing_id = "ViewingId";
  current_reconcile_properties.anonize_viewing_id = "AnonizeViewingId";
  current_reconcile_properties.registrar_vk = "RegistrarVk";
  current_reconcile_properties.pre_flight = "PreFlight";
  current_reconcile_properties.master_user_token = "MasterUserToken";
  current_reconcile_properties.surveyor_id = "SurveyorId";

  current_reconcile_properties.timestamp =
      std::numeric_limits<uint32_t>::max();
  current_reconcile_properties.rates = {
    { "BAT", 1.0 },
    { "ETH", 2.0 },
    { "LTC", 3.0 },
    { "BTC", 4.0 },
    { "USD", 5.0 },
    { "EUR", 6.0 }
  };
  current_reconcile_properties.amount = "Amount";
  current_reconcile_properties.currency = "Currency";
  current_reconcile_properties.fee = std::numeric_limits<double>::max();

  ReconcileDirectionProperties reconcile_direction_properties;
  reconcile_direction_properties.publisher_key = "PublisherKey";
  reconcile_direction_properties.amount_percent =
      std::numeric_limits<double>::max();
  current_reconcile_properties.directions.push_back(
      reconcile_direction_properties);

  current_reconcile_properties.type = RewardsType::ONE_TIME_TIP;
  current_reconcile_properties.retry_step = ContributionRetry::STEP_RECONCILE;
  current_reconcile_properties.retry_level =
      std::numeric_limits<int32_t>::max();
  current_reconcile_properties.destination = "Destination";
  current_reconcile_properties.proof = "Proof";

  const std::string json = "{\"viewingId\":\"ViewingId\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"preFlight\":\"PreFlight\",\"masterUserToken\":\"MasterUserToken\",\"surveyorInfo\":{\"surveyorId\":\"SurveyorId\"},\"timestamp\":4294967295,\"amount\":\"Amount\",\"currency\":\"Currency\",\"fee\":1.7976931348623157e308,\"type\":8,\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"directions\":[{\"amount_percent\":1.7976931348623157e308,\"publisher_key\":\"PublisherKey\"}],\"retry_step\":1,\"retry_level\":2147483647,\"destination\":\"Destination\",\"proof\":\"Proof\"}";  // NOLINT

  // Act
  CurrentReconcileProperties expected_current_reconcile_properties;
  const CurrentReconcileState current_reconcile_state;
  current_reconcile_state.FromJson(json,
      &expected_current_reconcile_properties);

  // Assert
  EXPECT_EQ(expected_current_reconcile_properties,
      current_reconcile_properties);
}

}  // namespace ledger
