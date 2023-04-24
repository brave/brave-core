/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/flagged_ad_info.h"

namespace brave_ads {

base::Value::Dict FlaggedAdInfo::ToValue() const {
  base::Value::Dict dict;
  dict.Set("creative_set_id", creative_set_id);
  return dict;
}

void FlaggedAdInfo::FromValue(const base::Value::Dict& dict) {
  if (const auto* const value = dict.FindString("creative_set_id")) {
    creative_set_id = *value;
  }
}

}  // namespace brave_ads
