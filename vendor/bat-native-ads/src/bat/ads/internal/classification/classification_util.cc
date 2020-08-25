/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/classification/classification_util.h"

#include "base/strings/string_split.h"

namespace ads {
namespace classification {

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
    std::string parent_category;

    const size_t pos = category.find_last_of(kCategorySeparator);
    if (pos != std::string::npos) {
      parent_category = category.substr(0, pos);
    } else {
      parent_category = category;
    }

    if (std::find(parent_categories.begin(), parent_categories.end(),
        parent_category) != parent_categories.end()) {
      continue;
    }

    parent_categories.push_back(parent_category);
  }

  return parent_categories;
}

}  // namespace classification
}  // namespace ads
