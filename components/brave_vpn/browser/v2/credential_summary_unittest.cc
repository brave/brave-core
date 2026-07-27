/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/credential_summary.h"

#include <optional>
#include <string>

#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {

TEST(CredentialSummaryTest, EmptyInputsAreEmpty) {
  // Trimming and empty-object handling are the parser's responsibility, so
  // all of these are "no subscription on record", not "malformed".
  for (const char* input : {"", "   \n\t ", "{}"}) {
    SCOPED_TRACE(testing::Message() << "input: '" << input << "'");
    std::optional<CredentialSummary> summary =
        CredentialSummary::FromMessage(input);
    ASSERT_TRUE(summary);
    EXPECT_TRUE(summary->IsEmpty());
    EXPECT_FALSE(summary->IsValid());
    EXPECT_FALSE(summary->NeedsActivation());
  }
}

TEST(CredentialSummaryTest, MalformedJsonReturnsNullopt) {
  // Non-empty body that is not a JSON object means a genuine parse failure.
  EXPECT_FALSE(CredentialSummary::FromMessage("not json").has_value());
  EXPECT_FALSE(CredentialSummary::FromMessage("[1, 2, 3]").has_value());
}

TEST(CredentialSummaryTest, ActiveWithRemainingIsValid) {
  std::optional<CredentialSummary> summary = CredentialSummary::FromMessage(
      absl::StrFormat(R"({"%s": true, "%s": 3})", kCredentialSummaryActiveKey,
                      kCredentialSummaryRemainingCredentialCountKey));
  ASSERT_TRUE(summary);
  EXPECT_FALSE(summary->IsEmpty());
  EXPECT_TRUE(summary->IsValid());
  EXPECT_FALSE(summary->NeedsActivation());
}

TEST(CredentialSummaryTest, InactiveWithRemainingNeedsActivation) {
  std::optional<CredentialSummary> summary = CredentialSummary::FromMessage(
      absl::StrFormat(R"({"%s": false, "%s": 3})", kCredentialSummaryActiveKey,
                      kCredentialSummaryRemainingCredentialCountKey));
  ASSERT_TRUE(summary);
  EXPECT_FALSE(summary->IsEmpty());
  EXPECT_FALSE(summary->IsValid());
  EXPECT_TRUE(summary->NeedsActivation());
}

TEST(CredentialSummaryTest, OutOfCredentialsVariantsFallThroughAllPredicates) {
  // A real subscription record with nothing left to redeem, in all its shapes.
  // These must be none of empty/valid/needs-activation so the caller routes
  // them to the session-expired path.
  for (const std::string& input :
       {absl::StrFormat(R"({"%s": false, "%s": 0})",
                        kCredentialSummaryActiveKey,
                        kCredentialSummaryRemainingCredentialCountKey),
        absl::StrFormat(R"({"%s": true, "%s": 0})", kCredentialSummaryActiveKey,
                        kCredentialSummaryRemainingCredentialCountKey),
        std::string(R"({"unrelated": "value"})")}) {
    SCOPED_TRACE(testing::Message() << "input: '" << input << "'");
    std::optional<CredentialSummary> summary =
        CredentialSummary::FromMessage(input);
    ASSERT_TRUE(summary);
    EXPECT_FALSE(summary->IsEmpty());
    EXPECT_FALSE(summary->IsValid());
    EXPECT_FALSE(summary->NeedsActivation());
  }
}

TEST(CredentialSummaryTest, SurroundingWhitespaceIsTolerated) {
  std::optional<CredentialSummary> summary =
      CredentialSummary::FromMessage(absl::StrFormat(
          "\n  {\"%s\": true, \"%s\": 1}  \n", kCredentialSummaryActiveKey,
          kCredentialSummaryRemainingCredentialCountKey));
  ASSERT_TRUE(summary);
  EXPECT_TRUE(summary->IsValid());
}

}  // namespace brave_vpn::v2
