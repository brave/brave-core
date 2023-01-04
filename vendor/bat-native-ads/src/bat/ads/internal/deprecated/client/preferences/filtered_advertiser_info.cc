/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/deprecated/client/preferences/filtered_advertiser_info.h"

namespace ads {

base::Value::Dict FilteredAdvertiserInfo::ToValue() const {
  base::Value::Dict dict;
  dict.Set("id", id);
  return dict;
}

void FilteredAdvertiserInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindString("id")) {
    id = *value;
  }
}

}  // namespace ads
