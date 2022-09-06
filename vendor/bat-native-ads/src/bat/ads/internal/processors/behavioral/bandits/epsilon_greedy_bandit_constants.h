/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_CONSTANTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_CONSTANTS_H_

#include "bat/ads/internal/segments/segment_alias.h"

namespace ads::targeting {

const SegmentList kSegments = {"architecture",
                               "arts & entertainment",
                               "automotive",
                               "business",
                               "careers",
                               "cell phones",
                               "drugs",
                               "education",
                               "family & parenting",
                               "fashion",
                               "folklore",
                               "food & drink",
                               "health & fitness",
                               "history",
                               "hobbies & interests",
                               "home",
                               "law",
                               "military",
                               "personal finance",
                               "pets",
                               "politics",
                               "real estate",
                               "religion",
                               "science",
                               "society",
                               "sports",
                               "technology & computing",
                               "travel",
                               "weather",
                               "crypto"};

}  // namespace ads::targeting

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_CONSTANTS_H_
