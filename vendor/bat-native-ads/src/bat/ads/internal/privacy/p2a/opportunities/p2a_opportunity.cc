/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/p2a/opportunities/p2a_opportunity.h"

#include <vector>

#include "base/strings/stringprintf.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/internal/privacy/p2a/opportunities/p2a_opportunity_questions.h"
#include "bat/ads/internal/privacy/p2a/p2a.h"

namespace ads::privacy::p2a {

std::string GetAdOpportunityNameForAdType(const AdType& ad_type) {
  return base::StringPrintf("%s_opportunity", ad_type.ToString().c_str());
}

void RecordAdOpportunityForSegments(const AdType& ad_type,
                                    const SegmentList& segments) {
  const std::string name = GetAdOpportunityNameForAdType(ad_type);

  const std::vector<std::string> questions =
      CreateAdOpportunityQuestions(segments);

  RecordEvent(name, questions);
}

}  // namespace ads::privacy::p2a
