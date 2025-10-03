// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/target_util.h"

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "chrome/common/actor/actor_constants.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "content/public/test/browser_task_environment.h"
#include "gmock/gmock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat::target_util {

class TargetUtilTest : public testing::Test {
 public:
  void SetUp() override {}

 protected:
  content::BrowserTaskEnvironment task_environment_;
};

// Tests for TargetProperty() function
TEST_F(TargetUtilTest, TargetProperty_CompleteSchemaStructure) {
  auto property = TargetProperty("Click target element");
  // Verify the complete schema structure
  EXPECT_THAT(property,
              base::test::IsSupersetOfValue(base::test::ParseJsonDict(R"JSON({
    "description": "Click target element",
    "anyOf": [
      {
        "properties": {
            "content_node_id": {
              "type": "integer"
            },
            "document_identifier": {
              "type": "string"
            }
        },
        "type": "object"
      }, {
        "properties": {
            "x": {
              "type": "number"
            },
            "y": {
              "type": "number"
            }
        },
        "type": "object"
      }
    ]
  })JSON")));
}

// Tests for ParseTargetInput() function
TEST_F(TargetUtilTest, ParseTargetInput_ValidCoordinates) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "x": 150.5,
    "y": 250.7
  })JSON");

  auto result = ParseTargetInput(target_dict);

  ASSERT_TRUE(result.has_value());

  const auto& target = result.value();
  EXPECT_TRUE(target.has_coordinate());
  EXPECT_FALSE(target.has_content_node_id());
  EXPECT_FALSE(target.has_document_identifier());

  const auto& coordinate = target.coordinate();
  EXPECT_EQ(coordinate.x(), 150);  // Truncated to int32
  EXPECT_EQ(coordinate.y(), 250);  // Truncated to int32
}

TEST_F(TargetUtilTest, ParseTargetInput_ValidDocumentIdentifier) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "document_identifier": "test_doc"
  })JSON");

  auto result = ParseTargetInput(target_dict);

  ASSERT_TRUE(result.has_value());

  const auto& target = result.value();
  EXPECT_FALSE(target.has_coordinate());
  // Should default to root node id
  EXPECT_TRUE(target.has_content_node_id());
  EXPECT_EQ(target.content_node_id(), actor::kRootElementDomNodeId);

  EXPECT_TRUE(target.has_document_identifier());

  EXPECT_EQ(target.document_identifier().serialized_token(), "test_doc");
}

TEST_F(TargetUtilTest, ParseTargetInput_ValidContentNodeId) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "content_node_id": 42,
    "document_identifier": "test_doc_123"
  })JSON");

  auto result = ParseTargetInput(target_dict);

  ASSERT_TRUE(result.has_value());

  const auto& target = result.value();
  EXPECT_FALSE(target.has_coordinate());
  EXPECT_TRUE(target.has_content_node_id());
  EXPECT_TRUE(target.has_document_identifier());

  EXPECT_EQ(target.content_node_id(), 42);
  EXPECT_EQ(target.document_identifier().serialized_token(), "test_doc_123");
}

TEST_F(TargetUtilTest, ParseTargetInput_MissingX) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "y": 200
  })JSON");

  auto result = ParseTargetInput(target_dict);

  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            "Invalid coordinates: both 'x' and 'y' are required");
}

TEST_F(TargetUtilTest, ParseTargetInput_MissingY) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "x": 100
  })JSON");

  auto result = ParseTargetInput(target_dict);

  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            "Invalid coordinates: both 'x' and 'y' are required");
}

TEST_F(TargetUtilTest, ParseTargetInput_MissingDocumentIdentifier) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "content_node_id": 42
  })JSON");

  auto result = ParseTargetInput(target_dict);

  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            "Invalid identifiers: 'document_identifier' is required when "
            "specifying 'content_node_id'");
}

TEST_F(TargetUtilTest, ParseTargetInput_BothApproaches) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "x": 100,
    "y": 200,
    "content_node_id": 42,
    "document_identifier": "test_doc"
  })JSON");

  auto result = ParseTargetInput(target_dict);

  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            "Target must contain either 'x' and 'y' or "
            "'document_identifier' with optional 'content_node_id', not both");
}

TEST_F(TargetUtilTest, ParseTargetInput_BothApproachesDocumentIdentifier) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "x": 100,
    "y": 200,
    "document_identifier": "test_doc"
  })JSON");

  auto result = ParseTargetInput(target_dict);

  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            "Target must contain either 'x' and 'y' or "
            "'document_identifier' with optional 'content_node_id', not both");
}

TEST_F(TargetUtilTest,
       ParseTargetInput_BothApproachesMissingDocumentIdentifier) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "x": 100,
    "y": 200,
    "content_node_id": 42,
  })JSON");

  auto result = ParseTargetInput(target_dict);

  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            "Target must contain either 'x' and 'y' or "
            "'document_identifier' with optional 'content_node_id', not both");
}

TEST_F(TargetUtilTest, ParseTargetInput_NeitherApproach) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({})JSON");

  auto result = ParseTargetInput(target_dict);

  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(
      result.error(),
      "Target must contain one of either 'x' and 'y' or 'document_identifier' "
      "and optional 'content_node_id'");
}

TEST_F(TargetUtilTest, ParseTargetInput_PartialCoordinatesBothPresent) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "x": 100,
    "content_node_id": 42
  })JSON");

  auto result = ParseTargetInput(target_dict);

  EXPECT_FALSE(result.has_value());
  EXPECT_THAT(
      result.error(),
      testing::HasSubstr(
          "Target must contain either 'x' and 'y' or 'document_identifier'"));
}

TEST_F(TargetUtilTest, ParseTargetInput_PartialIdentifiersBothPresent) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "y": 200,
    "document_identifier": "test_doc"
  })JSON");

  auto result = ParseTargetInput(target_dict);

  EXPECT_FALSE(result.has_value());
  EXPECT_THAT(
      result.error(),
      testing::HasSubstr(
          "Target must contain either 'x' and 'y' or 'document_identifier'"));
}

TEST_F(TargetUtilTest, ParseTargetInput_WithoutErrorString) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "x": 100
  })JSON");

  auto result = ParseTargetInput(target_dict);

  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            "Invalid coordinates: both 'x' and 'y' are required");
}

TEST_F(TargetUtilTest, ParseTargetInput_NegativeCoordinates) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "x": -50,
    "y": -100
  })JSON");

  auto result = ParseTargetInput(target_dict);

  ASSERT_TRUE(result.has_value());

  const auto& target = result.value();
  EXPECT_TRUE(target.has_coordinate());

  const auto& coordinate = target.coordinate();
  EXPECT_EQ(coordinate.x(), -50);
  EXPECT_EQ(coordinate.y(), -100);
}

TEST_F(TargetUtilTest, ParseTargetInput_ZeroContentNodeId) {
  auto target_dict = base::test::ParseJsonDict(R"JSON({
    "content_node_id": 0,
    "document_identifier": "root_doc"
  })JSON");

  auto result = ParseTargetInput(target_dict);

  ASSERT_TRUE(result.has_value());

  const auto& target = result.value();
  EXPECT_TRUE(target.has_content_node_id());
  EXPECT_EQ(target.content_node_id(), 0);
  EXPECT_EQ(target.document_identifier().serialized_token(), "root_doc");
}

}  // namespace ai_chat::target_util
