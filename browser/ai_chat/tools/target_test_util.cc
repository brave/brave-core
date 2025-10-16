// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/target_test_util.h"

#include "chrome/common/actor/actor_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat::target_test_util {

void VerifyContentNodeTarget(
    const optimization_guide::proto::ActionTarget& target,
    int expected_content_node_id,
    const std::string& expected_doc_id) {
  ExpectContentNodeTarget(target);
  EXPECT_EQ(target.content_node_id(), expected_content_node_id);
  EXPECT_EQ(target.document_identifier().serialized_token(), expected_doc_id);
}

void VerifyDocumentTarget(const optimization_guide::proto::ActionTarget& target,
                          const std::string& expected_doc_id) {
  ExpectDocumentTarget(target);
  EXPECT_EQ(target.content_node_id(), actor::kRootElementDomNodeId);
  EXPECT_EQ(target.document_identifier().serialized_token(), expected_doc_id);
}

void VerifyCoordinateTarget(
    const optimization_guide::proto::ActionTarget& target,
    int expected_x,
    int expected_y) {
  ExpectCoordinateTarget(target);
  EXPECT_EQ(target.coordinate().x(), expected_x);
  EXPECT_EQ(target.coordinate().y(), expected_y);
}

void ExpectContentNodeTarget(
    const optimization_guide::proto::ActionTarget& target) {
  EXPECT_TRUE(target.has_content_node_id());
  EXPECT_TRUE(target.has_document_identifier());
  EXPECT_FALSE(target.has_coordinate());
}

void ExpectDocumentTarget(
    const optimization_guide::proto::ActionTarget& target) {
  EXPECT_TRUE(target.has_content_node_id());
  EXPECT_TRUE(target.has_document_identifier());
  EXPECT_FALSE(target.has_coordinate());
}

void ExpectCoordinateTarget(
    const optimization_guide::proto::ActionTarget& target) {
  EXPECT_FALSE(target.has_content_node_id());
  EXPECT_FALSE(target.has_document_identifier());
  EXPECT_TRUE(target.has_coordinate());
}

optimization_guide::proto::ActionTarget GetContentNodeTarget(
    int content_node_id,
    const std::string& doc_id) {
  optimization_guide::proto::ActionTarget target;
  target.set_content_node_id(content_node_id);
  auto* doc_identifier = target.mutable_document_identifier();
  doc_identifier->set_serialized_token(doc_id);
  return target;
}

optimization_guide::proto::ActionTarget GetDocumentTarget(
    const std::string& doc_id) {
  optimization_guide::proto::ActionTarget target;
  auto* doc_identifier = target.mutable_document_identifier();
  doc_identifier->set_serialized_token(doc_id);
  return target;
}

optimization_guide::proto::ActionTarget GetCoordinateTarget(int x, int y) {
  optimization_guide::proto::ActionTarget target;
  auto* coordinate = target.mutable_coordinate();
  coordinate->set_x(x);
  coordinate->set_y(y);
  return target;
}

base::Value::Dict GetContentNodeTargetDict(int content_node_id,
                                           const std::string& doc_id) {
  base::Value::Dict target_dict;
  target_dict.Set("content_node_id", content_node_id);
  target_dict.Set("document_identifier", doc_id);
  return target_dict;
}

base::Value::Dict GetDocumentTargetDict(const std::string& doc_id) {
  base::Value::Dict target_dict;
  target_dict.Set("document_identifier", doc_id);
  return target_dict;
}

base::Value::Dict GetCoordinateTargetDict(double x, double y) {
  base::Value::Dict target_dict;
  target_dict.Set("x", x);
  target_dict.Set("y", y);
  return target_dict;
}

}  // namespace ai_chat::target_test_util
