/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_UTIL_H_

#include "base/types/optional_ref.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

namespace brave_ads {

struct NotificationAdInfo;

mojom::NotificationAdInfoPtr ToMojom(
    base::optional_ref<const NotificationAdInfo> ad);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_UTIL_H_
