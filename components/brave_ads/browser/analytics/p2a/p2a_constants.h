/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P2A_P2A_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P2A_P2A_CONSTANTS_H_

#include <cstddef>
#include <string_view>

#include "base/containers/fixed_flat_set.h"

namespace brave_ads {

inline constexpr size_t kP2AAnswerIndexIntervals[] = {0,     // 0
                                                      5,     // 1
                                                      10,    // 2
                                                      20,    // 3
                                                      50,    // 4
                                                      100,   // 5
                                                      250,   // 6
                                                      500};  // 7

inline constexpr auto kP2AAllowedNames = base::MakeFixedFlatSet<
    std::string_view>(
    {"Brave.P2A.ad_notification.opportunities",
     "Brave.P2A.ad_notification.opportunities_per_segment.architecture",
     "Brave.P2A.ad_notification.opportunities_per_segment.artsentertainment",
     "Brave.P2A.ad_notification.opportunities_per_segment.automotive",
     "Brave.P2A.ad_notification.opportunities_per_segment.business",
     "Brave.P2A.ad_notification.opportunities_per_segment.careers",
     "Brave.P2A.ad_notification.opportunities_per_segment.cellphones",
     "Brave.P2A.ad_notification.opportunities_per_segment.crypto",
     "Brave.P2A.ad_notification.opportunities_per_segment.education",
     "Brave.P2A.ad_notification.opportunities_per_segment.familyparenting",
     "Brave.P2A.ad_notification.opportunities_per_segment.fashion",
     "Brave.P2A.ad_notification.opportunities_per_segment.folklore",
     "Brave.P2A.ad_notification.opportunities_per_segment.fooddrink",
     "Brave.P2A.ad_notification.opportunities_per_segment.gaming",
     "Brave.P2A.ad_notification.opportunities_per_segment.healthfitness",
     "Brave.P2A.ad_notification.opportunities_per_segment.history",
     "Brave.P2A.ad_notification.opportunities_per_segment.hobbiesinterests",
     "Brave.P2A.ad_notification.opportunities_per_segment.home",
     "Brave.P2A.ad_notification.opportunities_per_segment.law",
     "Brave.P2A.ad_notification.opportunities_per_segment.military",
     "Brave.P2A.ad_notification.opportunities_per_segment.other",
     "Brave.P2A.ad_notification.opportunities_per_segment.personalfinance",
     "Brave.P2A.ad_notification.opportunities_per_segment.pets",
     "Brave.P2A.ad_notification.opportunities_per_segment.realestate",
     "Brave.P2A.ad_notification.opportunities_per_segment.science",
     "Brave.P2A.ad_notification.opportunities_per_segment.sports",
     "Brave.P2A.ad_notification.opportunities_per_segment.technologycomputing",
     "Brave.P2A.ad_notification.opportunities_per_segment.travel",
     "Brave.P2A.ad_notification.opportunities_per_segment.untargeted",
     "Brave.P2A.ad_notification.opportunities_per_segment.weather",
     "Brave.P2A.inline_content_ad.opportunities",
     "Brave.P2A.new_tab_page_ad.opportunities"});

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P2A_P2A_CONSTANTS_H_
