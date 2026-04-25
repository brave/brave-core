// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

TEST(EngineConsumerUnitTest, GetStrArrFromTabOrganizationResponses) {
  std::vector<EngineConsumer::GenerationResult> results;
  EXPECT_EQ(EngineConsumer::GetStrArrFromTabOrganizationResponses(results),
            base::unexpected(mojom::APIError::InternalError));

  auto add_result = [&results](const std::string& completion_text) {
    results.push_back(base::ok(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New(completion_text)),
        std::nullopt)));
  };

  // Test specifically the "Skip empty results" code path
  results.clear();

  // This creates a result with an event that is not a completion event
  results.push_back(base::ok(EngineConsumer::GenerationResultData(
      mojom::ConversationEntryEvent::NewSearchStatusEvent(
          mojom::SearchStatusEvent::New(true)),
      std::nullopt)));

  // This creates a result with no event
  results.push_back(
      base::ok(EngineConsumer::GenerationResultData(nullptr, std::nullopt)));

  // This creates a result with an empty completion
  results.push_back(base::ok(EngineConsumer::GenerationResultData(
      mojom::ConversationEntryEvent::NewCompletionEvent(
          mojom::CompletionEvent::New("")),
      std::nullopt)));

  // Add a valid result
  add_result("[\"validString\"]");

  // Verify the empty results are skipped and we get only the valid string
  EXPECT_EQ(EngineConsumer::GetStrArrFromTabOrganizationResponses(results),
            std::vector<std::string>({"validString"}));

  // Test with an empty vector
  results.clear();
  EXPECT_EQ(EngineConsumer::GetStrArrFromTabOrganizationResponses(results),
            base::unexpected(mojom::APIError::InternalError));

  // Test with only one invalid result
  add_result("   ");
  EXPECT_EQ(EngineConsumer::GetStrArrFromTabOrganizationResponses(results),
            base::unexpected(mojom::APIError::InternalError));

  // Test only valid strings are added to the result
  add_result("null");
  add_result("[]");
  add_result("[   ]");
  add_result("[null]");
  add_result("[\"\"]");
  add_result("[1, 2, 3]");
  add_result("[\"string1\", \"string2\", \"string3\"]");
  // Test response with newlines in the array
  add_result("[\n  \"string10\",\n  \"string11\",\n  \"string12\"\n]");
  add_result(
      "Result\n: [\"\xF0\x9F\x98\x8A string4\", \"string5\", \"string6\"] "
      "TEST");
  add_result("[{[\"string7\", \"string8\", \"string9\"]}]");

  EXPECT_EQ(
      EngineConsumer::GetStrArrFromTabOrganizationResponses(results),
      std::vector<std::string>(
          {"string1", "string2", "string3", "string10", "string11", "string12",
           "\xF0\x9F\x98\x8A string4", "string5", "string6"}));

  // Test having a error message inside the response
  results.clear();
  add_result("[\"string1\", \"string2\", \"string3\"]");
  results.push_back(base::unexpected(mojom::APIError::RateLimitReached));
  EXPECT_EQ(EngineConsumer::GetStrArrFromTabOrganizationResponses(results),
            base::unexpected(mojom::APIError::RateLimitReached));
}

}  // namespace ai_chat
