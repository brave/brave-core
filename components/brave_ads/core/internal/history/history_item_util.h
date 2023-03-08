/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_ITEM_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_ITEM_UTIL_H_

#include <string>

namespace ads {

class ConfirmationType;
struct AdInfo;
struct HistoryItemInfo;

HistoryItemInfo BuildHistoryItem(const AdInfo& ad,
                                 const ConfirmationType& confirmation_type,
                                 const std::string& title,
                                 const std::string& description);

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_ITEM_UTIL_H_
