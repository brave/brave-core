/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/p2a/p2a_ad_impressions/p2a_ad_impression.h"

#include <string>
#include <vector>

#include "base/strings/stringprintf.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/internal/p2a/p2a.h"
#include "bat/ads/internal/p2a/p2a_ad_impressions/p2a_ad_impression_questions.h"

namespace ads {
namespace p2a {

void RecordAdImpression(const AdInfo& ad) {
  const std::string type_as_string = std::string(ad.type);

  const std::string name =
      base::StringPrintf("%s_impression", type_as_string.c_str());

  const std::vector<std::string> questions =
      CreateAdImpressionQuestions(ad.segment);

  RecordEvent(name, questions);
}

}  // namespace p2a
}  // namespace ads
