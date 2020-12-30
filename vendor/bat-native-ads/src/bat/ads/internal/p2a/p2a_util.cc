/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/p2a/p2a_util.h"

#include "bat/ads/internal/ad_targeting/ad_targeting_util.h"

namespace ads {
namespace p2a {

namespace {
const char kOpportunityQuestionPrefix[] =
    "Brave.P2A.AdOpportunitiesPerSegment.";
const char kImpressionQuestionPrefix[] =
    "Brave.P2A.AdImpressionsPerSegment.";
}  // namespace

std::vector<std::string> CreateAdOpportunityQuestionList(
    const std::vector<std::string>& segments) {
  std::vector<std::string> questions;

  std::vector<std::string> parent_segments = GetParentSegments(segments);

  for (auto& segment : parent_segments) {
    segment.erase(std::remove_if(segment.begin(), segment.end(),
        [](char c) { return !std::isalnum(c); }), segment.end());

    std::string question = kOpportunityQuestionPrefix;
    question.append(segment);

    questions.push_back(question);
  }

  questions.push_back("Brave.P2A.TotalAdOpportunities");

  return questions;
}

std::vector<std::string> CreateAdImpressionQuestionList(
    const std::string& segment) {
  std::vector<std::string> questions;
  if (!segment.empty()) {
    std::string parent_segment = SplitSegment(segment).front();

    parent_segment.erase(
        std::remove_if(parent_segment.begin(), parent_segment.end(),
        [](char c) { return !std::isalnum(c); }), parent_segment.end());

    std::string question = kImpressionQuestionPrefix;
    question.append(parent_segment);
    questions.push_back(question);
  }

  questions.push_back("Brave.P2A.TotalAdImpressions");

  return questions;
}

}  // namespace p2a
}  // namespace ads
