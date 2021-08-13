/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CLIENT_PREFERENCES_FILTERED_CATEGORY_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CLIENT_PREFERENCES_FILTERED_CATEGORY_INFO_H_

#include <string>
#include <vector>

namespace ads {

struct FilteredCategoryInfo {
  FilteredCategoryInfo();
  FilteredCategoryInfo(const FilteredCategoryInfo& category);
  ~FilteredCategoryInfo();

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  std::string name;
};

using FilteredCategoryList = std::vector<FilteredCategoryInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CLIENT_PREFERENCES_FILTERED_CATEGORY_INFO_H_
