/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/credential_summary.h"

#include <optional>

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {

TEST(CredentialSummaryTest, EmptyMessageIsEmpty) {
  std::optional<CredentialSummary> summary = CredentialSummary::FromMessage("");
  ASSERT_TRUE(summary);
  EXPECT_TRUE(summary->IsEmpty());
  EXPECT_FALSE(summary->IsValid());
  EXPECT_FALSE(summary->NeedsActivation());
}

TEST(CredentialSummaryTest, WhitespaceOnlyMessageIsEmpty) {
  // Trimming is the parser's responsibility, so a whitespace body is "empty",
  // not "malformed".
  std::optional<CredentialSummary> summary =
      CredentialSummary::FromMessage("   \n\t ");
  ASSERT_TRUE(summary);
  EXPECT_TRUE(summary->IsEmpty());
}

TEST(CredentialSummaryTest, EmptyJsonObjectIsEmpty) {
  std::optional<CredentialSummary> summary =
      CredentialSummary::FromMessage("{}");
  ASSERT_TRUE(summary);
  EXPECT_TRUE(summary->IsEmpty());
}

TEST(CredentialSummaryTest, MalformedJsonReturnsNullopt) {
  // Non-empty body that is not a JSON object means a genuine parse failure.
  EXPECT_FALSE(CredentialSummary::FromMessage("not json").has_value());
  EXPECT_FALSE(CredentialSummary::FromMessage("[1, 2, 3]").has_value());
}

TEST(CredentialSummaryTest, ActiveWithRemainingIsValid) {
  std::optional<CredentialSummary> summary = CredentialSummary::FromMessage(
      R"({"active": true, "remaining_credential_count": 3})");
  ASSERT_TRUE(summary);
  EXPECT_FALSE(summary->IsEmpty());
  EXPECT_TRUE(summary->IsValid());
  EXPECT_FALSE(summary->NeedsActivation());
}

TEST(CredentialSummaryTest, InactiveWithRemainingNeedsActivation) {
  std::optional<CredentialSummary> summary = CredentialSummary::FromMessage(
      R"({"active": false, "remaining_credential_count": 3})");
  ASSERT_TRUE(summary);
  EXPECT_FALSE(summary->IsEmpty());
  EXPECT_FALSE(summary->IsValid());
  EXPECT_TRUE(summary->NeedsActivation());
}

TEST(CredentialSummaryTest, InactiveNoRemainingFallsThroughAllPredicates) {
  // The out-of-credentials case: a real subscription record, but nothing left
  // to redeem. It must be reported as none of empty/valid/needs-activation so
  // the caller routes it to the session-expired path.
  std::optional<CredentialSummary> summary = CredentialSummary::FromMessage(
      R"({"active": false, "remaining_credential_count": 0})");
  ASSERT_TRUE(summary);
  EXPECT_FALSE(summary->IsEmpty());
  EXPECT_FALSE(summary->IsValid());
  EXPECT_FALSE(summary->NeedsActivation());
}

TEST(CredentialSummaryTest, ActiveNoRemainingFallsThroughAllPredicates) {
  std::optional<CredentialSummary> summary = CredentialSummary::FromMessage(
      R"({"active": true, "remaining_credential_count": 0})");
  ASSERT_TRUE(summary);
  EXPECT_FALSE(summary->IsEmpty());
  EXPECT_FALSE(summary->IsValid());
  EXPECT_FALSE(summary->NeedsActivation());
}

TEST(CredentialSummaryTest, MissingFieldsDefaultToOutOfCredentials) {
  std::optional<CredentialSummary> summary =
      CredentialSummary::FromMessage(R"({"unrelated": "value"})");
  ASSERT_TRUE(summary);
  EXPECT_FALSE(summary->IsEmpty());
  EXPECT_FALSE(summary->IsValid());
  EXPECT_FALSE(summary->NeedsActivation());
}

TEST(CredentialSummaryTest, SurroundingWhitespaceIsTolerated) {
  std::optional<CredentialSummary> summary = CredentialSummary::FromMessage(
      "\n  {\"active\": true, \"remaining_credential_count\": 1}  \n");
  ASSERT_TRUE(summary);
  EXPECT_TRUE(summary->IsValid());
}

}  // namespace brave_vpn::v2
