/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"

#include <utility>

namespace brave_ads {

TabInfo::TabInfo() = default;

TabInfo::TabInfo(const int32_t id,
                 std::vector<GURL> redirect_chain,
                 const int32_t http_response_status_code,
                 const bool is_playing_media)
    : id(id),
      redirect_chain(std::move(redirect_chain)),
      http_response_status_code(http_response_status_code),
      is_playing_media(is_playing_media) {}

TabInfo::TabInfo(const TabInfo& other) = default;

TabInfo& TabInfo::operator=(const TabInfo& other) = default;

TabInfo::TabInfo(TabInfo&& other) noexcept = default;

TabInfo& TabInfo::operator=(TabInfo&& other) noexcept = default;

TabInfo::~TabInfo() = default;

}  // namespace brave_ads
