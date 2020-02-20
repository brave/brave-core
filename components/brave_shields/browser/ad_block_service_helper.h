/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_HELPER_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/vendor/adblock_rust_ffi/src/wrapper.hpp"

namespace brave_shields {

std::vector<adblock::FilterList>::const_iterator FindAdBlockFilterListByUUID(
    const std::vector<adblock::FilterList>& region_lists,
    const std::string& uuid);
std::vector<adblock::FilterList>::const_iterator FindAdBlockFilterListByLocale(
    const std::vector<adblock::FilterList>& region_lists,
    const std::string& locale);

void MergeResourcesInto(base::Value* into, base::Value* from);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_HELPER_H_
