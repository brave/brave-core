/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_segments.h"

namespace {

constexpr base::StringPiece kSegments[] = {"architecture",
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
}  // namespace

namespace ads::targeting {

base::span<const base::StringPiece> GetSegments() {
  return kSegments;
}

}  // namespace ads::targeting
