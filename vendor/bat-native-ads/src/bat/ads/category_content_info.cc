/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/category_content_info.h"

#include "base/values.h"

namespace ads {

CategoryContentInfo::CategoryContentInfo() = default;

CategoryContentInfo::CategoryContentInfo(const CategoryContentInfo& info) =
    default;

CategoryContentInfo& CategoryContentInfo::operator=(
    const CategoryContentInfo& info) = default;

CategoryContentInfo::~CategoryContentInfo() = default;

bool CategoryContentInfo::operator==(const CategoryContentInfo& rhs) const {
  return category == rhs.category && opt_action_type == rhs.opt_action_type;
}

bool CategoryContentInfo::operator!=(const CategoryContentInfo& rhs) const {
  return !(*this == rhs);
}

base::Value::Dict CategoryContentInfo::ToValue() const {
  base::Value::Dict dictionary;

  dictionary.Set("category", category);
  dictionary.Set("optAction", static_cast<int>(opt_action_type));

  return dictionary;
}

void CategoryContentInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindString("category")) {
    category = *value;
  }

  if (const auto value = root.FindInt("optAction")) {
    opt_action_type = static_cast<CategoryContentOptActionType>(*value);
  } else if (const auto value = root.FindInt("opt_action")) {
    // Migrate legacy
    opt_action_type = static_cast<CategoryContentOptActionType>(*value);
  }
}

}  // namespace ads
