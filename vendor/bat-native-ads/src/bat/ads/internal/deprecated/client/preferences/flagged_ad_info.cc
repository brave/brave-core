/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/deprecated/client/preferences/flagged_ad_info.h"

namespace ads {

FlaggedAdInfo::FlaggedAdInfo() = default;

FlaggedAdInfo::FlaggedAdInfo(const FlaggedAdInfo& info) = default;

FlaggedAdInfo& FlaggedAdInfo::operator=(const FlaggedAdInfo& info) = default;

FlaggedAdInfo::~FlaggedAdInfo() = default;

base::Value::Dict FlaggedAdInfo::ToValue() const {
  base::Value::Dict dict;
  dict.Set("creative_set_id", creative_set_id);
  return dict;
}

bool FlaggedAdInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindString("creative_set_id")) {
    creative_set_id = *value;
  }
  return true;
}

}  // namespace ads
