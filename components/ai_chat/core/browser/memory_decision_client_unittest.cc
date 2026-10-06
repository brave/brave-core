// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/memory_decision_client.h"

#include <optional>

#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {
constexpr double kThreshold = 0.8;
constexpr double kMargin = 0.2;
}  // namespace

TEST(MemoryDecisionClientTest, CertainAnswer) {
  AnswerProbabilities<bool> probabilities{{true, 0.95}, {false, 0.05}};
  EXPECT_EQ(GetCertainAnswer(probabilities, kThreshold, kMargin), true);

  AnswerProbabilities<bool> no{{true, 0.03}, {false, 0.97}};
  EXPECT_EQ(GetCertainAnswer(no, kThreshold, kMargin), false);
}

TEST(MemoryDecisionClientTest, TopAnswerBelowThresholdIsNotCertain) {
  AnswerProbabilities<bool> probabilities{{true, 0.75}, {false, 0.25}};
  EXPECT_EQ(GetCertainAnswer(probabilities, kThreshold, kMargin), std::nullopt);
}

TEST(MemoryDecisionClientTest, SmallMarginIsNotCertain) {
  // The top answer is above the threshold, but the next answer is too close.
  AnswerProbabilities<SafetyAnswer> probabilities{
      {SafetyAnswer::kOk, 0.82},
      {SafetyAnswer::kSensitive, 0.70},
      {SafetyAnswer::kInstruction, 0.01}};
  EXPECT_EQ(GetCertainAnswer(probabilities, kThreshold, kMargin), std::nullopt);
}

TEST(MemoryDecisionClientTest, MarginUsesTheNextHighestAnswer) {
  // The order of the map keys must not matter.
  AnswerProbabilities<SafetyAnswer> probabilities{
      {SafetyAnswer::kOk, 0.1},
      {SafetyAnswer::kSensitive, 0.85},
      {SafetyAnswer::kInstruction, 0.70},
      {SafetyAnswer::kShortLived, 0.0}};
  EXPECT_EQ(GetCertainAnswer(probabilities, kThreshold, kMargin), std::nullopt);

  probabilities[SafetyAnswer::kInstruction] = 0.2;
  EXPECT_EQ(GetCertainAnswer(probabilities, kThreshold, kMargin),
            SafetyAnswer::kSensitive);
}

TEST(MemoryDecisionClientTest, MarginEqualToTheLimitIsCertain) {
  AnswerProbabilities<bool> probabilities{{true, 0.9}, {false, 0.7}};
  EXPECT_EQ(GetCertainAnswer(probabilities, kThreshold, kMargin), true);
}

TEST(MemoryDecisionClientTest, NoAnswersIsNotCertain) {
  EXPECT_EQ(GetCertainAnswer(AnswerProbabilities<bool>(), kThreshold, kMargin),
            std::nullopt);
}

}  // namespace ai_chat
