/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_segments.h"

#include "base/no_destructor.h"

namespace brave_ads {

const SegmentList& SupportedEpsilonGreedyBanditSegments() {
  static const base::NoDestructor<SegmentList> kSegments(
      {"architecture",
       "arts & entertainment",
       "automotive",
       "business",
       "careers",
       "cell phones",
       "crypto",
       "education",
       "family & parenting",
       "fashion",
       "folklore",
       "food & drink",
       "gaming",
       "health & fitness",
       "history",
       "hobbies & interests",
       "home",
       "law",
       "military",
       "personal finance",
       "pets",
       "real estate",
       "science",
       "sports",
       "technology & computing",
       "travel",
       "weather"});

  return *kSegments;
}

}  // namespace brave_ads
