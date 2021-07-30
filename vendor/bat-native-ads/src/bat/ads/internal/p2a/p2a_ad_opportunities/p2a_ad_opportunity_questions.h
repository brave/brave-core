/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_P2A_P2A_AD_OPPORTUNITIES_P2A_AD_OPPORTUNITY_QUESTIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_P2A_P2A_AD_OPPORTUNITIES_P2A_AD_OPPORTUNITY_QUESTIONS_H_

#include <string>
#include <vector>

namespace ads {
namespace p2a {

std::vector<std::string> CreateAdOpportunityQuestions(
    const std::vector<std::string>& segments);

}  // namespace p2a
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_P2A_P2A_AD_OPPORTUNITIES_P2A_AD_OPPORTUNITY_QUESTIONS_H_
