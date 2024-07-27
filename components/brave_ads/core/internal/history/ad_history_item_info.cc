/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

AdHistoryItemInfo::AdHistoryItemInfo() = default;

AdHistoryItemInfo::AdHistoryItemInfo(const AdHistoryItemInfo& other) = default;

AdHistoryItemInfo& AdHistoryItemInfo::operator=(
    const AdHistoryItemInfo& other) = default;

AdHistoryItemInfo::AdHistoryItemInfo(AdHistoryItemInfo&& other) noexcept =
    default;

AdHistoryItemInfo& AdHistoryItemInfo::operator=(
    AdHistoryItemInfo&& other) noexcept = default;

AdHistoryItemInfo::~AdHistoryItemInfo() = default;

}  // namespace brave_ads
