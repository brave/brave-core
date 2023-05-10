/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/p2a/opportunities/p2a_opportunity_questions.h"

#include "base/check.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

namespace brave_ads::privacy::p2a {

namespace {
constexpr char kQuestionPrefix[] = "Brave.P2A.AdOpportunitiesPerSegment.";
}  // namespace

std::vector<std::string> CreateAdOpportunityQuestions(
    const SegmentList& segments) {
  std::vector<std::string> questions;

  const std::vector<std::string> parent_segments = GetParentSegments(segments);

  for (const auto& segment : parent_segments) {
    CHECK(!segment.empty());

    std::string stripped_segment = StripNonAlphaNumericCharacters(segment);

    base::ReplaceChars(stripped_segment, " ", "", &stripped_segment);
    CHECK(!stripped_segment.empty());

    const std::string question =
        base::StrCat({kQuestionPrefix, stripped_segment});

    questions.push_back(question);
  }

  questions.emplace_back("Brave.P2A.TotalAdOpportunities");

  return questions;
}

}  // namespace brave_ads::privacy::p2a
