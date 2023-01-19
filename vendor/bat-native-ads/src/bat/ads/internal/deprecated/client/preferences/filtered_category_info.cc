/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/deprecated/client/preferences/filtered_category_info.h"

namespace ads {

base::Value::Dict FilteredCategoryInfo::ToValue() const {
  base::Value::Dict dict;
  dict.Set("name", name);
  return dict;
}

void FilteredCategoryInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindString("name")) {
    name = *value;
  }
}

}  // namespace ads
