/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_HISTORY_AD_HISTORY_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_HISTORY_AD_HISTORY_CALLBACK_H_

#include <optional>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

namespace brave_ads {

using GetAdHistoryCallback =
    base::OnceCallback<void(std::optional<AdHistoryList> ad_history)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_HISTORY_AD_HISTORY_CALLBACK_H_
