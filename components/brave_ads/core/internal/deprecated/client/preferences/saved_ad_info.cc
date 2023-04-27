/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/saved_ad_info.h"

namespace brave_ads {

base::Value::Dict SavedAdInfo::ToValue() const {
  base::Value::Dict dict;
  dict.Set("uuid", creative_instance_id);
  return dict;
}

void SavedAdInfo::FromValue(const base::Value::Dict& dict) {
  if (const auto* const value = dict.FindString("uuid")) {
    creative_instance_id = *value;
  }
}

}  // namespace brave_ads
