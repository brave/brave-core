/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/p2a/impressions/p2a_impression.h"

#include <string>
#include <vector>

#include "base/strings/stringprintf.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/internal/privacy/p2a/impressions/p2a_impression_questions.h"
#include "bat/ads/internal/privacy/p2a/p2a.h"

namespace ads {
namespace privacy {
namespace p2a {

void RecordAdImpression(const AdInfo& ad) {
  const std::string name =
      base::StringPrintf("%s_impression", ad.type.ToString().c_str());

  const std::vector<std::string> questions =
      CreateAdImpressionQuestions(ad.segment);

  RecordEvent(name, questions);
}

}  // namespace p2a
}  // namespace privacy
}  // namespace ads
