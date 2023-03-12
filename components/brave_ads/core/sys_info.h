/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_SYS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_SYS_INFO_H_

#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"

namespace ads {

// Returns system information. |device_id| containing machine characteristics
// which should not be stored to disk or transmitted. |is_uncertain_future|
// containing |true| for guest operating systems otherwise |false|.
mojom::SysInfo& SysInfo();

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_SYS_INFO_H_
