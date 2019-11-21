/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>

#include "bat/ledger/internal/state/publisher_state.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PublisherStateTest.*

namespace ledger {

TEST(PublisherStateTest, ToJsonSerialization) {
  // Arrange
  PublisherProperties publisher_properties;
  publisher_properties.id = "Id";
  publisher_properties.duration = std::numeric_limits<uint32_t>::max();
  publisher_properties.score = std::numeric_limits<double>::max();
  publisher_properties.visits = std::numeric_limits<uint32_t>::max();
  publisher_properties.percent = std::numeric_limits<uint32_t>::max();
  publisher_properties.weight = std::numeric_limits<double>::max();
  publisher_properties.status = std::numeric_limits<uint32_t>::max();

  // Act
  const PublisherState publisher_state;
  const std::string json = publisher_state.ToJson(publisher_properties);

  // Assert
  PublisherProperties expected_publisher_properties;
  publisher_state.FromJson(json, &expected_publisher_properties);
  EXPECT_EQ(expected_publisher_properties, publisher_properties);
}

TEST(PublisherStateTest, FromJsonDeserialization) {
  // Arrange
  PublisherProperties publisher_properties;
  publisher_properties.id = "Id";
  publisher_properties.duration = std::numeric_limits<uint32_t>::max();
  publisher_properties.score = std::numeric_limits<double>::max();
  publisher_properties.visits = std::numeric_limits<uint32_t>::max();
  publisher_properties.percent = std::numeric_limits<uint32_t>::max();
  publisher_properties.weight = std::numeric_limits<double>::max();
  publisher_properties.status = std::numeric_limits<uint32_t>::max();

  const std::string json = "{\"id\":\"Id\",\"duration\":4294967295,\"score\":1.7976931348623157e308,\"visits\":4294967295,\"percent\":4294967295,\"weight\":1.7976931348623157e308,\"status\":4294967295}";  // NOLINT

  // Act
  PublisherProperties expected_publisher_properties;
  const PublisherState publisher_state;
  publisher_state.FromJson(json, &expected_publisher_properties);

  // Assert
  EXPECT_EQ(expected_publisher_properties, publisher_properties);
}

}  // namespace ledger
