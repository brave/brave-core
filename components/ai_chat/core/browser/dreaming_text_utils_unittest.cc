// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/dreaming_text_utils.h"

#include <string>
#include <vector>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

TEST(DreamingTextUtilsTest, SplitIntoSentences) {
  EXPECT_THAT(
      SplitIntoSentences("We just moved to Berlin. Any good gyms?"),
      testing::ElementsAre("We just moved to Berlin.", "Any good gyms?"));
  EXPECT_THAT(SplitIntoSentences("I have a dog. Her name is Luna!"),
              testing::ElementsAre("I have a dog.", "Her name is Luna!"));
}

TEST(DreamingTextUtilsTest, SplitIntoSentencesTrimsAndKeepsOneSentence) {
  EXPECT_THAT(SplitIntoSentences("  I am vegetarian  "),
              testing::ElementsAre("I am vegetarian"));
}

TEST(DreamingTextUtilsTest, SplitIntoSentencesEmpty) {
  EXPECT_TRUE(SplitIntoSentences("").empty());
  EXPECT_TRUE(SplitIntoSentences("   \n ").empty());
}

TEST(DreamingTextUtilsTest, SplitIntoSentencesKeepsNonAscii) {
  EXPECT_THAT(SplitIntoSentences("Ich wohne in München. 我住在柏林。"),
              testing::ElementsAre("Ich wohne in München.", "我住在柏林。"));
}

TEST(DreamingTextUtilsTest, DeniedPatterns) {
  EXPECT_TRUE(HasDeniedPattern("Mail me at jane.doe+leo@example.com"));
  EXPECT_TRUE(HasDeniedPattern("Call +1 (555) 123-4567 please"));
  EXPECT_TRUE(HasDeniedPattern("My account number is 12345678"));
  EXPECT_TRUE(HasDeniedPattern("My card is 4111 1111 1111 1111"));
  EXPECT_TRUE(HasDeniedPattern("My SSN is 123-45-6789"));
  EXPECT_TRUE(HasDeniedPattern("Room code 5551234"));
}

TEST(DreamingTextUtilsTest, AllowedNumbers) {
  EXPECT_FALSE(HasDeniedPattern("I live in Berlin"));
  EXPECT_FALSE(HasDeniedPattern("I prefer routes under 15 km"));
  EXPECT_FALSE(HasDeniedPattern("We moved on 2026-09-20"));
  EXPECT_FALSE(HasDeniedPattern("I was born in 1990"));
  EXPECT_FALSE(HasDeniedPattern("Between 2026 and 2027 I will train"));
  EXPECT_FALSE(HasDeniedPattern("Use @brave on social media"));
}

}  // namespace ai_chat
