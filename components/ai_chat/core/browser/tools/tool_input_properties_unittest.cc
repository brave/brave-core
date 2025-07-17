// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"

#include <string>
#include <vector>

#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

TEST(ToolInputPropertiesTest, StringPropertyWithDescription) {
  auto result = StringProperty("A location for weather data");

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "string",
    "description": "A location for weather data"
  })"));
}

TEST(ToolInputPropertiesTest, StringPropertyWithEmptyDescription) {
  auto result = StringProperty("");

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "string"
  })"));
}

TEST(ToolInputPropertiesTest, StringPropertyWithEnumValues) {
  auto result = StringProperty(
      "Temperature unit",
      std::vector<std::string>{"celsius", "fahrenheit", "kelvin"});

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "string",
    "description": "Temperature unit",
    "enum": ["celsius", "fahrenheit", "kelvin"]
  })"));
}

TEST(ToolInputPropertiesTest, StringPropertyWithEmptyDescriptionAndEnumValues) {
  auto result =
      StringProperty("", std::vector<std::string>{"option1", "option2"});

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "string",
    "enum": ["option1", "option2"]
  })"));
}

TEST(ToolInputPropertiesTest, StringPropertyWithEmptyEnumValues) {
  auto result = StringProperty("Test description", std::vector<std::string>{});

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "string",
    "description": "Test description",
  })"));
}

TEST(ToolInputPropertiesTest, ArrayPropertyWithDescription) {
  auto result =
      ArrayProperty("List of locations", StringProperty("A single location"));

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "array",
    "description": "List of locations",
    "items": {
      "type": "string",
      "description": "A single location"
    }
  })"));
}

TEST(ToolInputPropertiesTest, ArrayPropertyWithEmptyDescription) {
  auto result = ArrayProperty("", BooleanProperty("A boolean item"));

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "array",
    "items": {
      "type": "boolean",
      "description": "A boolean item"
    }
  })"));
}

TEST(ToolInputPropertiesTest, ObjectPropertyWithDescription) {
  auto result =
      ObjectProperty("Configuration object",
                     {{"enabled", BooleanProperty("Enable the feature")},
                      {"count", IntegerProperty("Number of items")}});

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "object",
    "description": "Configuration object",
    "properties": {
      "enabled": {
        "type": "boolean",
        "description": "Enable the feature"
      },
      "count": {
        "type": "integer",
        "description": "Number of items"
      }
    }
  })"));
}

TEST(ToolInputPropertiesTest, ObjectPropertyWithEmptyDescription) {
  auto result = ObjectProperty("", {{"name", StringProperty("User name")}});

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "object",
    "properties": {
      "name": {
        "type": "string",
        "description": "User name"
      }
    }
  })"));
}

TEST(ToolInputPropertiesTest, BooleanPropertyWithDescription) {
  auto result = BooleanProperty("Enable debugging mode");

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "boolean",
    "description": "Enable debugging mode"
  })"));
}

TEST(ToolInputPropertiesTest, BooleanPropertyWithEmptyDescription) {
  auto result = BooleanProperty("");

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "boolean"
  })"));
}

TEST(ToolInputPropertiesTest, NumberPropertyWithDescription) {
  auto result = NumberProperty("Temperature in degrees");

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "number",
    "description": "Temperature in degrees"
  })"));
}

TEST(ToolInputPropertiesTest, NumberPropertyWithEmptyDescription) {
  auto result = NumberProperty("");

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "number"
  })"));
}

TEST(ToolInputPropertiesTest, IntegerPropertyWithDescription) {
  auto result = IntegerProperty("Number of retries");

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "integer",
    "description": "Number of retries"
  })"));
}

TEST(ToolInputPropertiesTest, IntegerPropertyWithEmptyDescription) {
  auto result = IntegerProperty("");

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "integer"
  })"));
}

TEST(ToolInputPropertiesTest, ComplexNestedStructure) {
  auto result = ObjectProperty(
      "Weather request",
      {{"locations",
        ArrayProperty(
            "List of locations",
            ObjectProperty("Geographic location",
                           {{"lat", NumberProperty("Latitude coordinate")},
                            {"lng", NumberProperty("Longitude coordinate")},
                            {"name", StringProperty("Location name")}}))},
       {"unit",
        StringProperty("Temperature unit",
                       std::vector<std::string>{"celsius", "fahrenheit"})},
       {"detailed", BooleanProperty("Include detailed forecast")}});

  EXPECT_THAT(result, base::test::IsJson(R"({
    "type": "object",
    "description": "Weather request",
    "properties": {
      "locations": {
        "type": "array",
        "description": "List of locations",
        "items": {
          "type": "object",
          "description": "Geographic location",
          "properties": {
            "lat": {
              "type": "number",
              "description": "Latitude coordinate"
            },
            "lng": {
              "type": "number",
              "description": "Longitude coordinate"
            },
            "name": {
              "type": "string",
              "description": "Location name"
            }
          }
        }
      },
      "unit": {
        "type": "string",
        "description": "Temperature unit",
        "enum": ["celsius", "fahrenheit"]
      },
      "detailed": {
        "type": "boolean",
        "description": "Include detailed forecast"
      }
    }
  })"));
}

}  // namespace ai_chat
