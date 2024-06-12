/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/filtered_advertiser_info.h"

namespace brave_ads {

base::Value::Dict FilteredAdvertiserInfo::ToValue() const {
  return base::Value::Dict().Set("id", id);
}

void FilteredAdvertiserInfo::FromValue(const base::Value::Dict& dict) {
  if (const auto* const value = dict.FindString("id")) {
    id = *value;
  }
}

}  // namespace brave_ads
