/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ANALYTICS_P2A_P2A_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ANALYTICS_P2A_P2A_H_

#include <string>
#include <vector>

namespace brave_ads::p2a {

void RecordEvent(const std::vector<std::string>& events);

}  // namespace brave_ads::p2a

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ANALYTICS_P2A_P2A_H_
