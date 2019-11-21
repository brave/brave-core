/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>

#include "bat/ledger/internal/state/reconcile_direction_state.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ReconcileDirectionStateTest.*

namespace ledger {

TEST(ReconcileDirectionStateTest, ToJsonSerialization) {
  // Arrange
  ReconcileDirectionProperties reconcile_direction_properties;
  reconcile_direction_properties.publisher_key = "ViewingId";
  reconcile_direction_properties.amount_percent =
      std::numeric_limits<double>::max();

  // Act
  const ReconcileDirectionState reconcile_direction_state;
  const std::string json =
      reconcile_direction_state.ToJson(reconcile_direction_properties);

  // Assert
  ReconcileDirectionProperties expected_reconcile_direction_properties;
  reconcile_direction_state.FromJson(json,
      &expected_reconcile_direction_properties);
  EXPECT_EQ(expected_reconcile_direction_properties,
      reconcile_direction_properties);
}

TEST(ReconcileDirectionStateTest, FromJsonDeserialization) {
  // Arrange
  ReconcileDirectionProperties reconcile_direction_properties;
  reconcile_direction_properties.publisher_key = "ViewingId";
  reconcile_direction_properties.amount_percent =
      std::numeric_limits<double>::max();

  const std::string json = "{\"amount_percent\":1.7976931348623157e308,\"publisher_key\":\"ViewingId\"}";  // NOLINT

  // Act
  ReconcileDirectionProperties expected_reconcile_direction_properties;
  const ReconcileDirectionState reconcile_direction_state;
  reconcile_direction_state.FromJson(json,
      &expected_reconcile_direction_properties);

  // Assert
  EXPECT_EQ(expected_reconcile_direction_properties,
      reconcile_direction_properties);
}

}  // namespace ledger
