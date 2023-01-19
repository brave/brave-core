/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tabs/tab_info.h"

namespace ads {

TabInfo::TabInfo() = default;

TabInfo::TabInfo(const TabInfo& other) = default;

TabInfo& TabInfo::operator=(const TabInfo& other) = default;

TabInfo::TabInfo(TabInfo&& other) noexcept = default;

TabInfo& TabInfo::operator=(TabInfo&& other) noexcept = default;

TabInfo::~TabInfo() = default;

bool TabInfo::operator==(const TabInfo& other) const {
  return id == other.id && redirect_chain == other.redirect_chain &&
         is_playing_media == other.is_playing_media;
}

bool TabInfo::operator!=(const TabInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
