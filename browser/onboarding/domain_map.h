/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ONBOARDING_DOMAIN_MAP_H_
#define BRAVE_BROWSER_ONBOARDING_DOMAIN_MAP_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "url/gurl.h"

namespace domain_map {
std::string GetCompanyNameFromGURL(const GURL url);
std::pair<std::string, int> GetCompanyNamesAndCountFromAdsList(
    std::vector<GURL> ads_list);
}  // namespace domain_map

#endif  // BRAVE_BROWSER_ONBOARDING_DOMAIN_MAP_H_
