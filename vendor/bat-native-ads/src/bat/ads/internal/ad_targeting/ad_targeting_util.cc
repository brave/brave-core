/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting_util.h"

#include "base/strings/string_split.h"

namespace ads {
namespace ad_targeting {

namespace {
const char kCategorySeparator[] = "-";
}  // namespace

std::vector<std::string> SplitCategory(
    const std::string& category) {
  return base::SplitString(category, kCategorySeparator, base::KEEP_WHITESPACE,
      base::SPLIT_WANT_ALL);
}

CategoryList GetParentCategories(
    const CategoryList& categories) {
  CategoryList parent_categories;

  for (const auto& category : categories) {
    const std::vector<std::string> components = SplitCategory(category);

    const std::string parent_category = components.front();
    if (std::find(parent_categories.begin(), parent_categories.end(),
        parent_category) != parent_categories.end()) {
      continue;
    }

    parent_categories.push_back(parent_category);
  }

  return parent_categories;
}

}  // namespace ad_targeting
}  // namespace ads
