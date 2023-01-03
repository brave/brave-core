/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/p2a/impressions/p2a_impression_questions.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/common/strings/string_strip_util.h"
#include "bat/ads/internal/segments/segment_util.h"

namespace ads::privacy::p2a {

namespace {
constexpr char kQuestionPrefix[] = "Brave.P2A.AdImpressionsPerSegment.";
}  // namespace

std::vector<std::string> CreateAdImpressionQuestions(
    const std::string& segment) {
  std::vector<std::string> questions;

  const std::string parent_segment = GetParentSegment(segment);
  DCHECK(!parent_segment.empty());

  std::string stripped_parent_segment =
      StripNonAlphaNumericCharacters(parent_segment);

  base::ReplaceChars(stripped_parent_segment, " ", "",
                     &stripped_parent_segment);

  const std::string question = base::StringPrintf(
      "%s%s", kQuestionPrefix, stripped_parent_segment.c_str());

  questions.push_back(question);

  questions.emplace_back("Brave.P2A.TotalAdImpressions");

  return questions;
}

}  // namespace ads::privacy::p2a
