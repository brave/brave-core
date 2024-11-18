/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SEGMENTS_SEGMENT_TEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SEGMENTS_SEGMENT_TEST_CONSTANTS_H_

namespace brave_ads::test {

inline constexpr auto kSegments =
    std::to_array<std::string_view>({"architecture",
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
                                     "other",
                                     "personal finance",
                                     "pets",
                                     "real estate",
                                     "science",
                                     "sports",
                                     "technology & computing",
                                     "travel",
                                     "weather"});

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SEGMENTS_SEGMENT_TEST_CONSTANTS_H_
