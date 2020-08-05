/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLASSIFICATION_CLASSIFICATION_UTIL_H_
#define BAT_ADS_INTERNAL_CLASSIFICATION_CLASSIFICATION_UTIL_H_

#include <string>
#include <vector>

namespace ads {
namespace classification {

const char kCategorySeparator[] = "-";

std::vector<std::string> SplitCategory(
    const std::string& category);

}  // namespace classification
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLASSIFICATION_CLASSIFICATION_UTIL_H_
