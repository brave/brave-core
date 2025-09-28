// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_TARGET_TEST_UTIL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_TARGET_TEST_UTIL_H_

#include <string>

#include "base/values.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat::target_test_util {

// Verify that an ActionTarget contains the expected content node information
void VerifyContentNodeTarget(
    const optimization_guide::proto::ActionTarget& target,
    int expected_content_node_id,
    const std::string& expected_doc_id);

// Verify that an ActionTarget contains the expected document information
// with no node id.
void VerifyDocumentTarget(const optimization_guide::proto::ActionTarget& target,
                          const std::string& expected_doc_id);

// Verify that an ActionTarget contains the expected coordinate information
void VerifyCoordinateTarget(
    const optimization_guide::proto::ActionTarget& target,
    int expected_x,
    int expected_y);

// Verify that an ActionTarget is a content node target (vs coordinate)
void ExpectContentNodeTarget(
    const optimization_guide::proto::ActionTarget& target);

// Verify that an ActionTarget is a document target, with no node id
void ExpectDocumentTarget(
    const optimization_guide::proto::ActionTarget& target);

// Verify that an ActionTarget is a coordinate target (vs content node)
void ExpectCoordinateTarget(
    const optimization_guide::proto::ActionTarget& target);

// Creates a standard content node target for testing
optimization_guide::proto::ActionTarget GetContentNodeTarget(
    int content_node_id = 42,
    const std::string& doc_id = "doc123");

// Creates a standard document target for testing
optimization_guide::proto::ActionTarget GetDocumentTarget(
    const std::string& doc_id = "doc123");

// Creates a standard coordinate target for testing
optimization_guide::proto::ActionTarget GetCoordinateTarget(int x = 100,
                                                            int y = 200);

// Creates a standard content node target JSON for testing
base::Value::Dict GetContentNodeTargetDict(
    int content_node_id = 42,
    const std::string& doc_id = "doc123");

// Creates a standard document target JSON for testing
base::Value::Dict GetDocumentTargetDict(const std::string& doc_id = "doc123");

// Creates a standard coordinate target JSON for testing
base::Value::Dict GetCoordinateTargetDict(double x = 100.5, double y = 200.5);

}  // namespace ai_chat::target_test_util

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_TARGET_TEST_UTIL_H_
