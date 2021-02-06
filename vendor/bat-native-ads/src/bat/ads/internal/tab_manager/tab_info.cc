/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tab_manager/tab_info.h"

namespace ads {

TabInfo::TabInfo() = default;

TabInfo::TabInfo(const TabInfo& info) = default;

TabInfo::~TabInfo() = default;

bool TabInfo::operator==(const TabInfo& rhs) const {
  return id == rhs.id && url == rhs.url &&
         is_playing_media == rhs.is_playing_media;
}

bool TabInfo::operator!=(const TabInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
