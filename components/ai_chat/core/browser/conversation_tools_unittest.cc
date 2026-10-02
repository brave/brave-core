// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_tools.h"

#include <string>
#include <string_view>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

mojom::ToolUseEventPtr CreateToolUse(std::string_view tool_name,
                                     std::string_view arguments_json) {
  return mojom::ToolUseEvent::New(std::string(tool_name), "tool_id_1",
                                  std::string(arguments_json), std::nullopt,
                                  std::nullopt, nullptr, false);
}

mojom::ToolUseEventPtr CreateUserChoice(std::string_view arguments_json) {
  return CreateToolUse(mojom::kUserChoiceToolName, arguments_json);
}

}  // namespace

TEST(ConversationToolsUnitTest, FollowUpSuggestions) {
  auto tool_use = CreateUserChoice(
      R"({"choice_type":"follow_up","choices":["Ask A","Ask B"]})");
  EXPECT_THAT(GetFollowUpSuggestionsFromToolUse(*tool_use),
              testing::Optional(testing::ElementsAre("Ask A", "Ask B")));
}

TEST(ConversationToolsUnitTest, FollowUpSuggestionsIgnoresUnusableChoices) {
  // Anything the model provides which can't be displayed as a suggestion is
  // dropped, rather than discarding the whole request.
  auto tool_use =
      CreateUserChoice(R"({"choice_type":"follow_up",)"
                       R"("choices":["Ask A","",7,null,{},"Ask B"]})");
  EXPECT_THAT(GetFollowUpSuggestionsFromToolUse(*tool_use),
              testing::Optional(testing::ElementsAre("Ask A", "Ask B")));
}

TEST(ConversationToolsUnitTest, FollowUpSuggestionsWithNoUsableChoices) {
  // Still a follow-up request - the caller needs to know so it can stop waiting
  // for an answer - but there is nothing to suggest.
  for (const auto* arguments_json : {
           R"({"choice_type":"follow_up"})",
           R"({"choice_type":"follow_up","choices":[]})",
           R"({"choice_type":"follow_up","choices":["",""]})",
           R"({"choice_type":"follow_up","choices":"Ask A"})",
           R"({"choice_type":"follow_up","choices":null})",
       }) {
    SCOPED_TRACE(arguments_json);
    auto tool_use = CreateUserChoice(arguments_json);
    EXPECT_THAT(GetFollowUpSuggestionsFromToolUse(*tool_use),
                testing::Optional(testing::IsEmpty()));
  }
}

TEST(ConversationToolsUnitTest, NotFollowUpSuggestions) {
  // A preference choice is waiting on the user, and anything unrecognized
  // (e.g. an entry saved before choice_type existed) is treated the same way.
  for (const auto* arguments_json : {
           R"({"choice_type":"preference","choices":["1pm","2:30pm"]})",
           R"({"choice_type":"","choices":["Ask A"]})",
           R"({"choice_type":"followup","choices":["Ask A"]})",
           R"({"choice_type":"FOLLOW_UP","choices":["Ask A"]})",
           R"({"choice_type":7,"choices":["Ask A"]})",
           R"({"choices":["Ask A"]})",
           R"({})",
       }) {
    SCOPED_TRACE(arguments_json);
    auto tool_use = CreateUserChoice(arguments_json);
    EXPECT_EQ(GetFollowUpSuggestionsFromToolUse(*tool_use), std::nullopt);
  }
}

TEST(ConversationToolsUnitTest, FollowUpSuggestionsWithUnusableInput) {
  for (const auto* arguments_json : {
           "",
           "not json",
           R"({"choice_type":"follow_up")",
           R"(["choice_type","follow_up"])",
           R"("follow_up")",
           "null",
       }) {
    SCOPED_TRACE(arguments_json);
    auto tool_use = CreateUserChoice(arguments_json);
    EXPECT_EQ(GetFollowUpSuggestionsFromToolUse(*tool_use), std::nullopt);
  }
}

TEST(ConversationToolsUnitTest, FollowUpSuggestionsOnlyForUserChoiceTool) {
  // Another tool's arguments must never be mistaken for suggestions.
  auto tool_use = CreateToolUse(
      "some_other_tool",
      R"({"choice_type":"follow_up","choices":["Ask A","Ask B"]})");
  EXPECT_EQ(GetFollowUpSuggestionsFromToolUse(*tool_use), std::nullopt);
}

}  // namespace ai_chat
