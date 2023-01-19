/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/p2a/impressions/p2a_impression.h"

#include <vector>

#include "base/strings/stringprintf.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/internal/privacy/p2a/impressions/p2a_impression_questions.h"
#include "bat/ads/internal/privacy/p2a/p2a.h"

namespace ads::privacy::p2a {

std::string GetAdImpressionNameForAdType(const AdType& ad_type) {
  return base::StringPrintf("%s_impression", ad_type.ToString().c_str());
}

void RecordAdImpression(const AdInfo& ad) {
  const std::string name = GetAdImpressionNameForAdType(ad.type);

  const std::vector<std::string> questions =
      CreateAdImpressionQuestions(ad.segment);

  RecordEvent(name, questions);
}

}  // namespace ads::privacy::p2a
